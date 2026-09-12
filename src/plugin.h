#pragma once

#include <ISmmPlugin.h>
#include <entity2/entityinstance.h>
#include <entity2/entitysystem.h>
#include <schemasystem/schemasystem.h>
#include <icvar.h>

#include "version_gen.h"
#include "schema_resolver.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>

struct SkinSelection
{
    int paintKit = 0;
    int seed = 0;
    float wear = 0.0001f;
    int statTrak = -1;
};

enum class NativeMenuPage : std::uint8_t
{
    Closed = 0,
    Groups,
    Weapons,
    Skins,
};

struct NativeMenuState
{
    NativeMenuPage page = NativeMenuPage::Closed;
    std::size_t group = 0;
    std::size_t weapon = 0;
    std::size_t cursor = 0;
    std::uint64_t lastButtons = 0;
    int refreshFrames = 0;
};

class KHookSkinChanger final : public ISmmPlugin, public IMetamodListener
{
public:
    KHookSkinChanger();

    bool Load(PluginId id, ISmmAPI* ismm, char* error, size_t maxlen, bool late) override;
    bool Unload(char* error, size_t maxlen) override;

    void OnLevelInit(const char* mapName,
                     const char* mapEntities,
                     const char* oldLevel,
                     const char* landmarkName,
                     bool loadGame,
                     bool background) override;
    void OnLevelShutdown() override;

    KHook::Return<void> Hook_GameFrame(IServerGameDLL*, bool simulating, bool firstTick, bool lastTick);
    KHook::Return<void> Hook_ClientDisconnect(IServerGameClients*, CPlayerSlot slot,
                                              ENetworkDisconnectionReason reason,
                                              const char* name, uint64 xuid,
                                              const char* networkId);
    KHook::Return<void> Hook_DispatchConCommand(ICvar*, ConCommandRef command,
                                                const CCommandContext& context,
                                                const CCommand& args);

    void CommandSetSkin(CPlayerSlot slot, const CCommand& args);
    void CommandClearSkin(CPlayerSlot slot);
    void CommandSkinInfo(CPlayerSlot slot);
    void CommandOpenSkinMenu(CPlayerSlot slot);
    bool SendCenterTextForTest(CPlayerSlot slot, const std::string& text);

    const char* GetAuthor() override { return PLUGIN_AUTHOR; }
    const char* GetName() override { return PLUGIN_DISPLAY_NAME; }
    const char* GetDescription() override { return PLUGIN_DESCRIPTION; }
    const char* GetURL() override { return PLUGIN_URL; }
    const char* GetLicense() override { return PLUGIN_LICENSE; }
    const char* GetVersion() override { return PLUGIN_FULL_VERSION; }
    const char* GetDate() override { return __DATE__; }
    const char* GetLogTag() override { return PLUGIN_LOGTAG; }

private:
    static constexpr int kMaxPlayers = 64;
    static constexpr int kApplyEveryNFrames = 8;
    static constexpr int kMenuRefreshEveryNFrames = 32;

    void RefreshEntitySystem();
    void TickPlayers();
    CEntityIdentity* ResolveIdentity(CEntityIndex index) const;
    CEntityInstance* ResolveEntity(CEntityIndex index) const;
    CEntityInstance* ResolveEntity(const CEntityHandle& handle) const;
    CEntityInstance* GetController(CPlayerSlot slot) const;
    CEntityInstance* GetPawn(CPlayerSlot slot);
    CEntityInstance* GetActiveWeapon(CPlayerSlot slot, uint16_t* itemDefinition = nullptr);
    bool ReadItemDefinition(CEntityInstance* weapon, uint16_t& itemDefinition);
    bool ApplySelection(CEntityInstance* weapon, const SkinSelection& selection);
    bool ApplyForSlot(CPlayerSlot slot);
    bool SetSkinForDefinition(CPlayerSlot slot, uint16_t itemDefinition, const SkinSelection& selection);
    void ClearSlot(CPlayerSlot slot);

    // Built-in center-HUD menu. No external CS2Menus plugin is required.
    void OpenSkinMenu(CPlayerSlot slot);
    void CloseSkinMenu(CPlayerSlot slot, bool clearHud = true);
    void TickMenus();
    void TickMenu(CPlayerSlot slot);
    void MenuMove(CPlayerSlot slot, int delta);
    void MenuSelect(CPlayerSlot slot);
    void MenuBack(CPlayerSlot slot);
    std::size_t MenuItemCount(const NativeMenuState& state) const;
    std::uint64_t ReadButtons(CPlayerSlot slot);
    bool SendCenterText(CPlayerSlot slot, const std::string& text);
    std::string BuildMenuText(CPlayerSlot slot) const;

    template <typename T>
    static T* FieldPtr(void* base, std::ptrdiff_t offset)
    {
        if (!base || offset < 0)
            return nullptr;
        return reinterpret_cast<T*>(reinterpret_cast<std::uintptr_t>(base) + static_cast<std::uintptr_t>(offset));
    }

    KHook::Virtual<IServerGameDLL, void, bool, bool, bool> m_gameFrameHook;
    KHook::Virtual<IServerGameClients, void, CPlayerSlot, ENetworkDisconnectionReason,
                   const char*, uint64, const char*> m_clientDisconnectHook;
    KHook::Virtual<ICvar, void, ConCommandRef, const CCommandContext&, const CCommand&> m_dispatchConCommandHook;

    SchemaResolver m_schema;
    CGameEntitySystem* m_entitySystem = nullptr;
    IGameResourceService* m_gameResourceService = nullptr;
    int m_frameCounter = 0;
    std::array<std::unordered_map<uint16_t, SkinSelection>, kMaxPlayers> m_skins;
    std::array<NativeMenuState, kMaxPlayers> m_menuStates;
};

extern KHookSkinChanger g_KHookSkinChanger;

PLUGIN_GLOBALVARS();
