#include "../free_hand_orientation.h"
#include <cassert>

int main() {
    // Wrist at an arbitrary world position, knuckles pointing down, thumb-side up in +X.
    const armik::V wrist{4,7,11}, middle{4,7,2}, index{8,7,2}, pinky{2,7,2};
    for (float angle : {0.f, .53f, 1.57f, 3.14f}) {
        const armik::V forward{std::cos(angle), std::sin(angle), 0};
        const armik::V up{0,0,1};
        float delta[4][4]{};
        assert(freehand::Align(wrist,middle,index,pinky,forward,up,.75f,delta));
        const auto along = handanchor::Point(middle,delta) - handanchor::Point(wrist,delta);
        const auto across = handanchor::Point(index,delta) - handanchor::Point(pinky,delta);
        assert(armik::Length(along - forward*6.75f) < 1e-4f);
        assert(armik::Length(across - up*4.5f) < 1e-4f);
    }
    float sentinel[4][4]{{17}};
    assert(!freehand::Align(wrist,wrist,index,pinky,{1,0,0},{0,0,1},1,sentinel));
    assert(!freehand::Align(wrist,middle,index,index,{1,0,0},{0,0,1},1,sentinel));
    assert(!freehand::Align(wrist,middle,index,pinky,{1,0,0},{1,0,0},1,sentinel));
    assert(sentinel[0][0] == 17);
}
