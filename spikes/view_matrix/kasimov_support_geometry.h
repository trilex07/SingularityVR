#pragma once
// Original PSK + idle PSA; reproduce with audit_kasimov_attachment.py.
#include "seeker_geometry.h"
namespace kasimovsupport {
inline constexpr armbind::Mat RootBind{{
    {1.0000000000e+00f,3.6939837494e-08f,-5.0040912133e-08f,0.0000000000e+00f},
    {-3.6939838123e-08f,1.0000000000e+00f,-1.5282118721e-08f,0.0000000000e+00f},
    {5.0040911709e-08f,1.5282120488e-08f,1.0000000000e+00f,0.0000000000e+00f},
    {2.6627746692e-06f,-1.0949099736e+02f,4.4293211146e+01f,1.0000000000e+00f}}};
inline constexpr armbind::Mat LeftGrip{{
    {2.2896457492e-01f,-9.3171793289e-01f,-2.8191650707e-01f,7.6871732042e-19f},
    {8.1024990567e-01f,3.4292530426e-01f,-4.7528657256e-01f,-2.8293178835e-19f},
    {5.3950932688e-01f,-1.1959903520e-01f,8.3344211376e-01f,9.8675625553e-20f},
    {5.8963259862e+00f,3.2286554448e+00f,2.1790291212e+01f,1.0000000000e+00f}}};
// Separate native lens section, same displayed cog frame as the receiver.
inline bool LensPalette(const armbind::Mat& bodyRootLocal,const armbind::Mat& lensLocal,
                        armbind::Mat& out){
    armbind::Mat inverseBind,inverseLocal;
    if(!armbind::Finite(bodyRootLocal)||!armbind::Finite(lensLocal)||
       !skeletal::Inverse(inverseBind.m,RootBind.m)||!skeletal::Inverse(inverseLocal.m,lensLocal.m))return false;
    out=seekergeometry::Product(seekergeometry::Product(inverseBind,bodyRootLocal),inverseLocal);
    return armbind::Finite(out);
}
}
