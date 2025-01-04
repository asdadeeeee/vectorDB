/************************************************************************
Copyright 2017-2019 eBay Inc.
Author/Developer(s): Jung-Sang Ahn

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

#include <libnuraft/nuraft.hxx>
#include "cluster/raft_logger.h"

namespace vectordb {

/**
 * Example implementation of Raft logger, on top of SimpleLogger.
 */
class LoggerWrapper : public nuraft::logger {
 public:
  explicit LoggerWrapper(const std::string &log_file, int log_level = 6) {
    my_log_ = new SimpleLogger(log_file, 1024, 32 * 1024 * 1024, 10);
    my_log_->SetLogLevel(log_level);
    my_log_->SetDispLevel(-1);
    vectordb::SimpleLogger::SetCrashDumpPath("./", true);
    my_log_->Start();
  }

  ~LoggerWrapper() override { Destroy(); }

  void Destroy() {
    if (my_log_ != nullptr) {
      my_log_->FlushAll();
      my_log_->Stop();
      delete my_log_;
      my_log_ = nullptr;
    }
  }

  void put_details(int level, const char *source_file, const char *func_name, size_t line_number,
                   const std::string &msg) override {
    if (my_log_ != nullptr) {
      my_log_->Put(level, source_file, func_name, line_number, "%s", msg.c_str());
    }
  }

  void set_level(int l) override {
    if (my_log_ == nullptr) {
      return;
    }

    if (l < 0) {
      l = 1;
    }
    if (l > 6) {
      l = 6;
    }
    my_log_->SetLogLevel(l);
  }

  auto get_level() -> int override {
    if (my_log_ == nullptr) {
      return 0;
    }
    return my_log_->GetLogLevel();
  }

  auto GetLogger() const -> SimpleLogger * { return my_log_; }

 private:
  SimpleLogger *my_log_;
};

}  // namespace vectordb