#include "SecurityLogger.h"
#include <windows.h>
#include <shlobj.h>
#include <fstream>
#include <chrono>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <ctime>

namespace AnshuBio {

SecurityLogger& SecurityLogger::Instance() {
    static SecurityLogger s_instance;
    return s_instance;
}

SecurityLogger::SecurityLogger() {
    EnsureDirectoryExists();
}

SecurityLogger::~SecurityLogger() {}

std::string SecurityLogger::GetLogFilePath() {
    wchar_t localAppData[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, localAppData))) {
        char pathA[MAX_PATH];
        WideCharToMultiByte(CP_UTF8, 0, localAppData, -1, pathA, MAX_PATH, nullptr, nullptr);
        return std::string(pathA) + "\\AnshuBio\\logs\\security.log";
    }
    return "security.log";
}

void SecurityLogger::EnsureDirectoryExists() {
    wchar_t localAppData[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, localAppData))) {
        std::wstring baseDir = std::wstring(localAppData) + L"\\AnshuBio";
        std::wstring logsDir = baseDir + L"\\logs";
        CreateDirectoryW(baseDir.c_str(), nullptr);
        CreateDirectoryW(logsDir.c_str(), nullptr);
    }
}

void SecurityLogger::Log(const std::string& level, const std::string& tag, const std::string& message) {
    std::lock_guard<std::mutex> lock(m_mutex);

    int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    LogEntry entry;
    entry.timestamp = now;
    entry.level = level;
    entry.tag = tag;
    entry.message = message;

    m_logs.push_back(entry);
    if (m_logs.size() > 1000) {
        m_logs.erase(m_logs.begin());
    }

    // Write to file
    std::string filePath = GetLogFilePath();
    std::ofstream ofs(filePath, std::ios::app);
    if (ofs.is_open()) {
        auto timeT = static_cast<std::time_t>(now / 1000);
        std::tm tm{};
#if defined(_MSC_VER)
        localtime_s(&tm, &timeT);
#else
        std::tm* pTm = std::localtime(&timeT);
        if (pTm) tm = *pTm;
#endif
        ofs << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << " [" << level << "] [" << tag << "] " << message << "\n";
        ofs.close();
    }

    // Notify listeners
    for (const auto& listener : m_listeners) {
        if (listener) {
            listener(entry);
        }
    }
}

void SecurityLogger::Info(const std::string& tag, const std::string& message) {
    Log("INFO", tag, message);
}

void SecurityLogger::Security(const std::string& tag, const std::string& message) {
    Log("SECURITY", tag, message);
}

void SecurityLogger::Warn(const std::string& tag, const std::string& message) {
    Log("WARN", tag, message);
}

void SecurityLogger::Error(const std::string& tag, const std::string& message) {
    Log("ERROR", tag, message);
}

std::vector<LogEntry> SecurityLogger::GetRecentLogs(size_t count) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_logs.size() <= count) {
        return m_logs;
    }
    return std::vector<LogEntry>(m_logs.end() - count, m_logs.end());
}

void SecurityLogger::AddListener(std::function<void(const LogEntry&)> callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_listeners.push_back(callback);
}

} // namespace AnshuBio
