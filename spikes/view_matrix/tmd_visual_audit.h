#pragma once
#include <cstdint>
// Observation scheduler only: never alters tracking, materials or power state.
namespace tmdvisualaudit {
enum class Scene : unsigned { None=0, Neutral=1, Sky=2, GravityInput=3 };
inline const char* Name(Scene s) {
    switch(s){case Scene::Neutral:return "neutral";case Scene::Sky:return "sky";
        case Scene::GravityInput:return "gravity-input";default:return "none";}
}
struct Schedule {
    unsigned captured=0, stable=0; Scene candidate=Scene::None;
    std::uint64_t last=0;
    Scene Update(bool enabled,bool eligible,bool traceBusy,unsigned traceSequence,
                 float pitch,bool gravity,std::uint64_t now) {
        if(!enabled||!eligible||traceBusy||traceSequence>=4) {stable=0;candidate=Scene::None;return Scene::None;}
        // This threshold classifies a measurement, never a rendering correction.
        const Scene scene=gravity?Scene::GravityInput:(pitch>0.65f?Scene::Sky:Scene::Neutral);
        const unsigned bit=1u<<unsigned(scene);
        if((captured&bit)||(last&&now-last<4000)) {stable=0;candidate=Scene::None;return Scene::None;}
        if(scene!=candidate){candidate=scene;stable=0;}
        if(++stable<30)return Scene::None;
        captured|=bit;last=now;stable=0;candidate=Scene::None;return scene;
    }
};
}
