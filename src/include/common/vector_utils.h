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
}  // namespace vectordb