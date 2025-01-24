#pragma once
#pragma warning(disable : 4996)
#include <chrono>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <optional>
#include <mutex>
#include <thread>
#include <shared_mutex>
#include <iostream>
#include <syncstream>

using namespace std::literals;

#define LOG(...) Logger::GetInstance().Log(__VA_ARGS__)

class Logger {
    mutable std::mutex mutex;
    std::ofstream log_file_;
    auto GetTime() const {
        if (manual_ts_) {
            return *manual_ts_;
        }
        return std::chrono::system_clock::now();
    }

    auto GetTimeStamp() const {
        const auto now = GetTime();
        const auto t_c = std::chrono::system_clock::to_time_t(now);
        return std::put_time(std::localtime(&t_c), "%F %T");
    }
    static std::string GetStringDate(const std::chrono::system_clock::time_point &tp) {
        auto t_c = std::chrono::system_clock::to_time_t(tp);
        std::ostringstream oss;
        oss << std::put_time(std::localtime(&t_c), "%Y_%m_%d");
        return "/var/log/sample_log_"s + oss.str() + ".log";
    }
    // Для имени файла возьмите дату с форматом "%Y_%m_%d"
    std::string GetFileTimeStamp() const {
        auto now = GetTime();
        return GetStringDate(now);
    }

    Logger() = default;
    Logger(const Logger&) = delete;

public:
    static Logger& GetInstance() {
        static Logger obj;
        return obj;
    }
    template<class... T>
    void LogArgs(const T&... args) {
        ((log_file_ << args), ...);
    }
    // Выведите в поток все аргументы.
    template<class... Ts>
    void Log(const Ts&... args) {
        std::lock_guard<std::mutex> lock(mutex);
        auto d = GetTimeStamp();
        log_file_ << d << ": "sv;
        LogArgs(args...);
        log_file_ << std::endl;
    }

    // Установите manual_ts_. Учтите, что эта операция может выполняться
    // параллельно с выводом в поток, вам нужно предусмотреть 
    // синхронизацию.
    void SetTimestamp(std::chrono::system_clock::time_point ts) {
        using namespace std::chrono;
        std::lock_guard<std::mutex> lock(mutex);
	    auto fileName = GetStringDate(ts);
        manual_ts_ = ts;
	    if (manual_str_day_ != fileName) {
            std::cout << fileName << std::endl;
	        manual_str_day_ = fileName;
            log_file_.close();
            log_file_.open(manual_str_day_, std::ios::out | std::ios::app);
        }
    }

private:
    std::optional<std::chrono::system_clock::time_point> manual_ts_;
    std::string manual_str_day_;
};
