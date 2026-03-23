#pragma once
#include <ToolboxModule.h>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <winsock2.h>

class MapTrackerModule : public ToolboxModule {
    MapTrackerModule() = default;
    ~MapTrackerModule() override = default;
public:
    static MapTrackerModule& Instance() {
        static MapTrackerModule instance;
        return instance;
    }

    [[nodiscard]] const char* Name() const override { return "Map Tracker"; }
    [[nodiscard]] const char* Description() const override { return "Sends player position to a web map viewer via WebSocket relay"; }
    [[nodiscard]] const char* Icon() const override { return ICON_FA_MAP; }

    void Initialize() override;
    void Terminate() override;
    void Update(float delta) override;
    void LoadSettings(ToolboxIni* ini) override;
    void SaveSettings(ToolboxIni* ini) override;
    void DrawSettingsInternal() override;

    bool IsTracking() const { return tracking; }
    void SetTracking(bool val) { tracking = val; }

private:
    void ConnectLoop();
    void SendPosition(float x, float y, uint32_t map_id);
    bool DoWSHandshake();
    bool SendRaw(const void* data, int len);
    std::vector<uint8_t> MakeWSFrame(const std::string& json);

    char relay_host[256] = "your-relay.onrender.com";
    char session_key[64] = "my-session";
    float send_interval = 0.1f;
    float elapsed = 0.f;

    std::string relay_host_str = "your-relay.onrender.com";
    std::string session_key_str = "my-session";
    unsigned int relay_port = 443;

    SOCKET sock = INVALID_SOCKET;
    std::atomic<bool> connected{ false };
    std::atomic<bool> running{ false };
    std::atomic<bool> tracking{ false };
    std::thread ws_thread;
    std::mutex sock_mutex;
};
