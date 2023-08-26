#pragma once

#include <cmath>
#include <random>

namespace Haversine::MathUtils {
constexpr auto EARTH_RADIUS = 6372.8;

double square(double A);
double radiansFromDegrees(double Degrees);
double referenceHaversine(double x0, double y0, double x1, double y1,
                          double earthRadius = EARTH_RADIUS);

template <typename Rng>
inline double randomDegree(Rng &randSource, double center, double radius,
                           double maxAllowed) {
  auto minVal = std::max(center - radius, -maxAllowed);
  auto maxVal = std::min(center + radius, maxAllowed);

  std::uniform_real_distribution<double> unif{minVal, maxVal};
  return unif(randSource);
}
} // namespace Haversine::MathUtils
