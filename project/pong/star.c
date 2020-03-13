#include "star.h" 

int abStarCheck(const AbStar* star, const Vec2* centerPos, const Vec2* pixel){
    int size = star->size;
    int xDistFromCenter = abs(centerPos->axes[0] - pixel->axes[0]);
    int yDistFromCenter = abs(centerPos->axes[1] - pixel->axes[1]);
    
    if (centerPos->axes[0] == pixel->axes[0] && centerPos->axes[1] == pixel->axes[1]) return 1;
    
    if (yDistFromCenter == xDistFromCenter || yDistFromCenter == 0 || xDistFromCenter == 0) return 0;
    
    if (yDistFromCenter + xDistFromCenter <= size)
        return 1;
    return 0;
}

void abStarGetBounds(const AbStar* star, const Vec2 *centerPos, Region* bounds){
    int size = star->size;
    bounds->topLeft.axes[0] = centerPos->axes[0] - size;
    bounds->topLeft.axes[1] = centerPos->axes[1] - size;
    bounds->botRight.axes[0] = centerPos->axes[0] + size;
    bounds->botRight.axes[1] = centerPos->axes[1] + size;
}
