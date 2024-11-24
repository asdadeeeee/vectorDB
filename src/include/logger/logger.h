#pragma once

#include "spdlog/spdlog.h"

namespace vectordb {
extern std::shared_ptr<spdlog::logger> global_logger;

void InitGlobalLogger();
void SetLogLevel(spdlog::level::level_enum log_level);
}