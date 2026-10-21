#pragma once
#include <cstdint>
#include <cstring>

namespace nativeplayer {
inline bool ObjectField(const char* field,const char* type,int offset,int count,int size) {
    return field && type && count==1 && size==4 && offset>=0 && offset<=0x8000 && offset%4==0 &&
        !std::strcmp(type,!std::strcmp(field,"Mesh")?"ComponentProperty":"ObjectProperty");
}
enum class Stage { Camera, Owner, CameraReciprocal, Pawn, PawnType, PawnReciprocal, Ready };
struct Identity {
    std::uintptr_t controller=0, pawn=0;
    Stage stage=Stage::Camera;
};
inline const char* Name(Stage stage) {
    switch(stage) {
    case Stage::Camera:return "camera";
    case Stage::Owner:return "owner";
    case Stage::CameraReciprocal:return "camera-reciprocal";
    case Stage::Pawn:return "pawn";
    case Stage::PawnType:return "pawn-type";
    case Stage::PawnReciprocal:return "pawn-reciprocal";
    case Stage::Ready:return "ready";
    }
    return "unknown";
}
// No dependency on a previously discovered global pawn/controller. A camera
// owner can be valid while unpossessed; cinematic state belongs to that owner.
template<class Access> Identity Resolve(std::uintptr_t camera,Access& access) {
    Identity result;
    if(!access.IsCamera(camera))return result;
    result.stage=Stage::Owner;
    const auto controller=access.Link(camera,"PCOwner");
    if(!access.IsController(controller))return result;
    result.stage=Stage::CameraReciprocal;
    if(access.Link(controller,"PlayerCamera")!=camera)return result;
    result.controller=controller;
    result.stage=Stage::Pawn;
    const auto pawn=access.Link(controller,"Pawn");
    if(!pawn)return result;
    result.stage=Stage::PawnType;
    if(!access.IsPawn(pawn))return result;
    result.stage=Stage::PawnReciprocal;
    if(access.Link(pawn,"Controller")!=controller)return result;
    result.pawn=pawn;result.stage=Stage::Ready;
    return result;
}
}
