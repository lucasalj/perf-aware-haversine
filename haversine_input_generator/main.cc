#include "cli_utils.h"
#include "math_utils.h"

#include <charconv>
#include <cmath>
#include <iostream>
#include <random>

int main(int argc, const char *argv[]) {
  using namespace Haversine::CliUtils;
  using namespace Haversine::MathUtils;
  CommandLineArgument argMode{"uniform/cluster", &modeFrom, &dumpMode};
  CommandLineArgument argSeed{"random seed", &randomSeedFrom, &dumpU64};
  CommandLineArgument argNCoord{"number of coordinate pairs to generate",
                                &coordinatePairsFrom, &dumpU64};

  CliHelper cli{"haversine_input_generator", argMode, argSeed, argNCoord};

  auto help = cli.displayMenu();
  Mode mode{};
  std::uint64_t seed{};
  std::uint64_t coordinatePairs{};
  auto stdOutHandle = FileHandle{.mIsOpen = true,
                                 .mNeedsClosing = false,
                                 .mFileDescriptor = STDOUT_FILENO};
  IoBufferedWriter stdOutWriter(stdOutHandle);
  try {
    cli.parse(argc, argv, mode, seed, coordinatePairs);
  } catch (const std::exception &e) {
    stdOutWriter.printSv(e.what());
    stdOutWriter.printSv("\n");
    stdOutWriter.printSv(help);
    return 1;
  }

  auto jsonFilename =
      std::string("data_") + std::to_string(coordinatePairs) + "_flex.json";

  auto binFilename = std::string("data_") + std::to_string(coordinatePairs) +
                     "_haveanswer.f64";

  std::mt19937_64 randomNumberGenerator{seed};
  std::uniform_real_distribution<double> xRandomCenterGenerator{-180., 180.};
  std::uniform_real_distribution<double> yRandomCenterGenerator{-90., 90.};
  std::uniform_real_distribution<double> xRandomRadiusGenerator{0., 180.};
  std::uniform_real_distribution<double> yRandomRadiusGenerator{0., 90.};

  double xCenter = 0.;
  double yCenter = 0.;
  double xRadius = 180.;
  double yRadius = 90.;

  auto jsonFileHandle =
      FileHandle::open(jsonFilename, O_WRONLY | O_CREAT | O_TRUNC,
                       S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  IoBufferedWriter jsonFileWriter(jsonFileHandle);

  auto binFileHandle =
      FileHandle::open(binFilename, O_WRONLY | O_CREAT | O_TRUNC,
                       S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  IoBufferedWriter binFileWriter(binFileHandle);

  double sum = 0;
  const double clusterCountMax = 1 + (coordinatePairs / 64);
  std::uint64_t clusterCountLeft = 0;
  auto sumCoeficient =
      coordinatePairs > 0 ? (1. / double(coordinatePairs)) : 0.;

  jsonFileWriter.printSv("{\"pairs\":[");
  for (auto i = 0ULL; i < coordinatePairs; ++i) {
    if (mode == Mode::CLUSTER && clusterCountLeft-- == 0) {
      clusterCountLeft = clusterCountMax;
      xCenter = xRandomCenterGenerator(randomNumberGenerator);
      yCenter = yRandomCenterGenerator(randomNumberGenerator);
      xRadius = xRandomRadiusGenerator(randomNumberGenerator);
      yRadius = yRandomRadiusGenerator(randomNumberGenerator);
    }
    auto x0 = randomDegree(randomNumberGenerator, xCenter, xRadius, 180.);
    auto y0 = randomDegree(randomNumberGenerator, yCenter, yRadius, 90.);
    auto x1 = randomDegree(randomNumberGenerator, xCenter, xRadius, 180.);
    auto y1 = randomDegree(randomNumberGenerator, yCenter, yRadius, 90.);
    auto haversineDistance = referenceHaversine(x0, y0, x1, y1);
    sum += sumCoeficient * haversineDistance;

    if (i == 0) {
      jsonFileWriter.printSv("\n{\"x0\":");
    } else {
      jsonFileWriter.printSv(",\n{\"x0\":");
    }
    jsonFileWriter.printNumber(x0, std::chars_format::fixed, 16);
    jsonFileWriter.printSv(",\"y0\":");
    jsonFileWriter.printNumber(y0, std::chars_format::fixed, 16);
    jsonFileWriter.printSv(",\"x1\":");
    jsonFileWriter.printNumber(x1, std::chars_format::fixed, 16);
    jsonFileWriter.printSv(",\"y1\":");
    jsonFileWriter.printNumber(y1, std::chars_format::fixed, 16);
    jsonFileWriter.printSv("}");

    binFileWriter.writeBin(haversineDistance);
  }
  jsonFileWriter.printSv("\n]}\n");

  stdOutWriter.printSv("Method: ");
  stdOutWriter.printSv(modeToStrView(mode));
  stdOutWriter.printSv("\nRandom seed: ");
  stdOutWriter.printNumber(seed);
  stdOutWriter.printSv("\nPair count: ");
  stdOutWriter.printNumber(coordinatePairs);
  stdOutWriter.printSv("\nExpected sum: ");
  stdOutWriter.printNumber(sum, std::chars_format::fixed, 16);
  stdOutWriter.printSv("\n\n");
  return 0;
}
