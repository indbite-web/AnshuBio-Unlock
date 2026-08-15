#pragma once
#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include "../core/Models.h"

namespace AnshuBio {

class SecurityLogger {
public:
    static SecurityLogger& Instance();

    void Log(const std::string& level, const std::string& tag, const std::string& message);
    void Info(const std::string& tag, const std::string& message);
    void Security(const std::string& tag, const std::string& message);
    void Warn(const std::string& tag, const std::string& message);
    void Error(const std::string& tag, const std::string& message);

    std::vector<LogEntry> GetRecentLogs(size_t count = 100);
    void AddListener(std::function<void(const LogEntry&)> callback);

private:
    SecurityLogger();
    ~SecurityLogger();

    std::string GetLogFilePath();
    void EnsureDirectoryExists();

    std::mutex m_mutex;
    std::vector<LogEntry> m_logs;
    std::vector<std::function<void(const LogEntry&)>> m_listeners;
};

} // namespace AnshuBio
