#include "stdafx.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include "MapTrackerModule.h"
#include <GWCA/Managers/AgentMgr.h>
#include <GWCA/Managers/MapMgr.h>
#include <GWCA/GameEntities/Agent.h>
#include <imgui.h>
#include <Defines.h>

void MapTrackerModule::Initialize() {
    ToolboxModule::Initialize();
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
}

void MapTrackerModule::Terminate() {
    running = false;
    tracking = false;
    if (ws_thread.joinable()) ws_thread.join();
    std::lock_guard<std::mutex> lock(sock_mutex);
    if (sock != INVALID_SOCKET) {
        closesocket(sock);
        sock = INVALID_SOCKET;
    }
    WSACleanup();
    ToolboxModule::Terminate();
}

void MapTrackerModule::Update(float delta) {
    if (!tracking || !GW::Map::GetIsMapLoaded()) return;
    elapsed += delta;
    if (elapsed < send_interval) return;
    elapsed = 0.f;
    const auto* agent = GW::Agents::GetControlledCharacter();
    if (!agent) return;
    const uint32_t map_id = static_cast<uint32_t>(GW::Map::GetMapID());
    SendPosition(agent->pos.x, agent->pos.y, map_id);
}

void MapTrackerModule::SendPosition(float x, float y, uint32_t map_id) {
    if (!connected) {
        if (!running) {
            running = true;
            if (ws_thread.joinable()) ws_thread.join();
            ws_thread = std::thread(&MapTrackerModule::ConnectLoop, this);
        }
        return;
    }
    char buf[256];
    snprintf(buf, sizeof(buf),
        "{\"type\":\"position\",\"x\":%.2f,\"y\":%.2f,\"map\":%u}",
        x, y, map_id);
    auto frame = MakeWSFrame(buf);
    std::lock_guard<std::mutex> lock(sock_mutex);
    SendRaw(frame.data(), (int)frame.size());
}

void MapTrackerModule::ConnectLoop() {
    while (running && tracking) {
        {
            std::lock_guard<std::mutex> lock(sock_mutex);
            sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (sock == INVALID_SOCKET) { Sleep(3000); continue; }
            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_port = htons((u_short)relay_port);
            struct addrinfo* res = nullptr;
            if (getaddrinfo(relay_host, nullptr, nullptr, &res) == 0 && res) {
                addr.sin_addr = ((sockaddr_in*)res->ai_addr)->sin_addr;
                freeaddrinfo(res);
            }
            if (connect(sock, (sockaddr*)&addr, sizeof(addr)) != 0) {
                closesocket(sock); sock = INVALID_SOCKET;
                Sleep(3000); continue;
            }
        }
        if (!DoWSHandshake()) {
            std::lock_guard<std::mutex> lock(sock_mutex);
            closesocket(sock); sock = INVALID_SOCKET;
            Sleep(3000); continue;
        }
        connected = true;
        while (running && tracking) {
            fd_set fds; FD_ZERO(&fds); FD_SET(sock, &fds);
            timeval tv{ 1, 0 };
            int r = select(0, &fds, nullptr, nullptr, &tv);
            if (r > 0) { char tmp[64]; if (recv(sock, tmp, sizeof(tmp), 0) <= 0) break; }
            else if (r < 0) break;
        }
        connected = false;
        std::lock_guard<std::mutex> lock(sock_mutex);
        closesocket(sock); sock = INVALID_SOCKET;
        Sleep(1000);
    }
    connected = false;
    running = false;
}

bool MapTrackerModule::DoWSHandshake() {
    char req[512];
    snprintf(req, sizeof(req),
        "GET /?key=%s&role=plugin HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
        "Sec-WebSocket-Version: 13\r\n\r\n",
        session_key, relay_host);
    if (send(sock, req, (int)strlen(req), 0) <= 0) return false;
    char resp[1024] = {};
    recv(sock, resp, sizeof(resp) - 1, 0);
    return strstr(resp, "101") != nullptr;
}

bool MapTrackerModule::SendRaw(const void* data, int len) {
    return send(sock, (const char*)data, len, 0) > 0;
}

std::vector<uint8_t> MapTrackerModule::MakeWSFrame(const std::string& json) {
    std::vector<uint8_t> frame;
    frame.push_back(0x81);
    size_t len = json.size();
    if (len < 126) frame.push_back((uint8_t)(0x80 | len));
    else { frame.push_back(0x80 | 126); frame.push_back((len >> 8) & 0xFF); frame.push_back(len & 0xFF); }
    uint8_t mask[4] = { 0x12, 0x34, 0x56, 0x78 };
    for (auto b : mask) frame.push_back(b);
    for (size_t i = 0; i < len; i++) frame.push_back(json[i] ^ mask[i % 4]);
    return frame;
}

void MapTrackerModule::LoadSettings(ToolboxIni* ini) {
    ToolboxModule::LoadSettings(ini);
    bool t = tracking;
    LOAD_BOOL(t);
    tracking = t;
    LOAD_STRING(relay_host_str);
    LOAD_STRING(session_key_str);
    strncpy_s(relay_host, relay_host_str.c_str(), sizeof(relay_host) - 1);
    strncpy_s(session_key, session_key_str.c_str(), sizeof(session_key) - 1);
    LOAD_UINT(relay_port);
    LOAD_FLOAT(send_interval);
}

void MapTrackerModule::SaveSettings(ToolboxIni* ini) {
    ToolboxModule::SaveSettings(ini);
    bool t = tracking;
    SAVE_BOOL(t);
    relay_host_str = relay_host;
    session_key_str = session_key;
    SAVE_STRING(relay_host_str);
    SAVE_STRING(session_key_str);
    SAVE_UINT(relay_port);
    SAVE_FLOAT(send_interval);
}

void MapTrackerModule::DrawSettingsInternal() {
    bool t = tracking;
    if (ImGui::Checkbox("Enable Map Tracking", &t)) tracking = t;
    ImGui::InputText("Relay Host", relay_host, sizeof(relay_host));
    ImGui::InputText("Session Key", session_key, sizeof(session_key));
    int port = (int)relay_port;
    if (ImGui::InputInt("Port", &port)) relay_port = (unsigned int)port;
    ImGui::SliderFloat("Send Interval (s)", &send_interval, 0.05f, 1.0f);
    if (connected) ImGui::TextColored({0,1,0,1}, "Connected");
    else ImGui::TextColored({1,0,0,1}, "Disconnected");
}
