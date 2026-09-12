#include "plugin.h"
#include "platform_offsets.h"
#include "skin_catalog.h"
#include "ics2menus.h"

#include <iserver.h>
#include <tier1/convar.h>

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <string>

namespace
{
IServerGameDLL* g_server = nullptr;
IServerGameClients* g_gameClients = nullptr;
ICvar* g_cvar = nullptr;
ISchemaSystem* g_schemaSystem = nullptr;

bool ParseInt(const char* text, int& value)
{
    if (!text || !*text)
        return false;

    errno = 0;
    char* end = nullptr;
    const long parsed = std::strtol(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' || parsed < INT_MIN || parsed > INT_MAX)
        return false;

    value = static_cast<int>(parsed);
    return true;
}

bool ParseFloat(const char* text, float& value)
{
    if (!text || !*text)
        return false;

    errno = 0;
    char* end = nullptr;
    const float parsed = std::strtof(text, &end);
    if (errno != 0 || end == text || *end != '\0' || !std::isfinite(parsed))
        return false;

    value = parsed;
    return true;
}

void Reply(CPlayerSlot slot, const char* text)
{
    if (slot.IsValid())
        g_SMAPI->ClientConPrintf(slot, "%s", text);
    else
        META_CONPRINTF("%s", text);
}

std::string NormalizeChatCommand(const char* raw)
{
    if (!raw)
        return {};

    std::string text(raw);
    auto isSpace = [](unsigned char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; };

    while (!text.empty() && isSpace(static_cast<unsigned char>(text.front())))
        text.erase(text.begin());
    while (!text.empty() && isSpace(static_cast<unsigned char>(text.back())))
        text.pop_back();

    if (text.size() >= 2 && text.front() == '"' && text.back() == '"')
    {
        text.erase(text.begin());
        text.pop_back();
    }

    while (!text.empty() && isSpace(static_cast<unsigned char>(text.front())))
        text.erase(text.begin());
    while (!text.empty() && isSpace(static_cast<unsigned char>(text.back())))
        text.pop_back();

    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) {
        if (c >= 'A' && c <= 'Z')
            return static_cast<char>(c - 'A' + 'a');
        return static_cast<char>(c);
    });
    return text;
}

bool IsSkinChatCommand(const std::string& text)
{
    return text == "!skin" || text == "/skin" || text == "!skins" || text == "/skins";
}
}

KHookSkinChanger g_KHookSkinChanger;
PLUGIN_EXPOSE(KHookSkinChanger, g_KHookSkinChanger);

CON_COMMAND_F(kh_skin, "kh_skin <paintkit> [seed] [wear] [stattrak] - apply to active weapon",
              FCVAR_CLIENT_CAN_EXECUTE | FCVAR_GAMEDLL)
{
    const CPlayerSlot slot = context.GetPlayerSlot();
    if (!slot.IsValid())
    {
        META_CONPRINTF("[KHSKIN] kh_skin is a player command.\n");
        return;
    }
    g_KHookSkinChanger.CommandSetSkin(slot, args);
}

CON_COMMAND_F(kh_skin_menu, "Open KHook SkinChanger HTML menu",
              FCVAR_CLIENT_CAN_EXECUTE | FCVAR_GAMEDLL)
{
    const CPlayerSlot slot = context.GetPlayerSlot();
    if (!slot.IsValid())
    {
        META_CONPRINTF("[KHSKIN] kh_skin_menu is a player command.\n");
        return;
    }
    g_KHookSkinChanger.CommandOpenSkinMenu(slot);
}

CON_COMMAND_F(kh_skin_clear, "Clear saved skin for the active weapon",
              FCVAR_CLIENT_CAN_EXECUTE | FCVAR_GAMEDLL)
{
    const CPlayerSlot slot = context.GetPlayerSlot();
    if (!slot.IsValid())
    {
        META_CONPRINTF("[KHSKIN] kh_skin_clear is a player command.\n");
        return;
    }
    g_KHookSkinChanger.CommandClearSkin(slot);
}

CON_COMMAND_F(kh_skin_info, "Show active weapon definition and selected skin",
              FCVAR_CLIENT_CAN_EXECUTE | FCVAR_GAMEDLL)
{
    const CPlayerSlot slot = context.GetPlayerSlot();
    if (!slot.IsValid())
    {
        META_CONPRINTF("[KHSKIN] kh_skin_info is a player command.\n");
        return;
    }
    g_KHookSkinChanger.CommandSkinInfo(slot);
}

KHookSkinChanger::KHookSkinChanger()
    : m_gameFrameHook(&IServerGameDLL::GameFrame, this, nullptr, &KHookSkinChanger::Hook_GameFrame),
      m_clientDisconnectHook(&IServerGameClients::ClientDisconnect, this, nullptr,
                             &KHookSkinChanger::Hook_ClientDisconnect),
      m_dispatchConCommandHook(&ICvar::DispatchConCommand, this,
                               &KHookSkinChanger::Hook_DispatchConCommand, nullptr)
{
    m_mainMenu = kInvalidMenuHandle;
}

bool KHookSkinChanger::Load(PluginId id, ISmmAPI* ismm, char* error, size_t maxlen, bool late)
{
    PLUGIN_SAVEVARS();

    GET_V_IFACE_ANY(GetServerFactory, g_server, IServerGameDLL, INTERFACEVERSION_SERVERGAMEDLL);
    GET_V_IFACE_ANY(GetServerFactory, g_gameClients, IServerGameClients, INTERFACEVERSION_SERVERGAMECLIENTS);
    GET_V_IFACE_CURRENT(GetEngineFactory, g_cvar, ICvar, CVAR_INTERFACE_VERSION);
    GET_V_IFACE_ANY(GetEngineFactory, g_schemaSystem, ISchemaSystem, SCHEMASYSTEM_INTERFACE_VERSION);
    GET_V_IFACE_CURRENT(GetEngineFactory, m_gameResourceService, IGameResourceService,
                        GAMERESOURCESERVICESERVER_INTERFACE_VERSION);

    m_schema.Initialize(g_schemaSystem);
    if (!m_schema.IsReady())
    {
        std::snprintf(error, maxlen, "SchemaSystem server type scope is unavailable");
        return false;
    }

    g_SMAPI->AddListener(this, this);
    m_gameFrameHook.Add(g_server);
    m_clientDisconnectHook.Add(g_gameClients);
    m_dispatchConCommandHook.Add(g_cvar);

    g_pCVar = g_cvar;
    META_CONVAR_REGISTER(FCVAR_RELEASE | FCVAR_CLIENT_CAN_EXECUTE | FCVAR_GAMEDLL);

    RefreshEntitySystem();
    AcquireMenus();

    META_CONPRINTF("[KHSKIN] Loaded %s %s using KHook.\n", PLUGIN_DISPLAY_NAME, PLUGIN_FULL_VERSION);
    META_CONPRINTF("[KHSKIN] Chat: !skin | console: kh_skin_menu, kh_skin, kh_skin_clear, kh_skin_info\n");
    if (!m_menus)
        META_CONPRINTF("[KHSKIN] CS2Menus (ICS2Menus003) is not loaded; !skin will become available when it loads.\n");
    return true;
}

bool KHookSkinChanger::Unload(char* error, size_t maxlen)
{
    (void)error;
    (void)maxlen;

    DropMenus();

    if (g_server)
        m_gameFrameHook.Remove(g_server);
    if (g_gameClients)
        m_clientDisconnectHook.Remove(g_gameClients);
    if (g_cvar)
        m_dispatchConCommandHook.Remove(g_cvar);

    for (int i = 0; i < kMaxPlayers; ++i)
        m_skins[i].clear();

    m_entitySystem = nullptr;
    m_menus = nullptr;
    return true;
}

void KHookSkinChanger::AllPluginsLoaded()
{
    AcquireMenus();
}

void KHookSkinChanger::OnPluginLoad(PluginId /*id*/)
{
    AcquireMenus();
}

void KHookSkinChanger::OnPluginUnload(PluginId /*id*/)
{
    // CS2Menus may be unloaded/reloaded independently. Re-resolve the factory
    // instead of retaining a pointer into an unloaded Metamod plugin.
    AcquireMenus();
}

void KHookSkinChanger::OnLevelInit(const char* mapName, const char* mapEntities,
                                   const char* oldLevel, const char* landmarkName,
                                   bool loadGame, bool background)
{
    (void)mapEntities;
    (void)oldLevel;
    (void)landmarkName;
    (void)loadGame;
    (void)background;

    m_schema.ResetCache();
    RefreshEntitySystem();
    AcquireMenus();
    META_CONPRINTF("[KHSKIN] Level init: %s\n", mapName ? mapName : "<unknown>");
}

void KHookSkinChanger::OnLevelShutdown()
{
    m_entitySystem = nullptr;
    m_frameCounter = 0;
}

KHook::Return<void> KHookSkinChanger::Hook_GameFrame(IServerGameDLL*, bool simulating,
                                                      bool firstTick, bool lastTick)
{
    (void)firstTick;
    (void)lastTick;

    if (!simulating)
        return { KHook::Action::Ignore };

    if (!m_entitySystem)
        RefreshEntitySystem();

    if (m_entitySystem && ++m_frameCounter >= kApplyEveryNFrames)
    {
        m_frameCounter = 0;
        TickPlayers();
    }

    return { KHook::Action::Ignore };
}

KHook::Return<void> KHookSkinChanger::Hook_ClientDisconnect(IServerGameClients*, CPlayerSlot slot,
                                                             ENetworkDisconnectionReason reason,
                                                             const char* name, uint64 xuid,
                                                             const char* networkId)
{
    (void)reason;
    (void)name;
    (void)xuid;
    (void)networkId;
    ClearSlot(slot);
    return { KHook::Action::Ignore };
}

KHook::Return<void> KHookSkinChanger::Hook_DispatchConCommand(ICvar*, ConCommandRef /*command*/,
                                                               const CCommandContext& context,
                                                               const CCommand& args)
{
    const char* commandName = args.Arg(0);
    if (!commandName || (std::strcmp(commandName, "say") != 0 && std::strcmp(commandName, "say_team") != 0))
        return { KHook::Action::Ignore };

    const CPlayerSlot slot = context.GetPlayerSlot();
    if (!slot.IsValid() || slot.Get() < 0 || slot.Get() >= kMaxPlayers)
        return { KHook::Action::Ignore };

    const std::string text = NormalizeChatCommand(args.ArgS());
    if (!IsSkinChatCommand(text))
        return { KHook::Action::Ignore };

    OpenSkinMenu(slot);
    return { KHook::Action::Supersede };
}

void KHookSkinChanger::AcquireMenus()
{
    ICS2Menus* resolved = reinterpret_cast<ICS2Menus*>(g_SMAPI->MetaFactory(CS2MENUS_INTERFACE, nullptr, nullptr));
    if (!resolved)
    {
        // Do not touch old menu handles here: if the provider disappeared,
        // their owner DLL may already be gone. Just forget the interface.
        m_menus = nullptr;
        ForgetMenus();
        return;
    }

    if (resolved != m_menus)
    {
        m_menus = resolved;
        ForgetMenus();
    }

    if (m_mainMenu == kInvalidMenuHandle)
        BuildMenus();
}

void KHookSkinChanger::BuildMenus()
{
    if (!m_menus || m_mainMenu != kInvalidMenuHandle)
        return;

    m_mainMenu = m_menus->CreateMenu(MenuType::Html, "SKIN CHANGER", nullptr);
    if (m_mainMenu == kInvalidMenuHandle)
        return;

    m_ownedMenus.push_back(m_mainMenu);
    m_menus->SetMenuForceType(m_mainMenu, true);
    m_menus->SetExitButton(m_mainMenu, true);
    m_menus->SetCloseOnSelect(m_mainMenu, false);

    for (const CatalogGroup& group : GetSkinCatalog())
    {
        MenuHandle groupMenu = m_menus->CreateMenu(MenuType::Html, group.name, nullptr);
        if (groupMenu == kInvalidMenuHandle)
            continue;

        m_ownedMenus.push_back(groupMenu);
        m_menus->SetMenuForceType(groupMenu, true);
        m_menus->SetExitButton(groupMenu, true);
        m_menus->SetCloseOnSelect(groupMenu, false);

        for (const CatalogWeapon& weapon : group.weapons)
        {
            MenuHandle skinMenu = m_menus->CreateMenu(MenuType::Html, weapon.name,
                [](MenuHandle menu, int slot, int item)
                {
                    g_KHookSkinChanger.HandleSkinMenuSelection(static_cast<std::uint32_t>(menu), slot, item);
                });

            if (skinMenu == kInvalidMenuHandle)
                continue;

            m_ownedMenus.push_back(skinMenu);
            m_menus->SetMenuForceType(skinMenu, true);
            m_menus->SetExitButton(skinMenu, true);
            m_menus->SetCloseOnSelect(skinMenu, true);

            for (const CatalogSkin& skin : weapon.skins)
            {
                char info[128];
                std::snprintf(info, sizeof(info), "%u|%d|%d|%.7f|%d",
                              static_cast<unsigned>(weapon.itemDefinition), skin.paintKit,
                              skin.seed, skin.wear, skin.statTrak);
                m_menus->AddItem(skinMenu, skin.name, info, false);
            }

            m_menus->AddSubMenu(groupMenu, weapon.name, skinMenu, "");
        }

        m_menus->AddSubMenu(m_mainMenu, group.name, groupMenu, "");
    }

    META_CONPRINTF("[KHSKIN] HTML skin menu built using CS2Menus %s.\n", CS2MENUS_INTERFACE);
}

void KHookSkinChanger::DropMenus()
{
    if (m_menus)
    {
        for (auto it = m_ownedMenus.rbegin(); it != m_ownedMenus.rend(); ++it)
        {
            const MenuHandle handle = static_cast<MenuHandle>(*it);
            if (handle != kInvalidMenuHandle && m_menus->IsValidMenu(handle))
                m_menus->DestroyMenu(handle);
        }
    }
    ForgetMenus();
}

void KHookSkinChanger::ForgetMenus()
{
    m_ownedMenus.clear();
    m_mainMenu = kInvalidMenuHandle;
}

void KHookSkinChanger::OpenSkinMenu(CPlayerSlot slot)
{
    AcquireMenus();
    if (!m_menus || m_mainMenu == kInvalidMenuHandle)
    {
        Reply(slot, "[KHSKIN] !skin requires the native CS2Menus Metamod plugin (ICS2Menus003).\n");
        return;
    }

    m_menus->DisplayMenu(static_cast<MenuHandle>(m_mainMenu), slot.Get(), 0.0f);
}

void KHookSkinChanger::CommandOpenSkinMenu(CPlayerSlot slot)
{
    OpenSkinMenu(slot);
}

void KHookSkinChanger::HandleSkinMenuSelection(std::uint32_t menuValue, int slotNumber, int item)
{
    if (!m_menus || slotNumber < 0 || slotNumber >= kMaxPlayers)
        return;

    const MenuHandle menu = static_cast<MenuHandle>(menuValue);
    if (!m_menus->IsValidMenu(menu))
        return;

    const char* info = m_menus->GetItemInfo(menu, item);
    if (!info || !*info)
        return;

    unsigned definition = 0;
    SkinSelection selection;
    if (std::sscanf(info, "%u|%d|%d|%f|%d", &definition, &selection.paintKit,
                    &selection.seed, &selection.wear, &selection.statTrak) != 5 ||
        definition == 0 || definition > std::numeric_limits<uint16_t>::max())
    {
        META_CONPRINTF("[KHSKIN] Invalid menu item info: %s\n", info);
        return;
    }

    selection.wear = std::clamp(selection.wear, 0.000001f, 1.0f);

    const char* itemTextRaw = m_menus->GetItemText(menu, item);
    const char* weaponTextRaw = m_menus->GetTitle(menu);
    const std::string itemText = itemTextRaw ? itemTextRaw : "skin";
    const std::string weaponText = weaponTextRaw ? weaponTextRaw : "weapon";

    const CPlayerSlot slot(slotNumber);
    if (!SetSkinForDefinition(slot, static_cast<uint16_t>(definition), selection))
    {
        Reply(slot, "[KHSKIN] Could not save/apply this skin. Check server console for schema errors.\n");
        return;
    }

    char message[256];
    std::snprintf(message, sizeof(message), "[KHSKIN] Selected %s for %s (paint kit %d).\n",
                  itemText.c_str(), weaponText.c_str(), selection.paintKit);
    Reply(slot, message);
}

void KHookSkinChanger::RefreshEntitySystem()
{
    if (!m_gameResourceService)
    {
        m_entitySystem = nullptr;
        return;
    }

    auto base = reinterpret_cast<std::uintptr_t>(m_gameResourceService);
    auto location = reinterpret_cast<CGameEntitySystem**>(base + PlatformOffsets::kGameEntitySystem);
    m_entitySystem = location ? *location : nullptr;
}

CEntityIdentity* KHookSkinChanger::ResolveIdentity(CEntityIndex index) const
{
    if (!m_entitySystem)
        return nullptr;

    const int rawIndex = index.Get();
    if (rawIndex < 0 || rawIndex >= MAX_TOTAL_ENTITIES)
        return nullptr;

    const int listIndex = rawIndex / MAX_ENTITIES_IN_LIST;
    const int entryIndex = rawIndex % MAX_ENTITIES_IN_LIST;
    CEntityIdentity* chunk = m_entitySystem->m_EntityList.m_pIdentityChunks[listIndex];
    if (!chunk)
        return nullptr;

    CEntityIdentity* identity = &chunk[entryIndex];
    if (!identity->m_pInstance || identity->GetEntityIndex() != index)
        return nullptr;

    return identity;
}

CEntityInstance* KHookSkinChanger::ResolveEntity(CEntityIndex index) const
{
    CEntityIdentity* identity = ResolveIdentity(index);
    return identity ? identity->m_pInstance : nullptr;
}

CEntityInstance* KHookSkinChanger::ResolveEntity(const CEntityHandle& handle) const
{
    CEntityIdentity* identity = ResolveIdentity(handle.GetEntryIndex());
    if (!identity)
        return nullptr;

    // Validate the serial as well as the entry index. This avoids resolving a stale
    // handle after an entity slot has been recycled. Both operations are inline in
    // hl2sdk and do not create a runtime dependency on CEntitySystem symbols.
    if (identity->GetRefEHandle() != handle)
        return nullptr;

    return identity->m_pInstance;
}

CEntityInstance* KHookSkinChanger::GetController(CPlayerSlot slot) const
{
    if (!m_entitySystem || !slot.IsValid() || slot.Get() < 0 || slot.Get() >= kMaxPlayers)
        return nullptr;

    return ResolveEntity(CEntityIndex(slot.Get() + 1));
}

CEntityInstance* KHookSkinChanger::GetPawn(CPlayerSlot slot)
{
    CEntityInstance* controller = GetController(slot);
    if (!controller)
        return nullptr;

    CSchemaClassInfo* binding = controller->Schema_DynamicBinding().Get();
    const std::ptrdiff_t pawnHandleOffset = m_schema.FindOffset(binding, "m_hPlayerPawn");
    CEntityHandle* pawnHandle = FieldPtr<CEntityHandle>(controller, pawnHandleOffset);
    if (!pawnHandle)
        return nullptr;

    return ResolveEntity(*pawnHandle);
}

CEntityInstance* KHookSkinChanger::GetActiveWeapon(CPlayerSlot slot, uint16_t* itemDefinition)
{
    CEntityInstance* pawn = GetPawn(slot);
    if (!pawn)
        return nullptr;

    const std::ptrdiff_t weaponServicesOffset = m_schema.FindOffset(
        pawn->Schema_DynamicBinding().Get(), "m_pWeaponServices");
    void** weaponServicesField = FieldPtr<void*>(pawn, weaponServicesOffset);
    if (!weaponServicesField || !*weaponServicesField)
        return nullptr;

    const std::ptrdiff_t activeWeaponOffset = m_schema.FindOffset(
        "CPlayer_WeaponServices", "m_hActiveWeapon");
    CEntityHandle* activeWeaponHandle = FieldPtr<CEntityHandle>(*weaponServicesField, activeWeaponOffset);
    if (!activeWeaponHandle)
        return nullptr;

    CEntityInstance* weapon = ResolveEntity(*activeWeaponHandle);
    if (!weapon)
        return nullptr;

    if (itemDefinition)
    {
        uint16_t def = 0;
        if (!ReadItemDefinition(weapon, def))
            return nullptr;
        *itemDefinition = def;
    }

    return weapon;
}

bool KHookSkinChanger::ReadItemDefinition(CEntityInstance* weapon, uint16_t& itemDefinition)
{
    if (!weapon)
        return false;

    const std::ptrdiff_t attributeManagerOffset = m_schema.FindOffset(
        weapon->Schema_DynamicBinding().Get(), "m_AttributeManager");
    const std::ptrdiff_t itemOffset = m_schema.FindOffset("CAttributeContainer", "m_Item");
    const std::ptrdiff_t defIndexOffset = m_schema.FindOffset("CEconItemView", "m_iItemDefinitionIndex");

    if (attributeManagerOffset < 0 || itemOffset < 0 || defIndexOffset < 0)
        return false;

    void* attributeManager = FieldPtr<void>(weapon, attributeManagerOffset);
    void* itemView = FieldPtr<void>(attributeManager, itemOffset);
    uint16_t* defIndex = FieldPtr<uint16_t>(itemView, defIndexOffset);
    if (!defIndex)
        return false;

    itemDefinition = *defIndex;
    return itemDefinition != 0;
}

bool KHookSkinChanger::ApplySelection(CEntityInstance* weapon, const SkinSelection& selection)
{
    if (!weapon)
        return false;

    CSchemaClassInfo* binding = weapon->Schema_DynamicBinding().Get();
    const std::ptrdiff_t paintOffset = m_schema.FindOffset(binding, "m_nFallbackPaintKit");
    const std::ptrdiff_t seedOffset = m_schema.FindOffset(binding, "m_nFallbackSeed");
    const std::ptrdiff_t wearOffset = m_schema.FindOffset(binding, "m_flFallbackWear");
    const std::ptrdiff_t statOffset = m_schema.FindOffset(binding, "m_nFallbackStatTrak");
    const std::ptrdiff_t attributeManagerOffset = m_schema.FindOffset(binding, "m_AttributeManager");
    const std::ptrdiff_t itemOffset = m_schema.FindOffset("CAttributeContainer", "m_Item");
    const std::ptrdiff_t itemIdHighOffset = m_schema.FindOffset("CEconItemView", "m_iItemIDHigh");

    if (paintOffset < 0 || seedOffset < 0 || wearOffset < 0 || statOffset < 0 ||
        attributeManagerOffset < 0 || itemOffset < 0 || itemIdHighOffset < 0)
        return false;

    int* paint = FieldPtr<int>(weapon, paintOffset);
    int* seed = FieldPtr<int>(weapon, seedOffset);
    float* wear = FieldPtr<float>(weapon, wearOffset);
    int* statTrak = FieldPtr<int>(weapon, statOffset);

    void* attributeManager = FieldPtr<void>(weapon, attributeManagerOffset);
    void* itemView = FieldPtr<void>(attributeManager, itemOffset);
    uint32_t* itemIdHigh = FieldPtr<uint32_t>(itemView, itemIdHighOffset);

    if (!paint || !seed || !wear || !statTrak || !itemIdHigh)
        return false;

    bool changed = false;
    if (*paint != selection.paintKit) { *paint = selection.paintKit; changed = true; }
    if (*seed != selection.seed) { *seed = selection.seed; changed = true; }
    if (*wear != selection.wear) { *wear = selection.wear; changed = true; }
    if (*statTrak != selection.statTrak) { *statTrak = selection.statTrak; changed = true; }
    const uint32_t fallbackItemIdHigh = std::numeric_limits<uint32_t>::max();
    if (*itemIdHigh != fallbackItemIdHigh) { *itemIdHigh = fallbackItemIdHigh; changed = true; }

    if (changed)
        weapon->NetworkStateChanged(NetworkStateChangedData(true));

    return true;
}

bool KHookSkinChanger::SetSkinForDefinition(CPlayerSlot slot, uint16_t itemDefinition,
                                             const SkinSelection& selection)
{
    if (!slot.IsValid() || slot.Get() < 0 || slot.Get() >= kMaxPlayers || itemDefinition == 0)
        return false;

    m_skins[slot.Get()][itemDefinition] = selection;

    uint16_t activeDefinition = 0;
    CEntityInstance* active = GetActiveWeapon(slot, &activeDefinition);
    if (active && activeDefinition == itemDefinition)
        return ApplySelection(active, selection);

    return true;
}

bool KHookSkinChanger::ApplyForSlot(CPlayerSlot slot)
{
    if (!slot.IsValid() || slot.Get() < 0 || slot.Get() >= kMaxPlayers)
        return false;

    uint16_t itemDefinition = 0;
    CEntityInstance* weapon = GetActiveWeapon(slot, &itemDefinition);
    if (!weapon)
        return false;

    auto& selections = m_skins[slot.Get()];
    auto it = selections.find(itemDefinition);
    if (it == selections.end())
        return false;

    return ApplySelection(weapon, it->second);
}

void KHookSkinChanger::TickPlayers()
{
    for (int i = 0; i < kMaxPlayers; ++i)
    {
        if (m_skins[i].empty())
            continue;
        ApplyForSlot(CPlayerSlot(i));
    }
}

void KHookSkinChanger::ClearSlot(CPlayerSlot slot)
{
    if (!slot.IsValid() || slot.Get() < 0 || slot.Get() >= kMaxPlayers)
        return;
    m_skins[slot.Get()].clear();
}

void KHookSkinChanger::CommandSetSkin(CPlayerSlot slot, const CCommand& args)
{
    if (args.ArgC() < 2)
    {
        Reply(slot, "[KHSKIN] Usage: kh_skin <paintkit> [seed] [wear] [stattrak]\n");
        return;
    }

    SkinSelection selection;
    if (!ParseInt(args[1], selection.paintKit) || selection.paintKit < 0)
    {
        Reply(slot, "[KHSKIN] Invalid paint kit.\n");
        return;
    }

    if (args.ArgC() >= 3 && (!ParseInt(args[2], selection.seed) || selection.seed < 0))
    {
        Reply(slot, "[KHSKIN] Invalid seed.\n");
        return;
    }

    if (args.ArgC() >= 4 && !ParseFloat(args[3], selection.wear))
    {
        Reply(slot, "[KHSKIN] Invalid wear.\n");
        return;
    }
    selection.wear = std::clamp(selection.wear, 0.000001f, 1.0f);

    if (args.ArgC() >= 5 && (!ParseInt(args[4], selection.statTrak) || selection.statTrak < -1))
    {
        Reply(slot, "[KHSKIN] Invalid StatTrak value. Use -1 to disable it.\n");
        return;
    }

    uint16_t itemDefinition = 0;
    CEntityInstance* weapon = GetActiveWeapon(slot, &itemDefinition);
    if (!weapon)
    {
        Reply(slot, "[KHSKIN] Active econ weapon was not found. Hold a weapon and retry.\n");
        return;
    }

    m_skins[slot.Get()][itemDefinition] = selection;
    if (!ApplySelection(weapon, selection))
    {
        Reply(slot, "[KHSKIN] Schema fields for this weapon are unavailable on this build.\n");
        return;
    }

    char buffer[256];
    std::snprintf(buffer, sizeof(buffer),
                  "[KHSKIN] Applied: def=%u paint=%d seed=%d wear=%.6f stattrak=%d\n",
                  static_cast<unsigned>(itemDefinition), selection.paintKit, selection.seed,
                  selection.wear, selection.statTrak);
    Reply(slot, buffer);
}

void KHookSkinChanger::CommandClearSkin(CPlayerSlot slot)
{
    uint16_t itemDefinition = 0;
    CEntityInstance* weapon = GetActiveWeapon(slot, &itemDefinition);
    if (!weapon)
    {
        Reply(slot, "[KHSKIN] Active weapon was not found.\n");
        return;
    }

    auto& selections = m_skins[slot.Get()];
    const size_t erased = selections.erase(itemDefinition);
    if (!erased)
    {
        Reply(slot, "[KHSKIN] No saved selection for this weapon.\n");
        return;
    }

    Reply(slot, "[KHSKIN] Selection removed. Re-pick/re-give the weapon to restore its original econ state immediately.\n");
}

void KHookSkinChanger::CommandSkinInfo(CPlayerSlot slot)
{
    uint16_t itemDefinition = 0;
    CEntityInstance* weapon = GetActiveWeapon(slot, &itemDefinition);
    if (!weapon)
    {
        Reply(slot, "[KHSKIN] Active weapon was not found.\n");
        return;
    }

    char buffer[256];
    auto& selections = m_skins[slot.Get()];
    auto it = selections.find(itemDefinition);
    if (it == selections.end())
    {
        std::snprintf(buffer, sizeof(buffer), "[KHSKIN] Active weapon def=%u; no selection saved.\n",
                      static_cast<unsigned>(itemDefinition));
    }
    else
    {
        const SkinSelection& s = it->second;
        std::snprintf(buffer, sizeof(buffer),
                      "[KHSKIN] def=%u paint=%d seed=%d wear=%.6f stattrak=%d\n",
                      static_cast<unsigned>(itemDefinition), s.paintKit, s.seed, s.wear, s.statTrak);
    }
    Reply(slot, buffer);
}
