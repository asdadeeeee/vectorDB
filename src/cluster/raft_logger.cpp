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

#include "cluster/raft_logger.h"

#if defined(__linux__) || defined(__APPLE__)
#include "libnuraft/backtrace.h"
#endif

#include <algorithm>
#include <iomanip>
#include <iostream>

#include <cassert>

#if defined(__linux__) || defined(__APPLE__)
#include <dirent.h>
#ifdef __linux__
#include <pthread.h>
#endif
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

#elif defined(WIN32) || defined(_WIN32)
#include <Windows.h>
#undef min_
#undef max
#endif

#include <cstdlib>
#include <cstring>
namespace vectordb {

#ifndef _CLM_DEFINED
#define _CLM_DEFINED (1)

#ifdef LOGGER_NO_COLOR
#define _CLM_D_GRAY ""
#define _CLM_GREEN ""
#define _CLM_B_GREEN ""
#define _CLM_RED ""
#define _CLM_B_RED ""
#define _CLM_BROWN ""
#define _CLM_B_BROWN ""
#define _CLM_BLUE ""
#define _CLM_B_BLUE ""
#define _CLM_MAGENTA ""
#define _CLM_B_MAGENTA ""
#define _CLM_CYAN ""
#define _CLM_END ""

#define _CLM_WHITE_FG_RED_BG ""
#else
#define _CLM_D_GRAY "\033[1;30m"
#define _CLM_GREEN "\033[32m"
#define _CLM_B_GREEN "\033[1;32m"
#define _CLM_RED "\033[31m"
#define _CLM_B_RED "\033[1;31m"
#define _CLM_BROWN "\033[33m"
#define _CLM_B_BROWN "\033[1;33m"
#define _CLM_BLUE "\033[34m"
#define _CLM_B_BLUE "\033[1;34m"
#define _CLM_MAGENTA "\033[35m"
#define _CLM_B_MAGENTA "\033[1;35m"
#define _CLM_CYAN "\033[36m"
#define _CLM_B_GREY "\033[1;37m"
#define _CLM_END "\033[0m"

#define _CLM_WHITE_FG_RED_BG "\033[37;41m"
#endif

#define _CL_D_GRAY(str) _CLM_D_GRAY str _CLM_END
#define _CL_GREEN(str) _CLM_GREEN str _CLM_END
#define _CL_RED(str) _CLM_RED str _CLM_END
#define _CL_B_RED(str) _CLM_B_RED str _CLM_END
#define _CL_MAGENTA(str) _CLM_MAGENTA str _CLM_END
#define _CL_BROWN(str) _CLM_BROWN str _CLM_END
#define _CL_B_BROWN(str) _CLM_B_BROWN str _CLM_END
#define _CL_B_BLUE(str) _CLM_B_BLUE str _CLM_END
#define _CL_B_MAGENTA(str) _CLM_B_MAGENTA str _CLM_END
#define _CL_CYAN(str) _CLM_CYAN str _CLM_END
#define _CL_B_GRAY(str) _CLM_B_GREY str _CLM_END

#define _CL_WHITE_FG_RED_BG(str) _CLM_WHITE_FG_RED_BG str _CLM_END

#endif

std::atomic<SimpleLoggerMgr *> SimpleLoggerMgr::instance(nullptr);
std::mutex SimpleLoggerMgr::instance_lock;
std::mutex SimpleLoggerMgr::display_lock;

// Number of digits to represent thread IDs (Linux only).
std::atomic<int> tid_digits(2);

struct SimpleLoggerMgr::CompElem {
  CompElem(uint64_t num, SimpleLogger *logger) : file_num_(num), target_logger_(logger) {}
  uint64_t file_num_;
  SimpleLogger *target_logger_;
};

SimpleLoggerMgr::TimeInfo::TimeInfo(std::tm *src)
    : year_(src->tm_year + 1900),
      month_(src->tm_mon + 1),
      day_(src->tm_mday),
      hour_(src->tm_hour),
      min_(src->tm_min),
      sec_(src->tm_sec),
      msec_(0),
      usec_(0) {}

SimpleLoggerMgr::TimeInfo::TimeInfo(std::chrono::system_clock::time_point now) {
  std::time_t raw_time = std::chrono::system_clock::to_time_t(now);
  std::tm new_time;

#if defined(__linux__) || defined(__APPLE__)
  std::tm *lt_tm = localtime_r(&raw_time, &new_time);

#elif defined(WIN32) || defined(_WIN32)
  localtime_s(&new_time, &raw_time);
  std::tm *lt_tm = &new_time;
#endif

  year_ = lt_tm->tm_year + 1900;
  month_ = lt_tm->tm_mon + 1;
  day_ = lt_tm->tm_mday;
  hour_ = lt_tm->tm_hour;
  min_ = lt_tm->tm_min;
  sec_ = lt_tm->tm_sec;

  size_t us_epoch = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
  msec_ = (us_epoch / 1000) % 1000;
  usec_ = us_epoch % 1000;
}

auto SimpleLoggerMgr::Init() -> SimpleLoggerMgr * {
  SimpleLoggerMgr *mgr = instance.load(SimpleLogger::MOR);
  if (mgr == nullptr) {
    std::lock_guard<std::mutex> l(instance_lock);
    mgr = instance.load(SimpleLogger::MOR);
    if (mgr == nullptr) {
      mgr = new SimpleLoggerMgr();
      instance.store(mgr, SimpleLogger::MOR);
    }
  }
  return mgr;
}

auto SimpleLoggerMgr::Get() -> SimpleLoggerMgr * {
  SimpleLoggerMgr *mgr = instance.load(SimpleLogger::MOR);
  if (mgr == nullptr) {
    return Init();
  }
  return mgr;
}

auto SimpleLoggerMgr::GetWithoutInit() -> SimpleLoggerMgr * {
  SimpleLoggerMgr *mgr = instance.load(SimpleLogger::MOR);
  return mgr;
}

void SimpleLoggerMgr::Destroy() {
  std::lock_guard<std::mutex> l(instance_lock);
  SimpleLoggerMgr *mgr = instance.load(SimpleLogger::MOR);
  if (mgr != nullptr) {
    mgr->FlushAllLoggers();
    delete mgr;
    instance.store(nullptr, SimpleLogger::MOR);
  }
}

auto SimpleLoggerMgr::GetTzGap() -> int {
  std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
  std::time_t raw_time = std::chrono::system_clock::to_time_t(now);
  std::tm new_time;

#if defined(__linux__) || defined(__APPLE__)
  std::tm *lt_tm = localtime_r(&raw_time, &new_time);
  std::tm *gmt_tm = std::gmtime(&raw_time);

#elif defined(WIN32) || defined(_WIN32)
  localtime_s(&new_time, &raw_time);
  std::tm *lt_tm = &new_time;
  std::tm new_gmt_time;
  gmtime_s(&new_gmt_time, &raw_time);
  std::tm *gmt_tm = &new_gmt_time;
#endif

  TimeInfo lt(lt_tm);
  TimeInfo gmt(gmt_tm);

  return ((lt.day_ * 60 * 24 + lt.hour_ * 60 + lt.min_) - (gmt.day_ * 60 * 24 + gmt.hour_ * 60 + gmt.min_));
}

// LCOV_EXCL_START

void SimpleLoggerMgr::FlushCriticalInfo() {
  std::string msg = " === Critical info (given by user): ";
  msg += std::to_string(global_critical_info_.size()) + " bytes";
  msg += " ===";
  if (!global_critical_info_.empty()) {
    msg += "\n" + global_critical_info_;
  }
  FlushAllLoggers(2, msg);
  if (crash_dump_file_.is_open()) {
    crash_dump_file_ << msg << std::endl << std::endl;
  }
}

void SimpleLoggerMgr::FlushStackTraceBuffer(size_t buffer_len, uint32_t tid_hash, uint64_t kernel_tid,
                                            bool crash_origin) {
  std::string msg;
  char temp_buf[256];
  snprintf(temp_buf, 256, "\nThread %04x", tid_hash);
  msg += temp_buf;
  if (kernel_tid != 0U) {
    msg += " (" + std::to_string(kernel_tid) + ")";
  }
  if (crash_origin) {
    msg += " (crashed here)";
  }
  msg += "\n\n";
  msg += std::string(stack_trace_buffer_, buffer_len);

  size_t msg_len = msg.size();
  size_t per_log_size = SimpleLogger::MSG_SIZE - 1024;
  for (size_t ii = 0; ii < msg_len; ii += per_log_size) {
    FlushAllLoggers(2, msg.substr(ii, per_log_size));
  }

  if (crash_dump_file_.is_open()) {
    crash_dump_file_ << msg << std::endl;
  }
}

void SimpleLoggerMgr::FlushStackTraceBuffer(RawStackInfo &stack_info) {
#if defined(__linux__) || defined(__APPLE__)
  size_t len = _stack_interpret(&stack_info.stack_ptrs_[0], stack_info.stack_ptrs_.size(), stack_trace_buffer_,
                                STACK_TRACE_BUFFER_SIZE);
  if (len == 0U) {
    return;
  }

  FlushStackTraceBuffer(len, stack_info.tid_hash_, stack_info.kernel_tid_, stack_info.crash_origin_);
#endif
}

void SimpleLoggerMgr::LogStackBackTraceOtherThreads() {
  bool got_other_stacks = false;
#ifdef __linux__
  if (!crash_dump_origin_only_) {
    // Not support non-Linux platform.
    uint64_t my_tid = pthread_self();
    uint64_t exp = 0;
    if (!crash_origin_thread_.compare_exchange_strong(exp, my_tid)) {
      // Other thread is already working on it, stop here.
      return;
    }

    std::lock_guard<std::mutex> l(active_threads_lock_);
    std::string msg = "captured ";
    msg += std::to_string(active_threads_.size()) + " active threads";
    FlushAllLoggers(2, msg);
    if (crash_dump_file_.is_open()) {
      crash_dump_file_ << msg << "\n\n";
    }

    for (uint64_t ttid : active_threads_) {
      auto tid = static_cast<pthread_t>(ttid);
      if (ttid == crash_origin_thread_) {
        continue;
      }

      struct sigaction action;
      sigfillset(&action.sa_mask);
      action.sa_flags = SA_SIGINFO;
      action.sa_sigaction = SimpleLoggerMgr::HandleStackTrace;
      sigaction(SIGUSR2, &action, nullptr);

      pthread_kill(tid, SIGUSR2);

      sigset_t mask;
      sigfillset(&mask);
      sigdelset(&mask, SIGUSR2);
      sigsuspend(&mask);
    }

    msg = "got all stack traces, now flushing them";
    FlushAllLoggers(2, msg);

    got_other_stacks = true;
  }
#endif

  if (!got_other_stacks) {
    std::string msg = "will not explore other threads (disabled by user)";
    FlushAllLoggers(2, msg);
    if (crash_dump_file_.is_open()) {
      crash_dump_file_ << msg << "\n\n";
    }
  }
}

void SimpleLoggerMgr::FlushRawStack(RawStackInfo &stack_info) {
  if (!crash_dump_file_.is_open()) {
    return;
  }

  crash_dump_file_ << "Thread " << std::hex << std::setw(4) << std::setfill('0') << stack_info.tid_hash_ << std::dec
                   << " " << stack_info.kernel_tid_ << std::endl;
  if (stack_info.crash_origin_) {
    crash_dump_file_ << "(crashed here)" << std::endl;
  }
  for (void *stack_ptr : stack_info.stack_ptrs_) {
    crash_dump_file_ << std::hex << stack_ptr << std::dec << std::endl;
  }
  crash_dump_file_ << std::endl;
}

void SimpleLoggerMgr::AddRawStackInfo(bool crash_origin) {
#if defined(__linux__) || defined(__APPLE__)
  void *stack_ptr[256];
  size_t len = _stack_backtrace(stack_ptr, 256);

  crash_dump_thread_stacks_.emplace_back(RawStackInfo());
  RawStackInfo &stack_info = *(crash_dump_thread_stacks_.rbegin());
  std::thread::id tid = std::this_thread::get_id();
  stack_info.tid_hash_ = std::hash<std::thread::id>{}(tid) % 0x10000;
#ifdef __linux__
  stack_info.kernel_tid_ = static_cast<uint64_t>(syscall(SYS_gettid));
#endif
  stack_info.crash_origin_ = crash_origin;
  for (size_t ii = 0; ii < len; ++ii) {
    stack_info.stack_ptrs_.push_back(stack_ptr[ii]);
  }
#endif
}

void SimpleLoggerMgr::LogStackBacktrace(size_t timeout_ms) {
  // Set abort timeout: 60 seconds.
  abort_timer_ = timeout_ms;

  if (!crash_dump_path_.empty() && !crash_dump_file_.is_open()) {
    // Open crash dump file.
    TimeInfo lt(std::chrono::system_clock::now());
    int tz_gap = GetTzGap();
    int tz_gap_abs = (tz_gap < 0) ? (tz_gap * -1) : (tz_gap);

    char filename[128];
    snprintf(filename, 128, "dump_%04d%02d%02d_%02d%02d%02d%c%02d%02d.txt", lt.year_, lt.month_, lt.day_, lt.hour_,
             lt.min_, lt.sec_, (tz_gap >= 0) ? '+' : '-', (tz_gap_abs / 60), tz_gap_abs % 60);
    std::string path = crash_dump_path_ + "/" + filename;
    crash_dump_file_.open(path);

    char time_fmt[64];
    snprintf(time_fmt, 64, "%04d-%02d-%02dT%02d:%02d:%02d.%03d%03d%c%02d:%02d", lt.year_, lt.month_, lt.day_, lt.hour_,
             lt.min_, lt.sec_, lt.msec_, lt.usec_, (tz_gap >= 0) ? '+' : '-', (tz_gap_abs / 60), tz_gap_abs % 60);
    crash_dump_file_ << "When: " << time_fmt << std::endl << std::endl;
  }

  FlushCriticalInfo();
  AddRawStackInfo(true);
  // Collect other threads' stack info.
  LogStackBackTraceOtherThreads();

  // Now print out.
  // For the case where `addr2line` is hanging, Flush raw pointer first.
  for (RawStackInfo &entry : crash_dump_thread_stacks_) {
    FlushRawStack(entry);
  }
  for (RawStackInfo &entry : crash_dump_thread_stacks_) {
    FlushStackTraceBuffer(entry);
  }
}

auto SimpleLoggerMgr::ChkExitOnCrash() -> bool {
  if (exit_on_crash_) {
    return true;
  }

  std::string env_segv_str;
  const char *env_segv = std::getenv("SIMPLELOGGER_EXIT_ON_CRASH");
  if (env_segv != nullptr) {
    env_segv_str = env_segv;
  }

  return env_segv_str == "ON" || env_segv_str == "on" || env_segv_str == "TRUE" || env_segv_str == "true";
}

void SimpleLoggerMgr::HandleSegFault(int sig) {
#if defined(__linux__) || defined(__APPLE__)
  SimpleLoggerMgr *mgr = SimpleLoggerMgr::Get();
  signal(SIGSEGV, mgr->old_sig_segv_handler_);
  mgr->EnableOnlyOneDisplayer();
  mgr->FlushAllLoggers(1, "Segmentation fault");
  mgr->LogStackBacktrace();

  printf("[SEG FAULT] Flushed all logs_ safely.\n");
  fflush(stdout);

  if (mgr->ChkExitOnCrash()) {
    printf("[SEG FAULT] Exit on crash.\n");
    fflush(stdout);
    exit(-1);
  }

  if (mgr->old_sig_segv_handler_ != nullptr) {
    mgr->old_sig_segv_handler_(sig);
  }
#endif
}

void SimpleLoggerMgr::HandleSegAbort(int sig) {
#if defined(__linux__) || defined(__APPLE__)
  SimpleLoggerMgr *mgr = SimpleLoggerMgr::Get();
  signal(SIGABRT, mgr->old_sig_abort_handler_);
  mgr->EnableOnlyOneDisplayer();
  mgr->FlushAllLoggers(1, "Abort");
  mgr->LogStackBacktrace();

  printf("[ABORT] Flushed all logs_ safely.\n");
  fflush(stdout);

  if (mgr->ChkExitOnCrash()) {
    printf("[ABORT] Exit on crash.\n");
    fflush(stdout);
    exit(-1);
  }

  abort();
#endif
}

#if defined(__linux__) || defined(__APPLE__)
void SimpleLoggerMgr::HandleStackTrace(int sig, siginfo_t *info, void *secret) {
#ifndef __linux__
  // Not support non-Linux platform.
  return;
#else
  SimpleLoggerMgr *mgr = SimpleLoggerMgr::Get();
  if (mgr->crash_origin_thread_ == 0U) {
    return;
  }

  pthread_t myself = pthread_self();
  if (mgr->crash_origin_thread_ == myself) {
    return;
  }

  // NOTE:
  //   As getting exact line number is too expensive,
  //   keep stack pointers first and then interpret it.
  mgr->AddRawStackInfo();

  // Go back to origin thread.
  pthread_kill(mgr->crash_origin_thread_, SIGUSR2);
#endif
}
#endif

// LCOV_EXCL_STOP

void SimpleLoggerMgr::FlushWorker() {
#ifdef __linux__
  pthread_setname_np(pthread_self(), "sl_flusher");
#endif
  SimpleLoggerMgr *mgr = SimpleLoggerMgr::Get();
  while (!mgr->ChkTermination()) {
    // Every 500ms.
    size_t sub_ms = 500;
    mgr->SleepFlusher(sub_ms);
    mgr->FlushAllLoggers();
    if (mgr->abort_timer_ != 0U) {
      if (mgr->abort_timer_ > sub_ms) {
        mgr->abort_timer_.fetch_sub(sub_ms);
      } else {
        std::cerr << "STACK DUMP TIMEOUT, FORCE ABORT" << std::endl;
        exit(-1);
      }
    }
  }
}

void SimpleLoggerMgr::CompressWorker() {
#ifdef __linux__
  pthread_setname_np(pthread_self(), "sl_compressor");
#endif
  SimpleLoggerMgr *mgr = SimpleLoggerMgr::Get();
  bool sleep_next_time = true;
  while (!mgr->ChkTermination()) {
    // Every 500ms.
    size_t sub_ms = 500;
    if (sleep_next_time) {
      mgr->SleepCompressor(sub_ms);
    }
    sleep_next_time = true;

    CompElem *elem = nullptr;
    {
      std::lock_guard<std::mutex> l(mgr->pending_comp_elems_lock_);
      auto entry = mgr->pending_comp_elems_.begin();
      if (entry != mgr->pending_comp_elems_.end()) {
        elem = *entry;
        mgr->pending_comp_elems_.erase(entry);
      }
    }

    if (elem != nullptr) {
      elem->target_logger_->DoCompression(elem->file_num_);
      delete elem;
      // Continuous compression if pending item exists.
      sleep_next_time = false;
    }
  }
}

void SimpleLoggerMgr::SetCrashDumpPath(const std::string &path, bool origin_only) {
  crash_dump_path_ = path;
  SetStackTraceOriginOnly(origin_only);
}

void SimpleLoggerMgr::SetStackTraceOriginOnly(bool origin_only) { crash_dump_origin_only_ = origin_only; }

void SimpleLoggerMgr::SetExitOnCrash(bool exit_on_crash) { exit_on_crash_ = exit_on_crash; }

SimpleLoggerMgr::SimpleLoggerMgr()
    : termination_(false),
      old_sig_segv_handler_(nullptr),
      old_sig_abort_handler_(nullptr),
      stack_trace_buffer_(nullptr),
      crash_origin_thread_(0),
      crash_dump_origin_only_(true),
      exit_on_crash_(false),
      abort_timer_(0) {
#if defined(__linux__) || defined(__APPLE__)
  std::string env_segv_str;
  const char *env_segv = std::getenv("SIMPLELOGGER_HANDLE_SEGV");
  if (env_segv != nullptr) {
    env_segv_str = env_segv;
  }

  if (env_segv_str == "OFF" || env_segv_str == "off" || env_segv_str == "FALSE" || env_segv_str == "false") {
    // Manually turned off by user, via env var.
  } else {
    old_sig_segv_handler_ = signal(SIGSEGV, SimpleLoggerMgr::HandleSegFault);
    old_sig_abort_handler_ = signal(SIGABRT, SimpleLoggerMgr::HandleSegAbort);
  }
  stack_trace_buffer_ = static_cast<char *>(malloc(STACK_TRACE_BUFFER_SIZE));

#endif
  t_flush_ = std::thread(SimpleLoggerMgr::FlushWorker);
  t_compress_ = std::thread(SimpleLoggerMgr::CompressWorker);
}

SimpleLoggerMgr::~SimpleLoggerMgr() {
  termination_ = true;

#if defined(__linux__) || defined(__APPLE__)
  signal(SIGSEGV, old_sig_segv_handler_);
  signal(SIGABRT, old_sig_abort_handler_);
#endif
  {
    std::unique_lock<std::mutex> l(cv_flusher_lock_);
    cv_flusher_.notify_all();
  }
  {
    std::unique_lock<std::mutex> l(cv_compressor_lock_);
    cv_compressor_.notify_all();
  }
  if (t_flush_.joinable()) {
    t_flush_.join();
  }
  if (t_compress_.joinable()) {
    t_compress_.join();
  }

  free(stack_trace_buffer_);
}

// LCOV_EXCL_START
void SimpleLoggerMgr::EnableOnlyOneDisplayer() {
  bool marked = false;
  std::unique_lock<std::mutex> l(loggers_lock_);
  for (auto &entry : loggers_) {
    SimpleLogger *logger = entry;
    if (logger == nullptr) {
      continue;
    }
    if (!marked) {
      // The first logger: enable display
      if (logger->GetLogLevel() < 4) {
        logger->SetLogLevel(4);
      }
      logger->SetDispLevel(4);
      marked = true;
    } else {
      // The others: disable display
      logger->SetDispLevel(-1);
    }
  }
}
// LCOV_EXCL_STOP

void SimpleLoggerMgr::FlushAllLoggers(int level, const std::string &msg) {
  std::unique_lock<std::mutex> l(loggers_lock_);
  for (auto &entry : loggers_) {
    SimpleLogger *logger = entry;
    if (logger == nullptr) {
      continue;
    }
    if (!msg.empty()) {
      logger->Put(level, __FILE__, __func__, __LINE__, "%s", msg.c_str());
    }
    logger->FlushAll();
  }
}

void SimpleLoggerMgr::AddLogger(SimpleLogger *logger) {
  std::unique_lock<std::mutex> l(loggers_lock_);
  loggers_.insert(logger);
}

void SimpleLoggerMgr::RemoveLogger(SimpleLogger *logger) {
  std::unique_lock<std::mutex> l(loggers_lock_);
  loggers_.erase(logger);
}

void SimpleLoggerMgr::AddThread(uint64_t tid) {
  std::unique_lock<std::mutex> l(active_threads_lock_);
  active_threads_.insert(tid);
}

void SimpleLoggerMgr::RemoveThread(uint64_t tid) {
  std::unique_lock<std::mutex> l(active_threads_lock_);
  active_threads_.erase(tid);
}

void SimpleLoggerMgr::AddCompElem(SimpleLoggerMgr::CompElem *elem) {
  {
    std::unique_lock<std::mutex> l(pending_comp_elems_lock_);
    pending_comp_elems_.push_back(elem);
  }
  {
    std::unique_lock<std::mutex> l(cv_compressor_lock_);
    cv_compressor_.notify_all();
  }
}

void SimpleLoggerMgr::SleepFlusher(size_t ms) {
  std::unique_lock<std::mutex> l(cv_flusher_lock_);
  cv_flusher_.wait_for(l, std::chrono::milliseconds(ms));
}

void SimpleLoggerMgr::SleepCompressor(size_t ms) {
  std::unique_lock<std::mutex> l(cv_compressor_lock_);
  cv_compressor_.wait_for(l, std::chrono::milliseconds(ms));
}

auto SimpleLoggerMgr::ChkTermination() const -> bool { return termination_; }

void SimpleLoggerMgr::SetCriticalInfo(const std::string &info_str) { global_critical_info_ = info_str; }

auto SimpleLoggerMgr::GetCriticalInfo() const -> const std::string & { return global_critical_info_; }

// ==========================================

struct ThreadWrapper {
#ifdef __linux__
  ThreadWrapper() {
    my_self_ = static_cast<uint64_t>(pthread_self());
    my_tid_ = static_cast<uint32_t>(syscall(SYS_gettid));

    // Get the number of digits for alignment.
    int num_digits = 0;
    uint32_t tid = my_tid_;
    while (tid != 0U) {
      num_digits++;
      tid /= 10;
    }
    int exp = tid_digits;
    const size_t max_num_cmp = 10;
    size_t count = 0;
    while (exp < num_digits && count++ < max_num_cmp) {
      if (tid_digits.compare_exchange_strong(exp, num_digits)) {
        break;
      }
      exp = tid_digits;
    }

    SimpleLoggerMgr *mgr = SimpleLoggerMgr::GetWithoutInit();
    if (mgr != nullptr) {
      mgr->AddThread(my_self_);
    }
  }
  ~ThreadWrapper() {
    SimpleLoggerMgr *mgr = SimpleLoggerMgr::GetWithoutInit();
    if (mgr != nullptr) {
      mgr->RemoveThread(my_self_);
    }
  }
#else
  ThreadWrapper() : myTid(0) {}
  ~ThreadWrapper() {}
#endif
  uint64_t my_self_;
  uint32_t my_tid_;
};

// ==========================================

SimpleLogger::LogElem::LogElem() : len_(0), status_(CLEAN) { memset(ctx_, 0x0, MSG_SIZE); }

// True if dirty.
auto SimpleLogger::LogElem::NeedToFlush() -> bool { return status_.load(MOR) == DIRTY; }

// True if no other thread is working on it.
auto SimpleLogger::LogElem::Available() -> bool {
  Status s = status_.load(MOR);
  return s == CLEAN || s == DIRTY;
}

auto SimpleLogger::LogElem::Write(size_t _len, char *msg) -> int {
  Status exp = CLEAN;
  Status val = WRITING;
  if (!status_.compare_exchange_strong(exp, val)) {
    return -1;
  }

  len_ = (_len > MSG_SIZE) ? MSG_SIZE : _len;
  memcpy(ctx_, msg, len_);

  status_.store(LogElem::DIRTY);
  return 0;
}

auto SimpleLogger::LogElem::Flush(std::ofstream &fs_) -> int {
  Status exp = DIRTY;
  Status val = FLUSHING;
  if (!status_.compare_exchange_strong(exp, val)) {
    return -1;
  }

  fs_.write(ctx_, len_);

  status_.store(LogElem::CLEAN);
  return 0;
}

// ==========================================

SimpleLogger::SimpleLogger(const std::string &file_path, size_t max_log_elems, uint64_t log_file_size_limit,
                           uint32_t max_log_files)
    : file_path_(ReplaceString(file_path, "//", "/")),
      max_log_files_(max_log_files),
      max_log_file_size_(log_file_size_limit),
      num_comp_jobs_(0),
      cur_log_level_(4),
      cur_disp_level_(4),
      tz_gap_(SimpleLoggerMgr::GetTzGap()),
      cursor_(0),
      logs_(max_log_elems) {
  FindMinMaxRevNum(min_revnum_, cur_revnum_);
}

SimpleLogger::~SimpleLogger() { Stop(); }

void SimpleLogger::SetCriticalInfo(const std::string &info_str) {
  SimpleLoggerMgr *mgr = SimpleLoggerMgr::Get();
  if (mgr != nullptr) {
    mgr->SetCriticalInfo(info_str);
  }
}

void SimpleLogger::SetCrashDumpPath(const std::string &path, bool origin_only) {
  SimpleLoggerMgr *mgr = SimpleLoggerMgr::Get();
  if (mgr != nullptr) {
    mgr->SetCrashDumpPath(path, origin_only);
  }
}

void SimpleLogger::SetStackTraceOriginOnly(bool origin_only) {
  SimpleLoggerMgr *mgr = SimpleLoggerMgr::Get();
  if (mgr != nullptr) {
    mgr->SetStackTraceOriginOnly(origin_only);
  }
}

void SimpleLogger::LogStackBacktrace() {
  SimpleLoggerMgr *mgr = SimpleLoggerMgr::Get();
  if (mgr != nullptr) {
    mgr->EnableOnlyOneDisplayer();
    mgr->LogStackBacktrace(0);
  }
}

void SimpleLogger::Shutdown() {
  SimpleLoggerMgr *mgr = SimpleLoggerMgr::GetWithoutInit();
  if (mgr != nullptr) {
    mgr->Destroy();
  }
}

auto SimpleLogger::ReplaceString(const std::string &src_str, const std::string &before, const std::string &after)
    -> std::string {
  size_t last = 0;
  size_t pos = src_str.find(before, last);
  std::string ret;
  while (pos != std::string::npos) {
    ret += src_str.substr(last, pos - last);
    ret += after;
    last = pos + before.size();
    pos = src_str.find(before, last);
  }
  if (last < src_str.size()) {
    ret += src_str.substr(last);
  }
  return ret;
}

void SimpleLogger::FindMinMaxRevNum(size_t &min_revnum_out, size_t &max_revnum_out) {
  std::string dir_path = "./";
  std::string file_name_only = file_path_;
  size_t last_pos = file_path_.rfind('/');
  if (last_pos != std::string::npos) {
    dir_path = file_path_.substr(0, last_pos);
    file_name_only = file_path_.substr(last_pos + 1, file_path_.size() - last_pos - 1);
  }

  bool min_revnum_initialized = false;
  size_t min_revnum = 0;
  size_t max_revnum = 0;

#if defined(__linux__) || defined(__APPLE__)
  DIR *dir_info = opendir(dir_path.c_str());
  struct dirent *dir_entry = nullptr;
  while ((dir_info != nullptr) && ((dir_entry = readdir(dir_info)) != nullptr)) {
    std::string f_name(dir_entry->d_name);
    size_t f_name_pos = f_name.rfind(file_name_only);
    // Irrelavent file: skip.
    if (f_name_pos == std::string::npos) {
      continue;
    }

    FindMinMaxRevNumInternal(min_revnum_initialized, min_revnum, max_revnum, f_name);
  }
  if (dir_info != nullptr) {
    closedir(dir_info);
  }
#elif defined(WIN32) || defined(_WIN32)
  // Windows
  WIN32_FIND_DATA filedata;
  HANDLE hfind;
  std::string query_str = dir_path + "*";

  // find all files start with 'prefix'
  hfind = FindFirstFile(query_str.c_str(), &filedata);
  while (hfind != INVALID_HANDLE_VALUE) {
    std::string f_name(filedata.cFileName);
    size_t f_name_pos = f_name.rfind(file_name_only);
    // Irrelavent file: skip.
    if (f_name_pos != std::string::npos) {
      findMinMaxRevNumInternal(min_revnum_initialized, min_revnum, max_revnum, f_name);
    }

    if (!FindNextFile(hfind, &filedata)) {
      FindClose(hfind);
      hfind = INVALID_HANDLE_VALUE;
    }
  }
#endif

  min_revnum_out = min_revnum;
  max_revnum_out = max_revnum;
}

void SimpleLogger::FindMinMaxRevNumInternal(bool &min_revnum_initialized, size_t &min_revnum, size_t &max_revnum,
                                            std::string &f_name) {
  size_t last_dot = f_name.rfind('.');
  if (last_dot == std::string::npos) {
    return;
  }

  bool comp_file = false;
  std::string ext = f_name.substr(last_dot + 1, f_name.size() - last_dot - 1);
  if (ext == "gz" && f_name.size() > 7) {
    // Compressed file: asdf.log.123.tar.gz => need to Get 123.
    f_name = f_name.substr(0, f_name.size() - 7);
    last_dot = f_name.rfind('.');
    if (last_dot == std::string::npos) {
      return;
    }
    ext = f_name.substr(last_dot + 1, f_name.size() - last_dot - 1);
    comp_file = true;
  }

  size_t revnum = atoi(ext.c_str());
  max_revnum = std::max(max_revnum, ((comp_file) ? (revnum + 1) : (revnum)));
  if (!min_revnum_initialized) {
    min_revnum = revnum;
    min_revnum_initialized = true;
  }
  min_revnum = std::min(min_revnum, revnum);
}

auto SimpleLogger::GetLogFilePath(size_t file_num) const -> std::string {
  if (file_num != 0U) {
    return file_path_ + "." + std::to_string(file_num);
  }
  return file_path_;
}

auto SimpleLogger::Start() -> int {
  if (file_path_.empty()) {
    return 0;
  }

  // Append at the end.
  fs_.open(GetLogFilePath(cur_revnum_), std::ofstream::out | std::ofstream::app);
  if (!fs_) {
    return -1;
  }

  SimpleLoggerMgr *mgr = SimpleLoggerMgr::Get();
  SimpleLogger *ll = this;
  mgr->AddLogger(ll);

  LOG_SYS(ll, "Start logger: %s (%zu MB per file, up to %zu files)", file_path_.c_str(),
          max_log_file_size_ / 1024 / 1024, max_log_files_.load());

  const std::string &critical_info = mgr->GetCriticalInfo();
  if (!critical_info.empty()) {
    LOG_INFO(ll, "%s", critical_info.c_str());
  }

  return 0;
}

auto SimpleLogger::Stop() -> int {
  if (fs_.is_open()) {
    SimpleLoggerMgr *mgr = SimpleLoggerMgr::GetWithoutInit();
    if (mgr != nullptr) {
      SimpleLogger *ll = this;
      mgr->RemoveLogger(ll);

      LOG_SYS(ll, "Stop logger: %s", file_path_.c_str());
      FlushAll();
      fs_.flush();
      fs_.close();

      while (num_comp_jobs_.load() > 0) {
        std::this_thread::yield();
      }
    }
  }

  return 0;
}

void SimpleLogger::SetLogLevel(int level) {
  if (level > 6) {
    return;
  }
  if (!fs_) {
    return;
  }

  cur_log_level_ = level;
}

void SimpleLogger::SetDispLevel(int level) {
  if (level > 6) {
    return;
  }

  cur_disp_level_ = level;
}

void SimpleLogger::SetMaxLogFiles(size_t max_log_files) {
  if (max_log_files == 0) {
    return;
  }

  max_log_files_ = max_log_files;
}
/*
#define _snprintf(msg, avail_len, cur_len, msg_len, ...)         \
  avail_len = (avail_len > cur_len) ? (avail_len - cur_len) : 0; \
  msg_len = snprintf(msg + cur_len, avail_len, __VA_ARGS__);     \
  cur_len += (avail_len > msg_len) ? msg_len : avail_len
*/

#define _vsnprintf(msg, avail_len, cur_len, msg_len, ...)        \
  avail_len = (avail_len > cur_len) ? (avail_len - cur_len) : 0; \
  msg_len = vsnprintf(msg + cur_len, avail_len, __VA_ARGS__);    \
  cur_len += (avail_len > msg_len) ? msg_len : avail_len

void SimpleLogger::Put(int level, const char *source_file, const char *func_name, size_t line_number,
                       const char *format, ...) {
  if (level > cur_log_level_.load(MOR)) {
    return;
  }
  if (!fs_) {
    return;
  }

  static const char *lv_names[7] = {"====", "FATL", "ERRO", "WARN", "INFO", "DEBG", "TRAC"};
  char msg[MSG_SIZE];
  thread_local ThreadWrapper thread_wrapper;
#ifdef __linux__
  const int tid_digits = vectordb::tid_digits;
  thread_local uint32_t tid_hash = thread_wrapper.my_tid_;
#else
  thread_local std::thread::id tid = std::this_thread::get_id();
  thread_local uint32_t tid_hash = std::hash<std::thread::id>{}(tid) % 0x10000;
#endif

  // Print filename part only (excluding directory path).
  size_t last_slash = 0;
  for (size_t ii = 0; (source_file != nullptr) && source_file[ii] != 0; ++ii) {
    if (source_file[ii] == '/' || source_file[ii] == '\\') {
      last_slash = ii;
    }
  }

  SimpleLoggerMgr::TimeInfo lt(std::chrono::system_clock::now());
  int tz_gap_abs = (tz_gap_ < 0) ? (tz_gap_ * -1) : (tz_gap_);

  // [time] [tid] [log type] [user msg] [stack info]
  // Timestamp: ISO 8601 format.
  size_t cur_len = 0;
  size_t avail_len = MSG_SIZE;
  size_t msg_len = 0;

#ifdef __linux__
  _snprintf(msg, avail_len, cur_len, msg_len,
            "%04d-%02d-%02dT%02d:%02d:%02d.%03d_%03d%c%02d:%02d "
            "[%*u] "
            "[%s] ",
            lt.year_, lt.month_, lt.day_, lt.hour_, lt.min_, lt.sec_, lt.msec_, lt.usec_, (tz_gap_ >= 0) ? '+' : '-',
            tz_gap_abs / 60, tz_gap_abs % 60, tid_digits, tid_hash, lv_names[level]);
#else
  _snprintf(msg, avail_len, cur_len, msg_len,
            "%04d-%02d-%02dT%02d:%02d:%02d.%03d_%03d%c%02d:%02d "
            "[%04x] "
            "[%s] ",
            lt.year_, lt.month_, lt.day_, lt.hour_, lt.min_, lt.sec_, lt.msec_, lt.usec_, (tz_gap_ >= 0) ? '+' : '-',
            tz_gap_abs / 60, tz_gap_abs % 60, tid_hash, lv_names[level]);
#endif

  va_list args;
  va_start(args, format);
  _vsnprintf(msg, avail_len, cur_len, msg_len, format, args);
  va_end(args);

  if ((source_file != nullptr) && (func_name != nullptr)) {
    _snprintf(msg, avail_len, cur_len, msg_len, "\t[%s:%zu, %s()]\n",
              source_file + ((last_slash) ? (last_slash + 1) : 0), line_number, func_name);
  } else {
    _snprintf(msg, avail_len, cur_len, msg_len, "\n");
  }

  size_t num = logs_.size();
  uint64_t cursor_exp = 0;
  uint64_t cursor_val = 0;
  LogElem *ll = nullptr;
  do {
    cursor_exp = cursor_.load(MOR);
    cursor_val = (cursor_exp + 1) % num;
    ll = &logs_[cursor_exp];
  } while (!cursor_.compare_exchange_strong(cursor_exp, cursor_val, MOR));
  while (!ll->Available()) {
    std::this_thread::yield();
  }

  if (ll->NeedToFlush()) {
    // Allow only one thread to Flush.
    if (!Flush(cursor_exp)) {
      // Other threads: wait.
      while (ll->NeedToFlush()) {
        std::this_thread::yield();
      }
    }
  }
  ll->Write(cur_len, msg);

  if (level > cur_disp_level_) {
    return;
  }

  // Console part.
  static const char *colored_lv_names[7] = {
      _CL_B_BROWN("===="), _CL_WHITE_FG_RED_BG("FATL"), _CL_B_RED("ERRO"), _CL_B_MAGENTA("WARN"), "INFO",
      _CL_D_GRAY("DEBG"),  _CL_D_GRAY("TRAC")};

  cur_len = 0;
  avail_len = MSG_SIZE;
#ifdef __linux__
  _snprintf( msg, avail_len, cur_len, msg_len,
               " [" _CL_BROWN("%02d") ":" _CL_BROWN("%02d") ":" _CL_BROWN("%02d") "."
               _CL_BROWN("%03d") " " _CL_BROWN("%03d")
               "] [tid " _CL_B_BLUE("%*u") "] "
               "[%s] ",
               lt.hour_, lt.min_, lt.sec_, lt.msec_, lt.usec_,
               tid_digits, tid_hash,
               colored_lv_names[level] );
#else
  _snprintf( msg, avail_len, cur_len, msg_len,
               " [" _CL_BROWN("%02d") ":" _CL_BROWN("%02d") ":" _CL_BROWN("%02d") "."
               _CL_BROWN("%03d") " " _CL_BROWN("%03d")
               "] [tid " _CL_B_BLUE("%04x") "] "
               "[%s] ",
               lt.hour_, lt.min_, lt.sec_, lt.msec_, lt.usec_,
               tid_hash,
               colored_lv_names[level] );
#endif

  if ((source_file != nullptr) && (func_name != nullptr)) {
    _snprintf(msg, avail_len, cur_len, msg_len, "[" _CL_GREEN("%s") ":" _CL_B_RED("%zu") ", " _CL_CYAN("%s()") "]\n",
              source_file + ((last_slash) ? (last_slash + 1) : 0), line_number, func_name);
  } else {
    _snprintf(msg, avail_len, cur_len, msg_len, "\n");
  }

  va_start(args, format);

#ifndef LOGGER_NO_COLOR
  if (level == 0) {
    _snprintf(msg, avail_len, cur_len, msg_len, _CLM_B_BROWN);
  } else if (level == 1) {
    _snprintf(msg, avail_len, cur_len, msg_len, _CLM_B_RED);
  }
#endif

  _vsnprintf(msg, avail_len, cur_len, msg_len, format, args);

#ifndef LOGGER_NO_COLOR
  _snprintf(msg, avail_len, cur_len, msg_len, _CLM_END);
#endif

  va_end(args);
  (void)cur_len;

  std::unique_lock<std::mutex> l(SimpleLoggerMgr::display_lock);
  std::cout << msg << std::endl;
  l.unlock();
}

void SimpleLogger::ExecCmd(const std::string &cmd_given) {
  int r = 0;
  std::string cmd = cmd_given;

#if defined(__linux__)
  cmd += " > /dev/null";
  r = system(cmd.c_str());

#elif defined(__APPLE__)
  cmd += " 2> /dev/null";
  FILE *fp = popen(cmd.c_str(), "r");
  r = pclose(fp);
#endif
  (void)r;
}

void SimpleLogger::DoCompression(size_t file_num) {
#if defined(__linux__) || defined(__APPLE__)
  std::string filename = GetLogFilePath(file_num);
  std::string cmd;
  cmd = "tar zcvf " + filename + ".tar.gz " + filename;
  ExecCmd(cmd);

  cmd = "rm -f " + filename;
  ExecCmd(cmd);

  size_t max_log_files = max_log_files_.load();
  // Remove previous log files.
  if ((max_log_files != 0U) && file_num >= max_log_files) {
    for (size_t ii = min_revnum_; ii <= file_num - max_log_files; ++ii) {
      filename = GetLogFilePath(ii);
      std::string filename_tar = GetLogFilePath(ii) + ".tar.gz";
      cmd = "rm -f " + filename + " ";
      cmd.append(filename_tar);
      ExecCmd(cmd);
      min_revnum_ = ii + 1;
    }
  }
#endif

  num_comp_jobs_.fetch_sub(1);
}

auto SimpleLogger::Flush(size_t start_pos) -> bool {
  std::unique_lock<std::mutex> ll(flushing_logs_, std::try_to_lock);
  if (!ll.owns_lock()) {
    return false;
  }

  size_t num = logs_.size();
  // Circular Flush into file.
  for (size_t ii = start_pos; ii < num; ++ii) {
    LogElem &ll = logs_[ii];
    ll.Flush(fs_);
  }
  for (size_t ii = 0; ii < start_pos; ++ii) {
    LogElem &ll = logs_[ii];
    ll.Flush(fs_);
  }
  fs_.flush();

  if ((max_log_file_size_ != 0U) && fs_.tellp() > static_cast<int64_t>(max_log_file_size_)) {
    // Exceeded limit, make a new file.
    cur_revnum_++;
    fs_.close();
    fs_.open(GetLogFilePath(cur_revnum_), std::ofstream::out | std::ofstream::app);

    // Compress it (tar gz). Register to the global queue.
    SimpleLoggerMgr *mgr = SimpleLoggerMgr::GetWithoutInit();
    if (mgr != nullptr) {
      num_comp_jobs_.fetch_add(1);
      auto *elem = new SimpleLoggerMgr::CompElem(cur_revnum_ - 1, this);
      mgr->AddCompElem(elem);
    }
  }

  return true;
}

void SimpleLogger::FlushAll() {
  uint64_t start_pos = cursor_.load(MOR);
  Flush(start_pos);
}

}  // namespace vectordb