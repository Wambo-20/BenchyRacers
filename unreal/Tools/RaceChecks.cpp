#include "../Source/BenchyRacers/TrackLayout.h"
#include "../Source/BenchyRacers/RaceRules.h"
#include "../Source/BenchyRacers/CourseNavigation.h"
#include <cassert>
#include <iostream>
int main(){
 for(int t=0;t<3;t++){
  double shallowest=-1e9;
  for(int i=0;i<1280;i++){
   auto p=BenchyTracks::Route(t,i*.25),q=BenchyTracks::Route(t,i*.25+.1);double dx=q.x-p.x,dy=q.y-p.y,n=std::hypot(dx,dy);
   assert(n>0);
   for(double side:{-900.,0.,900.}){double h=BenchyTracks::Height(t,p.x-dy/n*side,p.y+dx/n*side);shallowest=std::max(shallowest,h);assert(h<-100);}
  }
  std::cout<<"Track "<<t<<": full navigable corridor, shallowest="<<shallowest<<"\n";
  const auto markers=BenchyCourse::MarkerIndices(t);assert(markers.size()>=16&&markers.size()<=40);
  for(int gate=0;gate<16;gate++)assert(std::any_of(markers.begin(),markers.end(),[&](double index){return std::abs(index-gate*20.)<.001;}));
  for(double index:markers){auto p=BenchyTracks::Route(t,index),d=BenchyCourse::Direction(t,index);
   for(double side:{-BenchyCourse::HalfWidth,BenchyCourse::HalfWidth})assert(BenchyTracks::Height(t,p.x-d.y*side,p.y+d.x*side)<-25);
  }
  for(int sector=0;sector<16;sector++){
   auto g=BenchyTracks::Route(t,sector*20),d=BenchyCourse::Direction(t,sector*20);
   auto position=[&](double along,double side){return BenchyCourse::Point{g.x+d.x*along-d.y*side,g.y+d.y*along+d.x*side};};
   for(double side:{-1100.,0.,1100.}){
    auto a=position(-1400,side),b=position(1400,side);
    assert(BenchyCourse::CrossCheckpoint(t,sector,a,b)==BenchyCourse::Crossing::Valid);
    assert(BenchyCourse::CrossCheckpoint(t,sector,b,a)==BenchyCourse::Crossing::None);
    assert(BenchyCourse::CrossCheckpoint(t,sector,b,b)==BenchyCourse::Crossing::None);
   }
   assert(BenchyCourse::CrossCheckpoint(t,sector,position(-900,1500),position(900,1500))==BenchyCourse::Crossing::Outside);
   double last=-1;
   for(int i=0;i<80;i++){
    auto p=BenchyTracks::Route(t,sector*20+i*.25);auto projection=BenchyCourse::ProjectSector(t,sector,p);
    assert(projection.Fraction>=last);assert(projection.Distance<.001);last=projection.Fraction;
    assert(!BenchyCourse::BeyondPendingGate(t,sector,p));
   }
   auto near=BenchyTracks::Route(t,sector*20+8),ahead=BenchyTracks::Route(t,sector*20+12);
   assert(BenchyCourse::ProjectSector(t,sector,ahead).Fraction>BenchyCourse::ProjectSector(t,sector,near).Fraction);
  }
  // Ordered sweeps over three complete laps, including checkpoint zero at the seam.
  int passed=0;auto previous=BenchyTracks::Route(t,0);
  for(int i=1;i<=3841;i++){
   auto p=BenchyTracks::Route(t,i*.25);
   if(!BenchyRules::Finished(passed)&&BenchyCourse::CrossCheckpoint(t,BenchyRules::Next(passed),previous,p)==BenchyCourse::Crossing::Valid)passed++;
   previous=p;
  }
  assert(passed==48);
  std::cout<<"Track "<<t<<": "<<markers.size()*2<<" water buoys, swept full-width crossings, curve ranking and three ordered laps PASS\n";
 }
 for(int passed=0;passed<=48;passed++){
  assert(BenchyRules::Next(passed)==(passed+1)%16);assert(BenchyRules::Last(passed)==passed%16);
  assert(BenchyRules::Lap(passed)==std::min(3,passed/16+1));assert(BenchyRules::Finished(passed)==(passed==48));
 }
 assert(BenchyRules::CoinSpeed(0)==1);assert(BenchyRules::CoinSpeed(10)>1.059f&&BenchyRules::CoinSpeed(10)<1.061f);
 assert(BenchyRules::CoinSpeed(100)==BenchyRules::CoinSpeed(10));
 std::cout<<"PASS: three tracks, checkpoint order, reset target, three laps, capped hidden coin bonus.\n";
}
