#include "cli_utils.h"
#include "json_parser.h"
#include "math_utils.h"
#include <cstring>

extern "C" {
#include <fcntl.h>
#include <unistd.h>
}

namespace {
constexpr ssize_t INITIAL_BUFFER_SIZE = ssize_t(4) * ssize_t(1024);

std::string getString(std::string_view txt) {
  return std::string(txt.data(), txt.size());
}

std::string_view toStringView(const std::string &txt) {
  return std::string_view(txt);
}
} // namespace

int main(int argc, const char *argv[]) {
  using namespace Haversine::CliUtils;
  using namespace Haversine::MathUtils;
  CommandLineArgument argFilename{"filename", &getString, &toStringView};

  CliHelper cli{"haversine_processor", argFilename};

  auto help = cli.displayMenu();
  auto stdOutHandle = FileHandle{.mIsOpen = true,
                                 .mNeedsClosing = false,
                                 .mFileDescriptor = STDOUT_FILENO};
  std::string filename;
  IoBufferedWriter stdOutWriter(stdOutHandle);
  try {
    cli.parse(argc, argv, filename);
  } catch (const std::exception &e) {
    stdOutWriter.printSv(e.what());
    stdOutWriter.printSv("\n");
    stdOutWriter.printSv(help);
    return 1;
  }

  auto inputFile = FileHandle::open(filename, O_RDONLY);
  ssize_t bufferSize = INITIAL_BUFFER_SIZE;
  std::unique_ptr<char[]> buffer = std::make_unique<char[]>(bufferSize);
  ssize_t readIndex = 0;
  while (true) {
    auto nextReadSize = bufferSize - readIndex;
    if (nextReadSize == 0) {
      auto newBufferSize = bufferSize * 2;
      std::unique_ptr<char[]> newBuffer =
          std::make_unique<char[]>(newBufferSize);
      std::memcpy(newBuffer.get(), buffer.get(), bufferSize);
      std::swap(buffer, newBuffer);
      bufferSize = newBufferSize;
      nextReadSize = bufferSize - readIndex;
    }
    auto bytesRead = ::read(inputFile.mFileDescriptor, buffer.get() + readIndex,
                            nextReadSize);

    if (bytesRead < 0)
      return int(bytesRead);

    if (bytesRead == 0)
      break;

    readIndex += bytesRead;
  }
  std::string_view fileString(buffer.get(), readIndex);
  auto json = json_parser::Json::parse(fileString);

  auto &arrayOfPairs = json.getMemberValue("pairs").getArray().mElements;
  auto sumCoeficient =
      !arrayOfPairs.empty() ? (1. / double(arrayOfPairs.size())) : 0.;
  double sum = 0;
  for (const auto &elem : arrayOfPairs) {
    auto x0 = elem->getMemberValue("x0").getNumber().getFloatingPoint();
    auto y0 = elem->getMemberValue("y0").getNumber().getFloatingPoint();
    auto x1 = elem->getMemberValue("x1").getNumber().getFloatingPoint();
    auto y1 = elem->getMemberValue("y1").getNumber().getFloatingPoint();

    auto haversineDistance = referenceHaversine(x0, y0, x1, y1);
    sum += sumCoeficient * haversineDistance;
  }

  stdOutWriter.printSv("Pair count: ");
  stdOutWriter.printNumber(arrayOfPairs.size());
  stdOutWriter.printSv("\nExpected sum: ");
  stdOutWriter.printNumber(sum, std::chars_format::fixed, 16);
  stdOutWriter.printSv("\n\n");
  return 0;
}
