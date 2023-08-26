#include "cli_utils.h"

#include <iterator>

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

std::uint64_t randomSeedFrom(std::string_view rawText) {
  return u64From(rawText, "Invalid seed value: ");
}

std::uint64_t coordinatePairsFrom(std::string_view rawText) {
  return u64From(rawText, "Invalid number of coordinate pairs: ");
}

void print(std::string_view text, int fileDescriptor) {
  auto r = write(fileDescriptor, text.data(), text.size());
  if (r != text.size()) {
    std::abort();
  }
}

FileHandle::~FileHandle() {
  if (mIsOpen && mNeedsClosing) {
    ::close(mFileDescriptor);
    mIsOpen = false;
  }
}

IoBufferedWriter::IoBufferedWriter(FileHandle &fileHandle)
    : mFileHandle(&fileHandle) {}

IoBufferedWriter::~IoBufferedWriter() {
  try {
    flush();
  } catch (...) {
  }
}

void IoBufferedWriter::printSv(std::string_view text) {
  auto copySize = std::min(BUFFER_CAPACITY - mSize, text.size());
  while (copySize > 0) {
    std::memcpy(mBuffer.data() + mSize, text.data(), copySize);
    mSize += copySize;
    if (mSize == BUFFER_CAPACITY)
      flush();
    text.remove_prefix(copySize);
    copySize = std::min(BUFFER_CAPACITY - mSize, text.size());
  }
}

void IoBufferedWriter::printStr(const std::string &text) {
  printSv(std::string_view(text));
}

void IoBufferedWriter::printBin(std::span<std::byte> data) {
  auto copySize = std::min(BUFFER_CAPACITY - mSize, data.size());
  while (copySize > 0) {
    std::memcpy(mBuffer.data() + mSize, data.data(), copySize);
    mSize += copySize;
    if (mSize == BUFFER_CAPACITY)
      flush();
    data = data.subspan(copySize);
    copySize = std::min(BUFFER_CAPACITY - mSize, data.size());
  }
}

void IoBufferedWriter::flush() {
  ssize_t alreadyWritten = 0;
  for (;;) {
    if (mSize == 0)
      return;
    auto r = ::write(mFileHandle->mFileDescriptor,
                     mBuffer.data() + alreadyWritten, mSize);
    if (r < 0) {
      throw std::runtime_error("Unable to write to file");
    }
    alreadyWritten += r;
    mSize -= r;
  }
}

} // namespace Haversine::CliUtils
