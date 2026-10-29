#include "../microstutter_probe.h"
#include <cassert>
#include <cstdio>
#include <cstring>

int main(int argc, char** argv) {
    assert(argc == 2);
    assert(microstutter::enabled == 0);
    microstutter::Mark(microstutter::Fire);
    assert(microstutter::pendingEvents == 0); // default-OFF no event capture
    const auto offStart=microstutter::Qpc();
    for(int i=0;i<100000;++i)microstutter::Mark(microstutter::Fire);
    const auto offTicks=microstutter::Qpc()-offStart;
    assert(microstutter::Start(argv[1]));
    const auto onStart=microstutter::Qpc();
    for(int i=0;i<100000;++i)microstutter::Mark(microstutter::Fire);
    const auto onTicks=microstutter::Qpc()-onStart;
    InterlockedExchange(&microstutter::pendingEvents,0);
    microstutter::Begin();
    microstutter::Mark(microstutter::Fire);
    const auto start=microstutter::Qpc();
    Sleep(1);
    microstutter::NativeDuration(microstutter::Fire,start);
    microstutter::Finish(500);
    Sleep(1);
    microstutter::Begin();
    microstutter::Mark(microstutter::Damage);
    microstutter::Finish(300);
    const auto framesStart=microstutter::Qpc();
    for(int i=0;i<128;++i){microstutter::Begin();microstutter::Finish(0);}
    const auto frameTicks=microstutter::Qpc()-framesStart;
    assert(microstutter::StopAndWait());
    assert(microstutter::ring[1].events == microstutter::Fire);
    assert(microstutter::ring[1].fireNativeMaxUs > 0);
    assert(microstutter::ring[2].events == microstutter::Damage);
    assert(microstutter::ring[2].frameWallUs > 0);
    FILE* f=nullptr;
    assert(fopen_s(&f,microstutter::outputPath,"r")==0 && f);
    char line[512];
    assert(fgets(line,sizeof(line),f) && strstr(line,"frame_wall_us"));
    assert(fgets(line,sizeof(line),f) && line[0]=='1');
    assert(fgets(line,sizeof(line),f) && line[0]=='2');
    bool sawLoss=false;
    while(fgets(line,sizeof(line),f))if(strstr(line,"lost_frames=0"))sawLoss=true;
    assert(sawLoss);
    fclose(f);
    std::remove(microstutter::outputPath);
    printf("microstutter probe: OFF mark %.3f us/call, ON mark %.3f us/call, synthetic frame %.2f us/call\n",
        microstutter::Us(offTicks)/100000.0,microstutter::Us(onTicks)/100000.0,
        microstutter::Us(frameTicks)/128.0);
    puts("microstutter probe: ring, worker flush, event duration, OFF gate PASS");
}
