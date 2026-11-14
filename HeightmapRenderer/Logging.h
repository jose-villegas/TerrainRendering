#pragma once
// Minimal thread-safe logger that writes "date time:line <severity> message"
// records to the console and to a log file, replacing Boost.Log.
#include <ctime>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>

namespace logging
{
    enum class severity { trace, debug, info, warning, error, fatal };

    inline const char * severityName(severity level)
    {
        static const char * names[] = { "trace", "debug", "info", "warning", "error", "fatal" };
        return names[static_cast<int>(level)];
    }

    class Logger
    {
        public:
            static Logger &get()
            {
                static Logger instance;
                return instance;
            }

            void addFile(const std::string &filename)
            {
                std::lock_guard<std::mutex> lock(mutex);
                file.open(filename, std::ios::out | std::ios::trunc);
            }

            void setMinimum(severity level) { minimum = level; }

            void write(severity level, const std::string &message)
            {
                if(level < minimum) return;

                char stamp[32];
                std::time_t now = std::time(nullptr);
                std::tm local;
                localtime_s(&local, &now);
                std::strftime(stamp, sizeof(stamp), "%Y-%m-%d %H:%M:%S", &local);
                std::lock_guard<std::mutex> lock(mutex);
                std::ostringstream record;
                record << stamp << ":" << ++lineId << " <" << severityName(level)
                       << "> " << message << "\n";
                std::cout << record.str();

                if(file.is_open()) file << record.str() << std::flush;
            }

        private:
            Logger() = default;
            std::mutex mutex;
            std::ofstream file;
            unsigned int lineId = 0;
            severity minimum = severity::info;
    };

    // collects one record through operator<< and writes it when destroyed
    class Record
    {
        public:
            explicit Record(severity level) : level(level) {}
            ~Record() { Logger::get().write(level, stream.str()); }

            template<typename T>
            Record &operator<<(const T &value)
            {
                stream << value;
                return *this;
            }

        private:
            severity level;
            std::ostringstream stream;
    };
}

#define APP_LOG(level) logging::Record(logging::severity::level)
