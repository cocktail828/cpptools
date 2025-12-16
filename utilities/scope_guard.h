#pragma once

#include <utility>
#include <type_traits>
#include <exception>

namespace cpptools {
namespace utilities {

template <typename F>
class ScopeGuard {
   public:
    static_assert(std::is_nothrow_destructible_v<F> || std::is_invocable_v<F>, "Deferred callable must be invocable");

    explicit ScopeGuard(F&& f) noexcept(std::is_nothrow_move_constructible_v<F>)
        : func_(std::forward<F>(f)), active_(true) {}

    // non-copyable
    ScopeGuard(const ScopeGuard&) = delete;
    ScopeGuard& operator=(const ScopeGuard&) = delete;

    // movable
    ScopeGuard(ScopeGuard&& other) noexcept(std::is_nothrow_move_constructible_v<F>)
        : func_(std::move(other.func_)), active_(other.active_) {
        other.dismiss();
    }

    ScopeGuard& operator=(ScopeGuard&& other) noexcept(std::is_nothrow_move_assignable_v<F>) {
        if (this != &other) {
            execute();
            func_ = std::move(other.func_);
            active_ = other.active_;
            other.dismiss();
        }
        return *this;
    }

    ~ScopeGuard() noexcept { execute(); }

    void dismiss() noexcept { active_ = false; }

   private:
    void execute() noexcept {
        if (active_) {
            try {
                func_();
            } catch (...) {
                std::terminate();
            }
            active_ = false;
        }
    }

   private:
    F func_;
    bool active_;
};

template <typename F>
[[nodiscard]] ScopeGuard<F> make_scope_guard(F&& f) {
    return ScopeGuard<F>(std::forward<F>(f));
}

}  // namespace utilities
}  // namespace cpptools

#define CONCAT_IMPL(x, y) x##y
#define CONCAT(x, y) CONCAT_IMPL(x, y)

// c++17 or higher is required
#define ON_SCOPE_EXIT_RAW(code) ::cpptools::utilities::make_scope_guard(code)
#define ON_SCOPE_EXIT(code) auto CONCAT(_scope_guard_, __LINE__) = ::cpptools::utilities::make_scope_guard(code)
