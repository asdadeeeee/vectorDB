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

template <typename Derived>
class Singleton : public NonCopyable {
public:
    static auto Instance() -> Derived &
    {
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


inline auto GetCfgPath() -> std::string{
    const char* code_base = std::getenv("VECTORDB_CODE_BASE");
    std::string cfg_name = "vectordb_config";
    char *config_path =static_cast<char*>(malloc(strlen(code_base)+cfg_name.length()+2));
    memset(config_path, 0, strlen(code_base) + cfg_name.length() + 2);
    strcat(config_path, code_base);
    strcat(config_path, "/");
    strcat(config_path,cfg_name.c_str());
    std::string path_str(config_path);
    return path_str;
}



}  // namespace vectordb

