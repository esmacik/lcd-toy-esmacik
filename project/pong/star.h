#include "shape.h" 

//Size will be the radius, not the diameter
typedef struct AbStar_s {
  void (*getBounds)(const struct AbStar_s* shape, const Vec2 *centerPos, Region *bounds);
  int (*check)(const struct AbStar_s *shape, const Vec2 *centerPos, const Vec2 *pixelLoc);
  int size;
} AbStar;

int abStarCheck(const AbStar* star, const Vec2* centerPos, const Vec2 *pixel);

void abStarGetBounds(const AbStar *star, const Vec2 *centerPos, Region *bounds);
