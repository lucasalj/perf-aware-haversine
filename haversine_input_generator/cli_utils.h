#pragma once

#include <charconv>
#include <concepts>
#include <cstring>
#include <functional>
#include <random>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>

extern "C" {
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
}

namespace Haversine::CliUtils {

namespace {
template <class F, class Tuple1, class Tuple2, std::size_t... I>
constexpr decltype(auto)
applyToTupleAndArgsImpl(F &&f, Tuple1 &&t1, Tuple2 &&t2,
                        std::index_sequence<I...>) // exposition only
{
  return (std::invoke(std::forward<F>(f), std::get<I>(std::forward<Tuple1>(t1)),
                      std::get<I>(std::forward<Tuple2>(t2))),
          ...);
}

template <class F, class Tuple1, class... Args>
constexpr decltype(auto) applyToTupleAndArgs(F &&f, Tuple1 &&t1,
                                             Args &&...args) {
  return applyToTupleAndArgsImpl(
      std::forward<F>(f), std::forward<Tuple1>(t1),
      std::forward_as_tuple(std::forward<Args>(args)...),
      std::make_index_sequence<std::tuple_size_v<std::decay_t<Tuple1>>>{});
}
} // namespace

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

  template <typename... Args>
    requires(sizeof...(Args) == sizeof...(T))
  void parse(int argc, const char *argv[], Args &...args) {
    applyToTupleAndArgs(
        [&, i = 1](const auto &cliArgHelper, auto &out) mutable {
          if (i >= argc) {
            std::string errorMessage{
                "Error: Not all required arguments were filled!"};
            throw std::runtime_error(errorMessage);
          }
          out = cliArgHelper.extractValue(argv[i++]);
        },
        mCliArgs, args...);
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

void print(std::string_view text, int fileDescriptor = STDOUT_FILENO);

struct FileHandle {

  template <typename... Args>
  static FileHandle open(std::string_view filename, Args... args);
  ~FileHandle();

  bool mIsOpen = false;
  bool mNeedsClosing = false;
  int mFileDescriptor{0};
};

struct IoBufferedWriter {
  static constexpr std::size_t BUFFER_CAPACITY = 4096;

  explicit IoBufferedWriter(FileHandle &fileHandle);
  ~IoBufferedWriter();

  void printStr(const std::string &text);
  void printSv(std::string_view text);
  void printBin(std::span<std::byte> data);

  template <typename T> void writeBin(T &&value);

  template <typename Number, typename... FmtArgs>
  void printNumber(Number value, FmtArgs... fmtArgs);

  void flush();

  FileHandle *mFileHandle{nullptr};
  std::uint16_t mSize{0};
  std::array<std::byte, BUFFER_CAPACITY> mBuffer;
};

template <typename... Args>
inline FileHandle FileHandle::open(std::string_view filename, Args... args) {
  FileHandle result;
  auto fd = ::open(filename.data(), args...);
  if (fd == -1) {
    std::string errMsg = "Could not open file: ";
    errMsg.append(filename);
    throw std::runtime_error{errMsg};
  }
  result.mFileDescriptor = fd;
  result.mIsOpen = true;
  result.mNeedsClosing = true;
  return result;
}

template <typename T> void IoBufferedWriter::writeBin(T &&value) {
  std::array<std::byte, sizeof(T)> buffer;
  std::memcpy(buffer.data(), &value, sizeof(T));
  printBin(std::span{buffer.data(), buffer.size()});
}

template <typename Number, typename... FmtArgs>
void IoBufferedWriter::printNumber(Number value, FmtArgs... fmtArgs) {
  std::array<char, 24> buffer;
  auto [ptr, ec] = std::to_chars(buffer.data(), buffer.data() + buffer.size(),
                                 value, fmtArgs...);
  printSv(std::string_view(buffer.data(), ptr));
}

} // namespace Haversine::CliUtils
