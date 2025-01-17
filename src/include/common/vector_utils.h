#pragma once

#include <cstring>
#include <string>
namespace vectordb {
class NonCopyable {
 public:
  NonCopyable(const NonCopyable &) = delete;
  auto operator=(const NonCopyable &) -> const NonCopyable & = delete;
  NonCopyable(NonCopyable &&) = delete;
  auto operator=(NonCopyable &&) -> const NonCopyable & = delete;

 protected:
  NonCopyable() = default;
  ~NonCopyable() = default;
};

// 仅链接到一个可执行文件 且保持默认导出所有符号表
template <typename Derived>
class Singleton : public NonCopyable {
 public:
  static auto Instance() -> Derived & {
    static Derived ins{};
    return ins;
  }
};

template <typename T>
constexpr auto CompileValue(T value __attribute__((unused)), T debugValue __attribute__((unused))) -> T {
#ifdef NDEBUG
  return value;
#else
  return debugValue;
#endif
}

inline auto GetCfgPath(const std::string &config_name) -> std::string {
  const char *code_base = std::getenv("VECTORDB_CODE_BASE");
  const std::string& cfg_name = config_name;
  char *config_path = static_cast<char *>(malloc(strlen(code_base) + cfg_name.length() + 2));
  memset(config_path, 0, strlen(code_base) + cfg_name.length() + 2);
  strcat(config_path, code_base);
  strcat(config_path, "/");
  strcat(config_path, cfg_name.c_str());
  std::string path_str(config_path);
  return path_str;
}


}  // namespace vectordb
