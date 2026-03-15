// CDPBase.h
// Base class for trainers targeting web-based desktop games (Tauri/WebView2, Electron, CEF, NW.js).
// Communicates via Chrome DevTools Protocol (CDP) over a persistent WebSocket connection.
// Derived classes implement game-specific cheat functions using executeJS().
#pragma once

#include <winsock2.h>
#include <Windows.h>
#include <winhttp.h>
#include <string>
#include <vector>
#include <chrono>
#include <iostream>
#include <shlobj.h>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

#pragma comment(lib, "winhttp.lib")

using json = nlohmann::json;

/// How the game process receives the remote debugging port argument.
enum class CDPLaunchMethod
{
    /// WebView2 / Tauri: set env var WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS
    WebView2EnvVar,
    /// Electron / CEF / NW.js: append --remote-debugging-port=PORT to command line
    CommandLineArg
};

class CDPBase
{
public:
    std::string gamePath;

    CDPBase(const std::string &settingsKey, CDPLaunchMethod method = CDPLaunchMethod::WebView2EnvVar)
        : settingsKey_(settingsKey), launchMethod_(method)
    {
        loadSettings();
    }

    virtual ~CDPBase()
    {
        wsDisconnect();
    }

    // ============================================================
    // FLTKUtils compatibility interface
    // ============================================================

    std::wstring getProcessName() const
    {
        return L"localhost:" + std::to_wstring(cdpPort_);
    }

    DWORD getProcessId() const
    {
        return lastCheckResult_ ? (DWORD)cdpPort_ : 0;
    }

    void cleanUp()
    {
        lastCheckResult_ = false;
    }

    /// Called periodically by FLTKUtils timer (~1s).
    bool isProcessRunning()
    {
        if (!gameLaunched_)
            return false;

        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastCheckTime_).count();

        if (elapsed < 3000)
            return lastCheckResult_;

        lastCheckTime_ = now;

        if (wsHandle_)
        {
            std::string pingResult;
            lastCheckResult_ = wsEvaluate(cachedWsPath_, "1", &pingResult);
        }
        else
        {
            // WebSocket is down — quick HTTP check first (fast fail if game is gone)
            if (!checkCDP())
            {
                lastCheckResult_ = false;
            }
            else
            {
                // Game is still alive — try to reconnect WebSocket
                if (cachedWsPath_.empty())
                    discoverPageWsPath(cachedWsPath_);

                if (!cachedWsPath_.empty())
                    lastCheckResult_ = wsReconnect(cachedWsPath_);
                else
                    lastCheckResult_ = true; // CDP alive, just can't get WS yet
            }
        }

        if (!lastCheckResult_)
        {
            cdpFailCount_++;
            if (cdpFailCount_ >= 3)
            {
                wsDisconnect();
                cachedWsPath_.clear();
                gameLaunched_ = false;
                cdpFailCount_ = 0;
            }
        }
        else
        {
            cdpFailCount_ = 0;
        }

        return lastCheckResult_;
    }

    // ============================================================
    // Game Launching
    // ============================================================

    bool launchGame()
    {
        if (gamePath.empty())
            return false;

        cdpPort_ = findFreePort(9222);

        std::string workDir = gamePath;
        size_t lastSlash = workDir.find_last_of("\\/");
        if (lastSlash != std::string::npos)
            workDir = workDir.substr(0, lastSlash);

        STARTUPINFOA si = {};
        PROCESS_INFORMATION pi = {};
        si.cb = sizeof(si);

        bool ok = false;

        if (launchMethod_ == CDPLaunchMethod::WebView2EnvVar)
        {
            std::string envVal = "--remote-debugging-port=" + std::to_string(cdpPort_);
            SetEnvironmentVariableA("WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS", envVal.c_str());

            ok = CreateProcessA(
                gamePath.c_str(), nullptr, nullptr, nullptr, FALSE,
                CREATE_NEW_PROCESS_GROUP, nullptr, workDir.c_str(), &si, &pi);
        }
        else // CDPLaunchMethod::CommandLineArg
        {
            std::string cmdLine = "\"" + gamePath + "\" --remote-debugging-port=" + std::to_string(cdpPort_);
            ok = CreateProcessA(
                nullptr, cmdLine.data(), nullptr, nullptr, FALSE,
                CREATE_NEW_PROCESS_GROUP, nullptr, workDir.c_str(), &si, &pi);
        }

        if (ok)
        {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            gameLaunched_ = true;
            saveSettings();
        }

        return ok;
    }

    // ============================================================
    // Core JS Execution via CDP
    // ============================================================

    /// Execute JavaScript in the game's web context via CDP.
    /// @param expression  JS code to evaluate (should return 'ok' on success)
    /// @param result      Optional: receives the return value as string
    /// @return true if JS executed and did NOT return a 'fail' string
    bool executeJS(const std::string &expression, std::string *result = nullptr)
    {
        std::string safeExpr = "try{" + expression + "}catch(_e){'fail: '+_e.message}";

        if (cachedWsPath_.empty())
        {
            if (!discoverPageWsPath(cachedWsPath_))
            {
                std::cerr << "[!] Failed to discover CDP page target.\n";
                return false;
            }
        }

        std::string jsResult;
        bool connected = wsEvaluate(cachedWsPath_, safeExpr, &jsResult);

        if (!connected)
        {
            wsDisconnect();
            cachedWsPath_.clear();
            if (discoverPageWsPath(cachedWsPath_))
                connected = wsEvaluate(cachedWsPath_, safeExpr, &jsResult);
            if (!connected)
                return false;
        }

        if (result)
            *result = jsResult;

        if (jsResult.size() >= 4 && jsResult.compare(0, 4, "fail") == 0)
            return false;

        return true;
    }

    // ============================================================
    // Settings Persistence
    // ============================================================

    void saveSettings()
    {
        std::string path = getSettingsFilePath();
        json settings;

        std::ifstream ifile(path);
        if (ifile.is_open())
        {
            try
            {
                ifile >> settings;
            }
            catch (...)
            {
            }
            ifile.close();
        }
        else
        {
            std::filesystem::create_directories(std::filesystem::path(path).parent_path());
        }

        settings["game_path"] = gamePath;

        std::ofstream ofile(path);
        ofile << settings.dump(4);
    }

    void loadSettings()
    {
        std::string path = getSettingsFilePath();
        std::ifstream file(path);
        if (!file.is_open())
            return;

        try
        {
            json settings;
            file >> settings;

            if (settings.contains("game_path"))
                gamePath = settings["game_path"].get<std::string>();
        }
        catch (...)
        {
        }
    }

private:
    std::string settingsKey_;
    CDPLaunchMethod launchMethod_;

    int cdpPort_ = 9222;
    bool gameLaunched_ = false;
    bool lastCheckResult_ = false;
    int cdpFailCount_ = 0;
    std::string cachedWsPath_;
    std::chrono::steady_clock::time_point lastCheckTime_;

    // Persistent WebSocket connection handles
    HINTERNET wsSession_ = nullptr;
    HINTERNET wsConnect_ = nullptr;
    HINTERNET wsHandle_ = nullptr;
    int nextMsgId_ = 1;

    std::string getSettingsFilePath()
    {
        PWSTR appdata = nullptr;
        SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &appdata);
        std::filesystem::path dir(appdata);
        CoTaskMemFree(appdata);
        return (dir / "GCM Settings" / (settingsKey_ + ".json")).string();
    }

    int findFreePort(int startPort)
    {
        for (int port = startPort; port < startPort + 100; ++port)
        {
            SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (sock == INVALID_SOCKET)
                continue;

            sockaddr_in addr = {};
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            addr.sin_port = htons((u_short)port);

            int result = bind(sock, (sockaddr *)&addr, sizeof(addr));
            closesocket(sock);

            if (result == 0)
                return port;
        }
        return startPort;
    }

    // ============================================================
    // CDP HTTP / WebSocket Implementation
    // ============================================================

    bool checkCDP()
    {
        HINTERNET hSession = WinHttpOpen(L"CDPTrainer", WINHTTP_ACCESS_TYPE_NO_PROXY, NULL, NULL, 0);
        if (!hSession)
            return false;

        DWORD connectTimeout = 200;
        DWORD ioTimeout = 300;
        WinHttpSetOption(hSession, WINHTTP_OPTION_CONNECT_TIMEOUT, &connectTimeout, sizeof(connectTimeout));
        WinHttpSetOption(hSession, WINHTTP_OPTION_RECEIVE_TIMEOUT, &ioTimeout, sizeof(ioTimeout));
        WinHttpSetOption(hSession, WINHTTP_OPTION_SEND_TIMEOUT, &ioTimeout, sizeof(ioTimeout));

        HINTERNET hConnect = WinHttpConnect(hSession, L"localhost", (INTERNET_PORT)cdpPort_, 0);
        if (!hConnect)
        {
            WinHttpCloseHandle(hSession);
            return false;
        }

        HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", L"/json/version",
                                                NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
        if (!hRequest)
        {
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return false;
        }

        bool ok = false;
        if (WinHttpSendRequest(hRequest, NULL, 0, NULL, 0, 0, 0) &&
            WinHttpReceiveResponse(hRequest, NULL))
        {
            DWORD statusCode = 0;
            DWORD sz = sizeof(statusCode);
            WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                                NULL, &statusCode, &sz, NULL);
            ok = (statusCode == 200);
        }

        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return ok;
    }

    std::string httpGet(const wchar_t *path)
    {
        HINTERNET hSession = WinHttpOpen(L"CDPTrainer", WINHTTP_ACCESS_TYPE_NO_PROXY, NULL, NULL, 0);
        if (!hSession)
            return "";

        DWORD timeout = 500;
        WinHttpSetOption(hSession, WINHTTP_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
        WinHttpSetOption(hSession, WINHTTP_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));
        WinHttpSetOption(hSession, WINHTTP_OPTION_SEND_TIMEOUT, &timeout, sizeof(timeout));

        HINTERNET hConnect = WinHttpConnect(hSession, L"localhost", (INTERNET_PORT)cdpPort_, 0);
        if (!hConnect)
        {
            WinHttpCloseHandle(hSession);
            return "";
        }

        HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path,
                                                NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
        if (!hRequest)
        {
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return "";
        }

        std::string body;
        if (WinHttpSendRequest(hRequest, NULL, 0, NULL, 0, 0, 0) &&
            WinHttpReceiveResponse(hRequest, NULL))
        {
            DWORD avail = 0;
            while (WinHttpQueryDataAvailable(hRequest, &avail) && avail > 0)
            {
                std::vector<char> buf(avail);
                DWORD bytesRead = 0;
                WinHttpReadData(hRequest, buf.data(), avail, &bytesRead);
                body.append(buf.data(), bytesRead);
            }
        }

        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return body;
    }

    bool discoverPageWsPath(std::string &outPath)
    {
        std::string body = httpGet(L"/json");
        if (body.empty())
            return false;

        json targets = json::parse(body, nullptr, false);
        if (targets.is_discarded() || !targets.is_array())
            return false;

        for (auto &target : targets)
        {
            if (target.value("type", "") == "page")
            {
                std::string wsUrl = target.value("webSocketDebuggerUrl", "");
                if (wsUrl.empty())
                    continue;

                size_t pathStart = wsUrl.find('/', wsUrl.find("//") + 2);
                if (pathStart != std::string::npos)
                {
                    outPath = wsUrl.substr(pathStart);
                    return true;
                }
            }
        }

        return false;
    }

    bool wsEvaluate(const std::string &wsPath, const std::string &expression, std::string *result)
    {
        if (!wsHandle_ && !wsReconnect(wsPath))
            return false;

        int msgId = nextMsgId_++;

        json msg = {
            {"id", msgId},
            {"method", "Runtime.evaluate"},
            {"params", {{"expression", expression}, {"returnByValue", true}, {"awaitPromise", false}}}};
        std::string msgStr = msg.dump();

        DWORD err = WinHttpWebSocketSend(wsHandle_,
                                         WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE,
                                         (PVOID)msgStr.c_str(), (DWORD)msgStr.size());

        if (err != ERROR_SUCCESS)
        {
            wsDisconnect();
            return false;
        }

        for (int attempts = 0; attempts < 50; attempts++)
        {
            std::string response = wsReadMessage(wsHandle_);
            if (response.empty())
            {
                wsDisconnect();
                return false;
            }

            json resp = json::parse(response, nullptr, false);
            if (resp.is_discarded())
                continue;

            if (resp.contains("id") && resp["id"].get<int>() == msgId)
            {
                if (result && resp.contains("result") && resp["result"].contains("result"))
                {
                    auto &val = resp["result"]["result"];
                    if (val.contains("value"))
                    {
                        if (val["value"].is_string())
                            *result = val["value"].get<std::string>();
                        else
                            *result = val["value"].dump();
                    }
                }
                return true;
            }
        }

        wsDisconnect();
        return false;
    }

    bool wsReconnect(const std::string &wsPath)
    {
        wsDisconnect();

        wsSession_ = WinHttpOpen(L"CDPTrainer", WINHTTP_ACCESS_TYPE_NO_PROXY, NULL, NULL, 0);
        if (!wsSession_)
            return false;

        DWORD connectTimeout = 500;
        DWORD sendRecvTimeout = 3000;
        WinHttpSetOption(wsSession_, WINHTTP_OPTION_CONNECT_TIMEOUT, &connectTimeout, sizeof(connectTimeout));
        WinHttpSetOption(wsSession_, WINHTTP_OPTION_RECEIVE_TIMEOUT, &sendRecvTimeout, sizeof(sendRecvTimeout));
        WinHttpSetOption(wsSession_, WINHTTP_OPTION_SEND_TIMEOUT, &sendRecvTimeout, sizeof(sendRecvTimeout));

        wsConnect_ = WinHttpConnect(wsSession_, L"localhost", (INTERNET_PORT)cdpPort_, 0);
        if (!wsConnect_)
        {
            wsDisconnect();
            return false;
        }

        std::wstring wPath(wsPath.begin(), wsPath.end());
        HINTERNET hRequest = WinHttpOpenRequest(wsConnect_, L"GET", wPath.c_str(),
                                                NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
        if (!hRequest)
        {
            wsDisconnect();
            return false;
        }

        WinHttpSetOption(hRequest, WINHTTP_OPTION_UPGRADE_TO_WEB_SOCKET, NULL, 0);

        if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, NULL, 0, 0, 0) ||
            !WinHttpReceiveResponse(hRequest, NULL))
        {
            WinHttpCloseHandle(hRequest);
            wsDisconnect();
            return false;
        }

        wsHandle_ = WinHttpWebSocketCompleteUpgrade(hRequest, 0);
        WinHttpCloseHandle(hRequest);

        if (!wsHandle_)
        {
            wsDisconnect();
            return false;
        }

        return true;
    }

    void wsDisconnect()
    {
        if (wsHandle_)
        {
            WinHttpCloseHandle(wsHandle_);
            wsHandle_ = nullptr;
        }
        if (wsConnect_)
        {
            WinHttpCloseHandle(wsConnect_);
            wsConnect_ = nullptr;
        }
        if (wsSession_)
        {
            WinHttpCloseHandle(wsSession_);
            wsSession_ = nullptr;
        }
    }

    std::string wsReadMessage(HINTERNET hWs)
    {
        std::string message;
        char buffer[8192];
        DWORD bytesRead = 0;
        WINHTTP_WEB_SOCKET_BUFFER_TYPE bufferType;

        do
        {
            DWORD err = WinHttpWebSocketReceive(hWs, buffer, sizeof(buffer), &bytesRead, &bufferType);
            if (err != ERROR_SUCCESS)
                return "";

            if (bufferType == WINHTTP_WEB_SOCKET_CLOSE_BUFFER_TYPE)
                return "";

            message.append(buffer, bytesRead);
        } while (bufferType == WINHTTP_WEB_SOCKET_UTF8_FRAGMENT_BUFFER_TYPE ||
                 bufferType == WINHTTP_WEB_SOCKET_BINARY_FRAGMENT_BUFFER_TYPE);

        return message;
    }
};
