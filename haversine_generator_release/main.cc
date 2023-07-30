#include "cli_utils.h"

int main(int argc, const char *argv[]) {
  using namespace Haversine::CliUtils;
  CommandLineArgument argMode{"uniform/cluster", &modeFrom, &dumpMode};
  CommandLineArgument argSeed{"random seed", &randomSeedFrom, &dumpU64};
  CommandLineArgument argNCoord{"number of coordinate pairs to generate",
                                &coordinatePairsFrom, &dumpU64};

  CliHelper cli{"haversine_genator_release", argMode, argSeed, argNCoord};

  auto help = cli.displayMenu();
  print(help);
  return 0;
}
