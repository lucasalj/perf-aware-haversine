#include "cli_utils.h"

namespace Haversine::CliUtils {
Mode modeFrom(std::string_view rawText) {
  if (rawText == "uniform") {
    return Mode::UNIFORM;
  }

  if (rawText == "cluster") {
    return Mode::CLUSTER;
  }

  std::string errorMessage = "Unrecognized mode: ";
  errorMessage.append(rawText);
  throw std::runtime_error(errorMessage);
}

std::string_view modeToStrView(Mode mode) {
  switch (mode) {
  case Mode::UNIFORM: {
    return "uniform";
  } break;
  case Mode::CLUSTER: {
    return "cluster";
  } break;
  default:
    break;
  }
  std::string errorMessage = "Invalid value for mode: ";
  errorMessage.append(std::to_string(int(mode)));
  throw std::runtime_error(errorMessage);
}

void dumpMode(std::string &out, Mode mode) { out.append(modeToStrView(mode)); }

std::uint64_t u64From(std::string_view rawText, std::string_view errMsgPrefix) {
  std::uint64_t value{0};
  auto [_, ec] =
      std::from_chars(rawText.data(), rawText.data() + rawText.size(), value);
  if (ec != std::errc()) {
    std::string errorMessage{errMsgPrefix};
    errorMessage.append(rawText);
    throw std::runtime_error(errorMessage);
  }
  return value;
}

void dumpU64(std::string &out, std::uint64_t val) {
  out.append(std::to_string(val));
}

std::string u64ToStr(std::uint64_t val) { return std::to_string(val); }

std::uint64_t randomSeedFrom(std::string_view rawText) {
  return u64From(rawText, "Invalid seed value: ");
}

std::uint64_t coordinatePairsFrom(std::string_view rawText) {
  return u64From(rawText, "Invalid number of coordinate pairs: ");
}

void print(std::string_view text) {
  auto r = write(STDOUT_FILENO, text.data(), text.size());
  if (r != text.size()) {
    std::abort();
  }
}

} // namespace Haversine::CliUtils
