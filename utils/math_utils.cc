#include "math_utils.h"

namespace Haversine::MathUtils {
double square(double A) {
  double Result = (A * A);
  return Result;
}

double radiansFromDegrees(double Degrees) {
  double Result = 0.01745329251994329577f * Degrees;
  return Result;
}

double referenceHaversine(double x0, double y0, double x1, double y1,
                          double earthRadius) {
  double lat1 = y0;
  double lat2 = y1;
  double lon1 = x0;
  double lon2 = x1;

  double dLat = radiansFromDegrees(lat2 - lat1);
  double dLon = radiansFromDegrees(lon2 - lon1);
  lat1 = radiansFromDegrees(lat1);
  lat2 = radiansFromDegrees(lat2);

  double a = square(std::sin(dLat / 2.0)) +
             std::cos(lat1) * std::cos(lat2) * square(std::sin(dLon / 2));
  double c = 2.0 * std::asin(std::sqrt(a));

  double result = earthRadius * c;

  return result;
}
} // namespace Haversine::MathUtils
