#pragma once
#include "IslandLayout.h"
namespace BenchyTracks {
using namespace BenchyIsland;
inline constexpr Point Harbor[]={
 {-18000,-12000},{-21000,-4500},{-17000,5500},{-10000,10500},
 {-2500,7000},{3200,10500},{12500,13000},{19500,7000},
 {17500,0},{11500,-1500},{10000,-8500},{17500,-14500},
 {9500,-20000},{-1500,-19500},{-7500,-13500},{-13000,-17500}};
inline constexpr Point Forest[]={
 {-19500,-12500},{-12500,-14000},{-7000,-9000},{-10500,-2000},
 {-6500,5300},{-11500,11200},{-2000,15300},{7600,13200},
 {13000,6500},{7000,1500},{11500,-4500},{19000,-10000},
 {14500,-17800},{4500,-18500},{-2000,-21800},{-14500,-21000}};
inline constexpr Point Fjord[]={
 {-19500,-14000},{-23000,-5500},{-17500,4300},{-11000,11800},
 {-4700,3500},{3300,9600},{13000,14900},{22500,5400},
 {16000,-1500},{7300,-4300},{3500,-11200},{16500,-15000},
 {9500,-21400},{-4500,-20800},{-11900,-12800},{-17500,-20800}};
inline constexpr Island HarborIslands[]={
 {{-4000,1500},{15200,14000},1700,.8},{{16500,4000},{8200,12000},1300,2.3},
 {{-25500,14500},{5000,4700},400,1.1},{{15000,-24500},{6500,3200},330,4.1},{{-23000,-23000},{6500,4300},450,5.1}};
inline constexpr Island ForestIslands[]={
 {{-2300,-500},{18000,16000},2400,2.1},{{23000,16000},{6500,6200},1100,3.2},
 {{-26000,14000},{5200,7100},900,1.6},{{17000,-24500},{7000,3400},450,4.7},{{-22000,-23000},{6000,3300},360,2.8}};
inline constexpr Island FjordIslands[]={
 {{-1500,2500},{18000,16300},6200,2.4},{{23500,15400},{6800,7200},4100,1.3},
 {{-26300,15300},{5800,6400},3300,4.3},{{16500,-24700},{7500,3400},750,3.7},{{-26000,-21500},{6600,4500},820,2.1}};
inline const Island* Land(int track){return track==1?HarborIslands:track==2?FjordIslands:Islands;}
inline Point Route(int track,double index){if(track==0)return BenchyIsland::Route(index);const Point* c=track==1?Harbor:Fjord;
 double v=std::fmod(index,320.);if(v<0)v+=320.;int i=int(v/20);return Spline(c[(i+15)%16],c[i],c[(i+1)%16],c[(i+2)%16],std::fmod(v,20.)/20.);}
inline const std::array<Point,81>& Samples(int track){
 static const auto all=[] {std::array<std::array<Point,81>,3> a{};for(int t=0;t<3;t++)for(int i=0;i<=80;i++)a[t][i]=Route(t,i*4.);return a;}();return all[track];}
inline double Distance(int track,Point p){double d=1e30;const auto& a=Samples(track);for(int i=0;i<80;i++)d=std::min(d,SegmentDistance2(p,a[i],a[i+1]));return std::sqrt(d);}
inline double Height(int track,double x,double y){if(track==0)return BenchyIsland::Height(x,y);double h=-650;
 for(int i=0;i<5;i++){const auto& s=Land(track)[i];double px=(x-s.center.x)/s.radius.x,py=(y-s.center.y)/s.radius.y;
  double a=std::atan2(py,px),inland=1-std::hypot(px,py)/(1+.11*std::sin(a*3+s.phase)+.07*std::sin(a*5-s.phase));
  double z=inland<0?-100+inland*2400:-100+s.height*std::pow(inland,1.1);
  if(inland>0)z+=Smooth(0,.25,inland)*(110*std::sin(x*.0006+s.phase)*std::cos(y*.0007)+65*std::sin(x*.001+y*.0008));h=std::max(h,z);
 }
 double d=Distance(track,{x,y}),half=1400+100*std::sin(x*.0005)+110*std::cos(y*.0008);
 double bed=-330+25*std::sin(x*.0006)*std::cos(y*.0009),bank=Smooth(half,half+1900,d);
 h=std::min(h,bed+(std::max(h,bed)-bed)*bank);
 double shelf=-110+std::max(0.,d-half)*.24;
 return std::min(h,shelf+std::max(0.,h-shelf)*Smooth(4400,7000,d));
}
}
