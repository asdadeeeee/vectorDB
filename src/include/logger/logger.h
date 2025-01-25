#ifndef LOGGER_H
#define LOGGER_H

#include "spdlog/spdlog.h"
#include "common/vector_cfg.h"
namespace vectordb {
extern std::shared_ptr<spdlog::logger> global_logger;

void InitGlobalLogger(const std::string &log_name);
void SetLogLevel(spdlog::level::level_enum log_level);
}
#endif