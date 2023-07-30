#pragma once

#include <charconv>
#include <functional>
#include <random>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>

#include <unistd.h>

namespace Haversine::CliUtils {

template <typename ValueExtractor, typename DebugValuePrinter>
class CommandLineArgument {
public:
  CommandLineArgument(std::string_view displayText,
                      ValueExtractor &&valueExtractor,
                      DebugValuePrinter &&debugValuePrinter)
      : mDisplayText{displayText}, mValueExtractor{std::forward<ValueExtractor>(
                                       valueExtractor)},
        mDebugValuePrinter{std::forward<DebugValuePrinter>(debugValuePrinter)} {
  }

  auto extractValue(std::string_view arg) const {
    return std::invoke(mValueExtractor, arg);
  }

  std::string_view displayText() const { return mDisplayText; }

  void debugValuePrinter(std::string &out, std::string_view arg) const {
    auto val = extractValue(arg);
    std::invoke(mDebugValuePrinter, out, val);
  }

private:
  std::string mDisplayText;
  ValueExtractor mValueExtractor;
  DebugValuePrinter mDebugValuePrinter;
};

template <typename... T> class CliHelper {
public:
  CliHelper(std::string_view programName, T &...args)
      : mProgramName{programName}, mCliArgs{args...} {}

  std::string displayMenu() const {
    using namespace std::literals;
    std::string result{"Usage: "};
    result.append(mProgramName);
    auto appendArg = [](auto &buf, const auto &arg) {
      buf += " [";
      buf += arg.displayText();
      buf += "]";
    };
    std::apply([&](const auto &...args) { ((appendArg(result, args)), ...); },
               mCliArgs);
    result.append("\n");
    return result;
  }

  std::string debugValues(int argc, const char *argv[]) const {
    std::string result;
    auto appendKeyValue = [&, idx = 1](auto &buf,
                                       const auto &arg) mutable -> void {
      buf.append(arg.displayText());
      buf.append(" = ");
      if (idx >= argc) {
        buf.append("[None]");
      } else {
        arg.debugValuePrinter(buf, argv[idx++]);
      }
      buf.append("\n");
    };
    std::apply(
        [&](const auto &...args) { ((appendKeyValue(result, args)), ...); },
        mCliArgs);
    return result;
  }

private:
  std::string_view mProgramName;
  std::tuple<T &...> mCliArgs;
};

enum class Mode { UNIFORM, CLUSTER };

Mode modeFrom(std::string_view rawText);

std::string_view modeToStrView(Mode mode);

void dumpMode(std::string &out, Mode mode);

std::uint64_t u64From(std::string_view rawText, std::string_view errMsgPrefix);
void dumpU64(std::string &out, std::uint64_t val);
std::string u64ToStr(std::uint64_t val);

std::uint64_t randomSeedFrom(std::string_view rawText);
std::uint64_t coordinatePairsFrom(std::string_view rawText);

void print(std::string_view text);
} // namespace Haversine::CliUtils
