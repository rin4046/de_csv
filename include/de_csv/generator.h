#pragma once

#include <coroutine>

namespace de_csv {
template <typename T> struct generator {
  struct promise_type {
    T value_;
    auto get_return_object() {
      return generator{*this};
    }
    auto initial_suspend() {
      return std::suspend_never{};
    }
    auto final_suspend() noexcept {
      return std::suspend_always{};
    }
    auto yield_value(T &&value) {
      value_ = std::forward<T>(value);
      return std::suspend_always{};
    }
    void return_void() {}
    void unhandled_exception() {
      std::terminate();
    }
  };

  generator(generator const &) = delete;
  generator(generator &&rhs) : coro_(rhs.coro_) {
    rhs.coro_ = nullptr;
  }
  ~generator() {
    if (coro_) {
      coro_.destroy();
    }
  }

  struct iterator {
    generator *gen_;
    T operator*() {
      return gen_->coro_.promise().value_;
    }
    iterator &operator++() {
      if (gen_->coro_) {
        gen_->coro_.resume();
      }
      return *this;
    }
    bool operator!=(const iterator &) {
      return !gen_->coro_.done();
    }
  };

  iterator begin() {
    return {this};
  }
  iterator end() {
    return {nullptr};
  }

private:
  std::coroutine_handle<promise_type> coro_;
  generator(promise_type &p)
      : coro_(std::coroutine_handle<promise_type>::from_promise(p)) {}
};
} // namespace de_csv
