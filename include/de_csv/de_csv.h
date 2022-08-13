#pragma once

#include <concepts>
#include <istream>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "generator.h"

namespace de_csv {
template <typename T>
concept InputStream = std::derived_from<
    T, std::basic_istream<typename T::char_type, typename T::traits_type>>;

template <typename T>
concept FromVector = std::constructible_from<T, std::vector<std::string> &&>;

template <typename T>
concept FromUnorderedMap =
    std::constructible_from<T, std::unordered_map<std::string, std::string> &&>;

template <InputStream S> class CsvReader {
  S stream_;
  std::vector<std::string> header_;
  const bool has_header_;

public:
  // streamをmoveして使う
  CsvReader(S &&stream, bool has_header = true)
      : stream_(std::move(stream)), has_header_(has_header) {
    if (has_header_) {
      std::string buf;
      std::getline(stream_, buf);
      header_ = tokenize_(buf);
    }
  }

  CsvReader(S &&stream, std::vector<std::string> &&header)
      : stream_(std::move(stream)),
        header_(std::forward<std::vector<std::string>>(header)),
        has_header_(true) {}

  template <FromVector T> generator<std::optional<T>> deserialize() {
    std::string buf;
    while (std::getline(stream_, buf)) {
      try {
        co_yield T{tokenize_(buf)};
      } catch (...) {
        co_yield std::nullopt;
      }
    }
  }

  template <FromUnorderedMap T> generator<std::optional<T>> deserialize() {
    std::string buf;
    while (std::getline(stream_, buf)) {
      try {
        std::unordered_map<std::string, std::string> res;
        auto tmp = tokenize_(buf);
        for (size_t i = 0; i < header_.size(); i++) {
          res.emplace(header_.at(i), tmp.at(i));
        }
        co_yield T{std::move(res)};
      } catch (...) {
        co_yield std::nullopt;
      }
    }
  }

  const std::vector<std::string> &header() {
    return header_;
  }

private:
  // CSVの各行をtokenize
  std::vector<std::string> tokenize_(const std::string &str) {
    if (str.empty()) {
      return {};
    }

    std::vector<std::string> tokens;
    std::string::size_type l = 0;

    while (1) {
      auto r = str.find(',', l);
      if (r == std::string::npos) {
        tokens.emplace_back(str.begin() + l, str.end());
        break;
      }
      tokens.emplace_back(str.begin() + l, str.begin() + r);
      l = r + 1;
    }

    return tokens;
  }
};
} // namespace de_csv
