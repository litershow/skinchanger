#include "plugin.h"
#include "platform_offsets.h"
#include "skin_catalog.h"

#include <iserver.h>
#include <tier1/convar.h>
#include <engine/igameeventsystem.h>
#include <networksystem/inetworkmessages.h>
#include <irecipientfilter.h>
#include <in_buttons.h>
#include "usermessages.pb.h"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

namespace
{
IServerGameDLL* g_server = nullptr;
IServerGameClients* g_gameClients = nullptr;
ICvar* g_cvar = nullptr;
ISchemaSystem* g_schemaSystem = nullptr;
IGameEventSystem* g_gameEventSystem = nullptr;
INetworkMessages* g_networkMessages = nullptr;
constexpr int kHudPrintCenter = 4;

class SingleRecipientFilter final : public IRecipientFilter
{
public:
    explicit SingleRecipientFilter(CPlayerSlot slot)
    {
        if (slot.IsValid() && slot.Get() >= 0 && slot.Get() < ABSOLUTE_PLAYER_LIMIT)
            m_recipients.Set(slot.Get());
    }

    NetChannelBufType_t GetNetworkBufType() const override { return BUF_RELIABLE; }
    bool IsInitMessage() const override { return false; }
    const CPlayerBitVec& GetRecipients() const override { return m_recipients; }
    CPlayerSlot GetPredictedPlayerSlot() const override { return -1; }

private:
    CPlayerBitVec m_recipients;
};

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

void PrintMenuDiagnostics(CPlayerSlot slot)
{
    INetworkMessageInternal* textMsg = g_networkMessages
        ? g_networkMessages->FindNetworkMessagePartial("TextMsg")
        : nullptr;

    char line[512];
    std::snprintf(line, sizeof(line),
                  "[KHSKIN] menu backend: renderer=TextMsg, GameEventSystem=%s, NetworkMessages=%s, TextMsg=%s\n",
                  g_gameEventSystem ? "OK" : "MISSING",
                  g_networkMessages ? "OK" : "MISSING",
                  textMsg ? "OK" : "MISSING");
    Reply(slot, line);
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

CON_COMMAND_F(kh_skin_menu, "Open KHook SkinChanger center menu",
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

CON_COMMAND_F(kh_skin_diag, "Show KHook SkinChanger built-in menu backend status",
              FCVAR_CLIENT_CAN_EXECUTE | FCVAR_GAMEDLL)
{
    PrintMenuDiagnostics(context.GetPlayerSlot());
}

CON_COMMAND_F(kh_skin_hudtest, "Send a safe TextMsg center-HUD test",
              FCVAR_CLIENT_CAN_EXECUTE | FCVAR_GAMEDLL)
{
    const CPlayerSlot slot = context.GetPlayerSlot();
    if (!slot.IsValid())
    {
        META_CONPRINTF("[KHSKIN] kh_skin_hudtest is a player command.\n");
        return;
    }
    if (!g_KHookSkinChanger.SendCenterTextForTest(slot, "KHook SkinChanger HUD OK"))
        Reply(slot, "[KHSKIN] TextMsg HUD test failed. Run kh_skin_diag.\n");
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

    // Menu renderer uses only the same public engine interfaces and TextMsg path
    // used by current native CS2 Metamod plugins (for example CS2Fixes).
    // No guessed server vtable indices and no legacy game-event serialization.
    CreateInterfaceFn engineFactory = ismm->GetEngineFactory();
    g_gameEventSystem = engineFactory
        ? static_cast<IGameEventSystem*>(engineFactory(GAMEEVENTSYSTEM_INTERFACE_VERSION, nullptr))
        : nullptr;
    g_networkMessages = engineFactory
        ? static_cast<INetworkMessages*>(engineFactory(NETWORKMESSAGES_INTERFACE_VERSION, nullptr))
        : nullptr;

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

    META_CONPRINTF("[KHSKIN] Loaded %s %s using KHook.\n", PLUGIN_DISPLAY_NAME, PLUGIN_FULL_VERSION);
    META_CONPRINTF("[KHSKIN] Built-in safe center menu enabled (TextMsg; no CS2Menus dependency).\n");
    META_CONPRINTF("[KHSKIN] Menu backend: GameEventSystem=%s, NetworkMessages=%s\n",
                   g_gameEventSystem ? "OK" : "MISSING",
                   g_networkMessages ? "OK" : "MISSING");
    META_CONPRINTF("[KHSKIN] Chat: !skin | console: kh_skin_menu, kh_skin_diag, kh_skin_hudtest, kh_skin, kh_skin_clear, kh_skin_info\n");
    return true;
}

bool KHookSkinChanger::Unload(char* error, size_t maxlen)
{
    (void)error;
    (void)maxlen;

    if (g_server)
        m_gameFrameHook.Remove(g_server);
    if (g_gameClients)
        m_clientDisconnectHook.Remove(g_gameClients);
    if (g_cvar)
        m_dispatchConCommandHook.Remove(g_cvar);

    for (int i = 0; i < kMaxPlayers; ++i)
    {
        m_skins[i].clear();
        m_menuStates[i] = {};
    }

    m_entitySystem = nullptr;
    g_gameEventSystem = nullptr;
    g_networkMessages = nullptr;
    return true;
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
    for (auto& state : m_menuStates)
        state = {};
    META_CONPRINTF("[KHSKIN] Level init: %s\n", mapName ? mapName : "<unknown>");
}

void KHookSkinChanger::OnLevelShutdown()
{
    m_entitySystem = nullptr;
    m_frameCounter = 0;
    for (auto& state : m_menuStates)
        state = {};
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

    if (m_entitySystem)
        TickMenus();

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

void KHookSkinChanger::OpenSkinMenu(CPlayerSlot slot)
{
    if (!slot.IsValid() || slot.Get() < 0 || slot.Get() >= kMaxPlayers)
        return;

    NativeMenuState& state = m_menuStates[slot.Get()];
    state = {};
    state.page = NativeMenuPage::Groups;
    state.lastButtons = ReadButtons(slot);
    state.refreshFrames = 0;

    if (!SendCenterText(slot, BuildMenuText(slot)))
    {
        state = {};
        Reply(slot, "[KHSKIN] Could not send the built-in center menu.\n");
        PrintMenuDiagnostics(slot);
        Reply(slot, "[KHSKIN] Run kh_skin_diag in console and send the output if any backend is MISSING.\n");
        return;
    }

    Reply(slot, "[KHSKIN] Menu: W/S = browse, E = select, R = back/close.\n");
}

void KHookSkinChanger::CommandOpenSkinMenu(CPlayerSlot slot)
{
    OpenSkinMenu(slot);
}

void KHookSkinChanger::CloseSkinMenu(CPlayerSlot slot, bool clearHud)
{
    if (!slot.IsValid() || slot.Get() < 0 || slot.Get() >= kMaxPlayers)
        return;

    m_menuStates[slot.Get()] = {};
    // Do not send a blank TextMsg here. CS2 keeps the center-panel background
    // for a blank message, which looks like an empty brown rectangle.
    // The previous center text expires naturally.
    (void)clearHud;
}

std::size_t KHookSkinChanger::MenuItemCount(const NativeMenuState& state) const
{
    const auto& catalog = GetSkinCatalog();
    switch (state.page)
    {
        case NativeMenuPage::Groups:
            return catalog.size();
        case NativeMenuPage::Weapons:
            return state.group < catalog.size() ? catalog[state.group].weapons.size() : 0;
        case NativeMenuPage::Skins:
            if (state.group < catalog.size() && state.weapon < catalog[state.group].weapons.size())
                return catalog[state.group].weapons[state.weapon].skins.size();
            return 0;
        default:
            return 0;
    }
}

void KHookSkinChanger::MenuMove(CPlayerSlot slot, int delta)
{
    NativeMenuState& state = m_menuStates[slot.Get()];
    const std::size_t count = MenuItemCount(state);
    if (count == 0)
        return;

    if (delta < 0)
        state.cursor = (state.cursor + count - 1) % count;
    else if (delta > 0)
        state.cursor = (state.cursor + 1) % count;
    state.refreshFrames = 0;
}

void KHookSkinChanger::MenuSelect(CPlayerSlot slot)
{
    NativeMenuState& state = m_menuStates[slot.Get()];
    const auto& catalog = GetSkinCatalog();

    if (state.page == NativeMenuPage::Groups)
    {
        if (state.cursor >= catalog.size())
            return;
        state.group = state.cursor;
        state.page = NativeMenuPage::Weapons;
        state.cursor = 0;
        state.refreshFrames = 0;
        return;
    }

    if (state.page == NativeMenuPage::Weapons)
    {
        if (state.group >= catalog.size() || state.cursor >= catalog[state.group].weapons.size())
            return;
        state.weapon = state.cursor;
        state.page = NativeMenuPage::Skins;
        state.cursor = 0;
        state.refreshFrames = 0;
        return;
    }

    if (state.page != NativeMenuPage::Skins || state.group >= catalog.size() ||
        state.weapon >= catalog[state.group].weapons.size())
        return;

    const CatalogWeapon& weapon = catalog[state.group].weapons[state.weapon];
    if (state.cursor >= weapon.skins.size())
        return;

    const CatalogSkin& skin = weapon.skins[state.cursor];
    SkinSelection selection;
    selection.paintKit = skin.paintKit;
    selection.seed = skin.seed;
    selection.wear = std::clamp(skin.wear, 0.000001f, 1.0f);
    selection.statTrak = skin.statTrak;

    if (!SetSkinForDefinition(slot, weapon.itemDefinition, selection))
    {
        Reply(slot, "[KHSKIN] Could not save/apply this skin. Check server console for schema errors.\n");
        return;
    }

    char message[256];
    std::snprintf(message, sizeof(message), "[KHSKIN] Selected %s for %s (paint kit %d).\n",
                  skin.name, weapon.name, selection.paintKit);
    Reply(slot, message);
    CloseSkinMenu(slot, false);

    char hudMessage[256];
    std::snprintf(hudMessage, sizeof(hudMessage), "Skin applied\n%s -> %s", skin.name, weapon.name);
    SendCenterText(slot, hudMessage);
}

void KHookSkinChanger::MenuBack(CPlayerSlot slot)
{
    NativeMenuState& state = m_menuStates[slot.Get()];
    if (state.page == NativeMenuPage::Skins)
    {
        state.page = NativeMenuPage::Weapons;
        state.cursor = state.weapon;
        state.refreshFrames = 0;
    }
    else if (state.page == NativeMenuPage::Weapons)
    {
        state.page = NativeMenuPage::Groups;
        state.cursor = state.group;
        state.refreshFrames = 0;
    }
    else
    {
        CloseSkinMenu(slot, false);
        SendCenterText(slot, "Skin menu closed");
    }
}

std::uint64_t KHookSkinChanger::ReadButtons(CPlayerSlot slot)
{
    CEntityInstance* pawn = GetPawn(slot);
    if (!pawn)
        return 0;

    const std::ptrdiff_t movementServicesOffset = m_schema.FindOffset(
        pawn->Schema_DynamicBinding().Get(), "m_pMovementServices");
    if (movementServicesOffset < 0)
    {
        static bool warnedMovement = false;
        if (!warnedMovement)
        {
            META_CONPRINTF("[KHSKIN] Menu input unavailable: schema field m_pMovementServices was not found.\n");
            warnedMovement = true;
        }
        return 0;
    }

    void** movementServices = FieldPtr<void*>(pawn, movementServicesOffset);
    if (!movementServices || !*movementServices)
        return 0;

    const std::ptrdiff_t buttonsOffset = m_schema.FindOffset("CPlayer_MovementServices", "m_nButtons");
    const std::ptrdiff_t statesOffset = m_schema.FindOffset("CInButtonState", "m_pButtonStates");
    if (buttonsOffset < 0 || statesOffset < 0)
    {
        static bool warnedButtons = false;
        if (!warnedButtons)
        {
            META_CONPRINTF("[KHSKIN] Menu input unavailable: m_nButtons=%td m_pButtonStates=%td.\n",
                           buttonsOffset, statesOffset);
            warnedButtons = true;
        }
        return 0;
    }

    std::uint64_t* states = FieldPtr<std::uint64_t>(
        *movementServices, buttonsOffset + statesOffset);
    return states ? states[0] : 0;
}

void KHookSkinChanger::TickMenus()
{
    for (int i = 0; i < kMaxPlayers; ++i)
    {
        if (m_menuStates[i].page != NativeMenuPage::Closed)
            TickMenu(CPlayerSlot(i));
    }
}

void KHookSkinChanger::TickMenu(CPlayerSlot slot)
{
    NativeMenuState& state = m_menuStates[slot.Get()];
    if (state.page == NativeMenuPage::Closed)
        return;

    if (!GetPawn(slot))
    {
        CloseSkinMenu(slot, false);
        return;
    }

    const std::uint64_t buttons = ReadButtons(slot);
    const std::uint64_t pressed = buttons & ~state.lastButtons;
    state.lastButtons = buttons;

    bool acted = false;
    if (pressed & static_cast<std::uint64_t>(IN_FORWARD))
    {
        MenuMove(slot, -1);
        acted = true;
    }
    else if (pressed & static_cast<std::uint64_t>(IN_BACK))
    {
        MenuMove(slot, 1);
        acted = true;
    }
    else if (pressed & static_cast<std::uint64_t>(IN_USE))
    {
        // E/Use is the only select key. Using D here made normal strafing
        // accidentally enter menu items while the player was still moving.
        MenuSelect(slot);
        acted = true;
    }
    else if (pressed & static_cast<std::uint64_t>(IN_RELOAD))
    {
        // R/Reload is back/close. A is intentionally not used because normal
        // left strafing used to close the menu unexpectedly.
        MenuBack(slot);
        acted = true;
    }

    if (state.page == NativeMenuPage::Closed)
        return;

    if (acted || --state.refreshFrames <= 0)
    {
        state.refreshFrames = kMenuRefreshEveryNFrames;
        SendCenterText(slot, BuildMenuText(slot));
    }
}

std::string KHookSkinChanger::BuildMenuText(CPlayerSlot slot) const
{
    if (!slot.IsValid() || slot.Get() < 0 || slot.Get() >= kMaxPlayers)
        return {};

    const NativeMenuState& state = m_menuStates[slot.Get()];
    const auto& catalog = GetSkinCatalog();
    if (state.page == NativeMenuPage::Closed)
        return {};

    // PrintToCenter/TextMsg has a fixed-height panel in CS2 and shrinks the
    // font aggressively as lines are added. Never render the whole list.
    // Show one large current choice plus position and controls instead.
    std::string title;
    std::string selected;
    std::string action = "E select";
    std::size_t count = 0;
    std::size_t cursor = state.cursor;

    if (state.page == NativeMenuPage::Groups)
    {
        title = "Weapon type";
        count = catalog.size();
        if (count != 0)
        {
            cursor = std::min(cursor, count - 1);
            selected = catalog[cursor].name;
        }
        action = "E open";
    }
    else if (state.page == NativeMenuPage::Weapons && state.group < catalog.size())
    {
        const CatalogGroup& group = catalog[state.group];
        title = group.name;
        count = group.weapons.size();
        if (count != 0)
        {
            cursor = std::min(cursor, count - 1);
            selected = group.weapons[cursor].name;
        }
        action = "E skins";
    }
    else if (state.page == NativeMenuPage::Skins && state.group < catalog.size() &&
             state.weapon < catalog[state.group].weapons.size())
    {
        const CatalogWeapon& weapon = catalog[state.group].weapons[state.weapon];
        title = weapon.name;
        count = weapon.skins.size();
        if (count != 0)
        {
            cursor = std::min(cursor, count - 1);
            selected = weapon.skins[cursor].name;
        }
        action = "E APPLY";
    }

    if (count == 0 || selected.empty())
        return "KHook SkinChanger\nNo menu items\nR back";

    char page[64];
    std::snprintf(page, sizeof(page), "%zu/%zu", cursor + 1, count);

    // Keep this at three lines. On the user's 16:9 HUD, seven-line center text
    // was reduced to an unreadably small font by the client.
    std::string text;
    text.reserve(192);
    text += title;
    text += "  [";
    text += page;
    text += "]\n>>> ";
    text += selected;
    text += " <<<\nW/S browse   ";
    text += action;
    text += "   R back";
    return text;
}

bool KHookSkinChanger::SendCenterText(CPlayerSlot slot, const std::string& text)
{
    if (!slot.IsValid() || slot.Get() < 0 || slot.Get() >= kMaxPlayers)
        return false;
    if (!g_gameEventSystem || !g_networkMessages)
        return false;

    INetworkMessageInternal* netMessage = g_networkMessages->FindNetworkMessagePartial("TextMsg");
    if (!netMessage)
    {
        META_CONPRINTF("[KHSKIN] Center menu: TextMsg network message was not found.\n");
        return false;
    }

    CNetMessage* raw = netMessage->AllocateMessage();
    if (!raw)
    {
        META_CONPRINTF("[KHSKIN] Center menu: TextMsg AllocateMessage failed.\n");
        return false;
    }

    auto* data = raw->ToPB<CUserMessageTextMsg>();
    if (!data)
    {
        META_CONPRINTF("[KHSKIN] Center menu: TextMsg protobuf cast failed.\n");
        delete raw;
        return false;
    }

    data->set_dest(kHudPrintCenter);
    data->add_param(text);

    SingleRecipientFilter filter(slot);
    g_gameEventSystem->PostEventAbstract(-1, false, &filter, netMessage, data, 0);

    // CNetMessage/ToPB ownership is the same allocation; current native CS2 plugins
    // delete the protobuf message after PostEventAbstract.
    delete data;
    return true;
}

bool KHookSkinChanger::SendCenterTextForTest(CPlayerSlot slot, const std::string& text)
{
    return SendCenterText(slot, text);
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
    m_menuStates[slot.Get()] = {};
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
