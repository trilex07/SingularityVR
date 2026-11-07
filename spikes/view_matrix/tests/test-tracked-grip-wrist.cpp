#include "../tracked_grip_wrist.h"
#include <cassert>
#include <cstdio>
#include <limits>
using armbind::Mat;
static Mat product(const Mat& a,const Mat& b){Mat r;skeletal::Multiply(r.m,a.m,b.m);return r;}
static Mat identity(){Mat r;for(unsigned j=0;j<4;++j)r.m[j][j]=1;return r;}
static void close(const Mat& a,const Mat& b){for(unsigned i=0;i<4;++i)for(unsigned j=0;j<4;++j)assert(std::fabs(a.m[i][j]-b.m[i][j])<.0003f);}
int main(){
 const float pi=3.14159265358979323846f;
 // Original LocalToWorld and VP forward from PID41912 frame11432:
 // local=[-1,0,0;0,0,-1;0,-1,0], view forward approximately [0,-1,0].
 const Mat observed{{{-1,0,0,0},{0,0,-1,0},{0,-1,0,0},{0,0,0,1}}};
 const Mat view=trackedgrip::NativeView(-pi/2,0);
 Mat iv;assert(skeletal::Inverse(iv.m,view.m));
 const Mat mount=product(observed,iv);
 for(unsigned vertices:{5799u,11387u,8297u,11389u})for(unsigned side=0;side<2;++side){
  const auto& reference=side?*dualhandreference::Right(vertices):dualhandreference::Left;
  // Neutral grip/head must preserve the original native wrist/gun orientation.
  // V2's fixed-world 'mount' rotated this exact recorded case an extra 90deg.
  gloves::Pose neutral{{.2f,1.3f,-.35f},{0,0,0,1}};Mat desired;
  assert(trackedgrip::Desired(side,vertices,neutral,{},0,-pi/2,70,1,observed,view,desired));
  Mat expected=product(reference,observed);
  for(unsigned j=0;j<3;++j)expected.m[3][j]=desired.m[3][j];
  close(desired,expected);
  // A changing camera must disappear from the target while a fixed physical
  // grip remains fixed. Include near-vertical head pitch and all body headings.
  for(int heading=-180;heading<=180;heading+=30)for(int pitch:{-89,-40,0,40,89})
  for(int axis=0;axis<3;++axis)for(int angle=-180;angle<=180;angle+=30){
   const float yaw=heading*pi/180,headPitch=pitch*pi/180,a=angle*pi/360;
   const Mat nativeView=trackedgrip::NativeView(yaw,headPitch);
   Mat component=product(mount,nativeView);component.m[3][0]=123;component.m[3][1]=-48;
   gloves::Pose grip{{-.2f,1.3f,-.35f},{0,0,0,std::cos(a)}};
   (&grip.orientation.x)[axis]=std::sin(a);
   Mat actual;
   assert(trackedgrip::Desired(side,vertices,grip,{.1f,1.6f,.2f},.7f,-1.1f,69.078949f,.75f,component,nativeView,actual));
   const Mat neutralMount=product(reference,mount);
   Mat oracle=identity();
   for(unsigned row=0;row<3;++row){
    const auto rotated=gloves::Rotate(grip.orientation,{neutralMount.m[row][1],neutralMount.m[row][2],-neutralMount.m[row][0]});
    const auto world=vrpose::TrackingDeltaToWorldUU({rotated.x,rotated.y,rotated.z},.7f,-1.1f,.75f);
    oracle.m[row][0]=world.x;oracle.m[row][1]=world.y;oracle.m[row][2]=world.z;
   }
   const auto pos=vrpose::TrackingDeltaToWorldUU({-.3f,-.3f,-.55f},.7f,-1.1f,69.078949f);
   oracle.m[3][0]=pos.x;oracle.m[3][1]=pos.y;oracle.m[3][2]=pos.z;close(actual,oracle);
   Mat source=product(reference,component);source.m[3][0]+=47;
   float correction[4][4];assert(trackedgrip::Target(source,actual,correction));
   Mat result;skeletal::Multiply(result.m,source.m,correction);close(result,actual);
  }
 }
 Mat out=identity();out.m[3][0]=123;gloves::Pose bad;bad.orientation={0,0,0,0};
 assert(!trackedgrip::Desired(0,11387,bad,{},0,0,70,1,observed,view,out));assert(out.m[3][0]==123);
 assert(!trackedgrip::Desired(2,11387,{}, {},0,0,70,1,observed,view,out));
 Mat singular{};float target[4][4]{};target[3][0]=123;
 assert(!trackedgrip::Target(singular,out,target));assert(target[3][0]==123);
 std::puts("PASS: recorded double-camera rotation regression, four native mounts, both hands, full quaternion, head/body/recenter invariance, source inversion");
}
