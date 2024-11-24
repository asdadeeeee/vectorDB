#include "logger/logger.h"
#include "spdlog/sinks/stdout_color_sinks.h"

namespace vectordb {
std::shared_ptr<spdlog::logger> global_logger;

void InitGlobalLogger() {
    global_logger = spdlog::stdout_color_mt("global_logger");
}

void SetLogLevel(spdlog::level::level_enum log_level) {
    global_logger->set_level(log_level);
}
}  // namespace vectordb
