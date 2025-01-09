/************************************************************************
Modifications Copyright 2017-present eBay Inc.
Author/Developer(s): Jung-Sang Ahn

Original Copyright 2017 Jung-Sang Ahn
See URL: https://github.com/greensky00/simple_logger
         (v0.3.28)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    https://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
**************************************************************************/

#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <fstream>
#include <list>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

#include <csignal>
#include <cstdarg>
#if defined(__linux__) || defined(__APPLE__)
    #include <sys/time.h>
#endif

namespace vectordb {

// To suppress false alarms by thread sanitizer,
// add -DSUPPRESS_TSAN_FALSE_ALARMS=1 flag to CXXFLAGS.
// #define SUPPRESS_TSAN_FALSE_ALARMS (1)

// 0: System  [====]
// 1: Fatal   [FATL]
// 2: Error   [ERRO]
// 3: Warning [WARN]
// 4: Info    [INFO]
// 5: Debug   [DEBG]
// 6: Trace   [TRAC]


// printf style log macro
#define LOG(level, l, ...)        \
    if (l && l->GetLogLevel() >= level) \
        (l)->Put(level, __FILE__, __func__, __LINE__, __VA_ARGS__)

#define LOG_SYS(l, ...)    LOG(SimpleLogger::SYS,     l, __VA_ARGS__)
#define LOG_FATAL(l, ...)  LOG(SimpleLogger::FATAL,   l, __VA_ARGS__)
#define LOG_ERR(l, ...)    LOG(SimpleLogger::ERROR,   l, __VA_ARGS__)
#define LOG_WARN(l, ...)   LOG(SimpleLogger::WARNING, l, __VA_ARGS__)
#define LOG_INFO(l, ...)   LOG(SimpleLogger::INFO,    l, __VA_ARGS__)
#define LOG_DEBUG(l, ...)  LOG(SimpleLogger::DEBUG,   l, __VA_ARGS__)
#define LOG_TRACE(l, ...)  LOG(SimpleLogger::TRACE,   l, __VA_ARGS__)


// stream log macro
#define STREAM(level, l)              \
    if ((l) && (l)->GetLogLevel() >= (level)) \
        (l)->Eos() = (l)->Stream(level, l, __FILE__, __func__, __LINE__)

#define STREAM_SYS(l)   STREAM(SimpleLogger::SYS,     l)
#define STREAM_FATAL(l) STREAM(SimpleLogger::FATAL,   l)
#define STREAM_ERR(l)   STREAM(SimpleLogger::ERROR,   l)
#define STREAM_WARN(l)  STREAM(SimpleLogger::WARNING, l)
#define STREAM_INFO(l)  STREAM(SimpleLogger::INFO,    l)
#define STREAM_DEBUG(l) STREAM(SimpleLogger::DEBUG,   l)
#define STREAM_TRACE(l) STREAM(SimpleLogger::TRACE,   l)


// Do printf style log, but print logs in `lv1` level during normal time,
// once in given `interval_ms` interval, print a log in `lv2` level.
// The very first log will be printed in `lv2` level.
//
// This function is global throughout the process, so that
// multiple threads will share the interval.
#define TIMED_LOG_G(l, interval_ms, lv1, lv2, ...)                     \
{                                                                       \
    TIMED_LOG_DEFINITION(static);                                      \
    TIMED_LOG_BODY(l, interval_ms, lv1, lv2, __VA_ARGS__);             \
}

// Same as `TIMED_LOG_G` but per-thread level.
#define TIMED_LOG_T(l, interval_ms, lv1, lv2, ...)                     \
{                                                                       \
    TIMED_LOG_DEFINITION(thread_local);                                \
    TIMED_LOG_BODY(l, interval_ms, lv1, lv2, __VA_ARGS__);             \
}

#define TIMED_LOG_DEFINITION(prefix)                                   \
    prefix std::mutex timer_lock;                                       \
    (prefix) bool first_event_fired = false;                              \
    prefix std::chrono::system_clock::time_point last_timeout =         \
        std::chrono::system_clock::now();                               \

#define TIMED_LOG_BODY(l, interval_ms, lv1, lv2, ...)                  \
    std::chrono::system_clock::time_point cur =                         \
        std::chrono::system_clock::now();                               \
    bool timeout = false;                                               \
    {   std::lock_guard<std::mutex> l(timer_lock);                      \
        std::chrono::duration<double> elapsed = cur - last_timeout;     \
        if ( elapsed.count() * 1000 > interval_ms ||                    \
             !first_event_fired ) {                                     \
            cur = std::chrono::system_clock::now();                     \
            elapsed = cur - last_timeout;                               \
            if ( elapsed.count() * 1000 > interval_ms ||                \
                 !first_event_fired ) {                                 \
                timeout = first_event_fired = true;                     \
                last_timeout = cur;                                     \
            }                                                           \
        }                                                               \
    }                                                                   \
    if (timeout) {                                                      \
        LOG(lv2, l, __VA_ARGS__);                                     \
    } else {                                                            \
        LOG(lv1, l, __VA_ARGS__);                                     \
    }


class SimpleLoggerMgr;
class SimpleLogger {
    friend class SimpleLoggerMgr;
public:
    static const int MSG_SIZE = 4096;
    static const std::memory_order MOR = std::memory_order_relaxed;

    enum Levels {
        SYS         = 0,
        FATAL       = 1,
        ERROR       = 2,
        WARNING     = 3,
        INFO        = 4,
        DEBUG       = 5,
        TRACE       = 6,
        UNKNOWN     = 99,
    };

    class LoggerStream : public std::ostream {
    public:
        LoggerStream() : std::ostream(&buf_){}

        template<typename T>
        inline auto operator<<(const T& data) -> LoggerStream& {
            s_stream_ << data;
            return *this;
        }

        using MyCout = std::basic_ostream< char, std::char_traits<char> >;
        using EndlFunc = MyCout &(*)(MyCout &);
        inline auto operator<<(EndlFunc func) -> LoggerStream& {
            func(s_stream_);
            return *this;
        }

        inline void Put() {
            if (logger_ != nullptr) {
                logger_->Put( level_, file_, func_, line_,
                             "%s", s_stream_.str().c_str() );
            }
        }

        inline void SetLogInfo(int _level,
                               SimpleLogger* _logger,
                               const char* _file,
                               const char* _func,
                               size_t _line)
        {
            s_stream_.str(std::string());
            level_ = _level;
            logger_ = _logger;
            file_ = _file;
            func_ = _func;
            line_ = _line;
        }

    private:
        std::stringbuf buf_;
        std::stringstream s_stream_;
        int level_{0};
        SimpleLogger* logger_{nullptr};
        const char* file_{nullptr};
        const char* func_{nullptr};
        size_t line_{0};
    };

    class EndOfStmt {
    public:
        EndOfStmt() = default;
        explicit EndOfStmt(LoggerStream& src) { src.Put(); }
        auto operator=(LoggerStream& src) -> EndOfStmt& { src.Put(); return *this; }
    };

    auto Stream( int level,
                          SimpleLogger* logger,
                          const char* file,
                          const char* func,
                          size_t line ) -> LoggerStream& {
        thread_local LoggerStream msg;
        msg.SetLogInfo(level, logger, file, func, line);
        return msg;
    }

    auto Eos() -> EndOfStmt& {
        thread_local EndOfStmt eos;
        return eos;
    }

private:
    struct LogElem {
        enum Status {
            CLEAN       = 0,
            WRITING     = 1,
            DIRTY       = 2,
            FLUSHING    = 3,
        };

        LogElem();

        // True if dirty.
        auto NeedToFlush() -> bool;

        // True if no other thread is working on it.
        auto Available() -> bool;

        auto Write(size_t _len, char* msg) -> int;
        auto Flush(std::ofstream& fs) -> int;

        size_t len_;
        char ctx_[MSG_SIZE];
        std::atomic<Status> status_;
    };

public:
    explicit SimpleLogger(const std::string& file_path,
                 size_t max_log_elems           = 4096,
                 uint64_t log_file_size_limit   = 32*1024*1024,
                 uint32_t max_log_files         = 16);
    ~SimpleLogger();

    static void SetCriticalInfo(const std::string& info_str);
    static void SetCrashDumpPath(const std::string& path,
                                 bool origin_only = true);
    static void SetStackTraceOriginOnly(bool origin_only);
    static void LogStackBacktrace();

    static void Shutdown();
    static auto ReplaceString(const std::string& src_str,
                                     const std::string& before,
                                     const std::string& after) -> std::string;

    auto Start() -> int;
    auto Stop() -> int;

    inline auto TraceAllowed() const -> bool { return (cur_log_level_.load(MOR) >= 6); }
    inline auto DebugAllowed() const -> bool { return (cur_log_level_.load(MOR) >= 5); }

    void SetLogLevel(int level);
    void SetDispLevel(int level);
    void SetMaxLogFiles(size_t max_log_files);

    inline auto GetLogLevel()  const -> int { return cur_log_level_.load(MOR); }
    inline auto GetDispLevel() const -> int { return cur_disp_level_.load(MOR); }

    void Put(int level,
             const char* source_file,
             const char* func_name,
             size_t line_number,
             const char* format,
             ...);
    void FlushAll();

private:
    void CalcTzGap();
    void FindMinMaxRevNum(size_t& min_revnum_out,
                          size_t& max_revnum_out);
    void FindMinMaxRevNumInternal(bool& min_revnum_initialized,
                                  size_t& min_revnum,
                                  size_t& max_revnum,
                                  std::string& f_name);
    auto GetLogFilePath(size_t file_num) const -> std::string;
    void ExecCmd(const std::string& cmd);
    void DoCompression(size_t file_num);
    auto Flush(size_t start_pos) -> bool;

    std::string file_path_;
    size_t min_revnum_;
    size_t cur_revnum_;
    std::atomic<size_t> max_log_files_;
    std::ofstream fs_;

    uint64_t max_log_file_size_;
    std::atomic<uint32_t> num_comp_jobs_;

    // Log up to `curLogLevel`, default: 6.
    // Disable: -1.
    std::atomic<int> cur_log_level_;

    // Display (print out on terminal) up to `curDispLevel`,
    // default: 4 (do not print debug and trace).
    // Disable: -1.
    std::atomic<int> cur_disp_level_;

    std::mutex display_lock_;

    int tz_gap_;
    std::atomic<uint64_t> cursor_;
    std::vector<LogElem> logs_;
    std::mutex flushing_logs_;
};

// Singleton class
class SimpleLoggerMgr {
public:
    // Copy is not allowed.
    SimpleLoggerMgr(const SimpleLoggerMgr&) = delete;
    auto operator=(const SimpleLoggerMgr&) -> SimpleLoggerMgr& = delete;
    struct CompElem;

    struct TimeInfo {
        explicit TimeInfo(std::tm* src);
        explicit TimeInfo(std::chrono::system_clock::time_point now);
        int year_;
        int month_;
        int day_;
        int hour_;
        int min_;
        int sec_;
        int msec_;
        int usec_;
    };

    struct RawStackInfo {
        uint32_t tid_hash_{0};
        uint64_t kernel_tid_{0};
        std::vector<void*> stack_ptrs_;
        bool crash_origin_{false};
    };

    static auto Init() -> SimpleLoggerMgr*;
    static auto Get() -> SimpleLoggerMgr*;
    static auto GetWithoutInit() -> SimpleLoggerMgr*;
    static void Destroy();
    static auto GetTzGap() -> int;
    static void HandleSegFault(int sig);
    static void HandleSegAbort(int sig);
#if defined(__linux__) || defined(__APPLE__)
    static void HandleStackTrace(int sig, siginfo_t* info, void* secret);
#endif
    static void FlushWorker();
    static void CompressWorker();

    void LogStackBacktrace(size_t timeout_ms = 60*1000);
    void FlushCriticalInfo();
    void EnableOnlyOneDisplayer();
    void FlushAllLoggers() { FlushAllLoggers(0, std::string()); }
    void FlushAllLoggers(int level, const std::string& msg);
    void AddLogger(SimpleLogger* logger);
    void RemoveLogger(SimpleLogger* logger);
    void AddThread(uint64_t tid);
    void RemoveThread(uint64_t tid);
    void AddCompElem(SimpleLoggerMgr::CompElem* elem);
    void SleepFlusher(size_t ms);
    void SleepCompressor(size_t ms);
    auto ChkTermination() const -> bool;
    void SetCriticalInfo(const std::string& info_str);
    void SetCrashDumpPath(const std::string& path,
                          bool origin_only);
    void SetStackTraceOriginOnly(bool origin_only);

    /**
     * Set the flag regarding exiting on crash.
     * If flag is `true`, custom segfault handler will not invoke
     * original handler so that process will terminate without
     * generating core dump.
     * The flag is `false` by default.
     *
     * @param exit_on_crash New flag value.
     * @return void.
     */
    void SetExitOnCrash(bool exit_on_crash);

    auto GetCriticalInfo() const -> const std::string&;

    static std::mutex display_lock;

private:


    static const size_t STACK_TRACE_BUFFER_SIZE = 65536;

    // Singleton instance and lock.
    static std::atomic<SimpleLoggerMgr*> instance;
    static std::mutex instance_lock;

    SimpleLoggerMgr();
    ~SimpleLoggerMgr();

    void FlushStackTraceBuffer(size_t buffer_len,
                                uint32_t tid_hash,
                                uint64_t kernel_tid,
                                bool crash_origin);
    void FlushStackTraceBuffer(RawStackInfo& stack_info);
    void FlushRawStack(RawStackInfo& stack_info);
    void AddRawStackInfo(bool crash_origin = false);
    void LogStackBackTraceOtherThreads();

    auto ChkExitOnCrash() -> bool;

    std::mutex loggers_lock_;
    std::unordered_set<SimpleLogger*> loggers_;

    std::mutex active_threads_lock_;
    std::unordered_set<uint64_t> active_threads_;

    // Periodic log flushing thread.
    std::thread t_flush_;

    // Old log file compression thread.
    std::thread t_compress_;

    // List of files to be compressed.
    std::list<CompElem*> pending_comp_elems_;

    // Lock for `pending_comp_files`.
    std::mutex pending_comp_elems_lock_;

    // Condition variable for BG flusher.
    std::condition_variable cv_flusher_;
    std::mutex cv_flusher_lock_;

    // Condition variable for BG compressor.
    std::condition_variable cv_compressor_;
    std::mutex cv_compressor_lock_;

    // Termination signal.
    std::atomic<bool> termination_;

    // Original segfault handler.
    void (*old_sig_segv_handler_)(int);

    // Original abort handler.
    void (*old_sig_abort_handler_)(int);

    // Critical info that will be displayed on crash.
    std::string global_critical_info_;

    // Reserve some buffer for stack trace.
    char* stack_trace_buffer_;

    // TID of thread where crash happens.
    std::atomic<uint64_t> crash_origin_thread_;

    std::string crash_dump_path_;
    std::ofstream crash_dump_file_;

    // If `true`, generate stack trace only for the origin thread.
    // Default: `true`.
    bool crash_dump_origin_only_;

    // If `true`, do not invoke original segfault handler
    // so that process just terminates.
    // Default: `false`.
    bool exit_on_crash_;

    std::atomic<uint64_t> abort_timer_;

    // Assume that only one thread is updating this.
    std::vector<RawStackInfo> crash_dump_thread_stacks_;
};

}  // namespace vectordb