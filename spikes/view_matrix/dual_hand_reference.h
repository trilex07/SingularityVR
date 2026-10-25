#pragma once
#include "arm_bind.h"
// Original idle attachment measurements. Reproduce with prepare_dual_hand_reference.py.
namespace dualhandreference {
inline constexpr armbind::Mat Left{{
    {-0.412792033f,0.256950999f,0.87383003f,0.0f},
    {-0.107820095f,0.938851948f,-0.327004352f,0.0f},
    {-0.904421121f,-0.229201228f,-0.359846124f,0.0f},
    {14.9065728f,-130.894609f,36.0628297f,1.0f},
}};
inline constexpr armbind::Mat Right5799{{
    {-0.993376104f,0.0200494532f,0.113145636f,0.0f},
    {-0.114855294f,-0.143344392f,-0.982985578f,0.0f},
    {-0.00348953099f,-0.989469759f,0.144697679f,0.0f},
    {-12.0333688f,-136.262203f,17.4954986f,1.0f},
}};
inline constexpr armbind::Mat Right11387{{
    {-0.981458439f,-0.107842573f,0.158459183f,0.0f},
    {-0.148152023f,-0.0977215785f,-0.984124723f,0.0f},
    {0.121615424f,-0.989353563f,0.0799325725f,0.0f},
    {-11.3308694f,-132.706035f,12.2282921f,1.0f},
}};
inline constexpr armbind::Mat Right8297{{
    {-0.9832944f,-0.106136235f,0.147875698f,0.0f},
    {-0.134199995f,-0.126139133f,-0.982893321f,0.0f},
    {0.122973509f,-0.986318416f,0.109788421f,0.0f},
    {-12.631815f,-132.017237f,3.81236883f,1.0f},
}};
inline constexpr armbind::Mat Right11389{{
    {-0.978534804f,-0.124175139f,0.164469367f,0.0f},
    {-0.150001444f,-0.118082881f,-0.981608884f,0.0f},
    {0.141312436f,-0.985209099f,0.0969217582f,0.0f},
    {-16.6280793f,-130.92082f,9.25689541f,1.0f},
}};
inline const armbind::Mat* Right(unsigned vertices) {
    switch(vertices) {
    case 5799: return &Right5799;
    case 11387: return &Right11387;
    case 8297: return &Right8297;
    case 11389: return &Right11389;
    default: return nullptr;
    }
}
}
