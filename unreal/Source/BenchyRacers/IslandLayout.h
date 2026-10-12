#pragma once
#include <array>
#include <algorithm>
#include <cmath>

// Shared by the rendered terrain, navigable water channels and layout audit.
namespace BenchyIsland {
struct Point { double x,y; };
struct Island { Point center,radius; double height,phase; };
inline constexpr Island Islands[]={
    {{-1500,500},{17500,15000},1550,.3},
    {{-26000,-17000},{5700,3000},320,1.4},
    {{27000,15000},{6200,4100},420,2.7},
    {{24000,-20000},{5300,2900},280,4.1},
    {{-24500,18000},{4700,5200},370,5.5}};
inline constexpr Point Course[]={
    {-16300,-6500},{-11000,-4500},{-6500,-6500},{-1800,-3500},
    {-3300,2000},{1500,5000},{6500,3500},{11500,7500},
    {16800,9000},{21000,4000},{19500,-5000},{15500,-12500},
    {7000,-16500},{-1500,-17000},{-10500,-15200},{-18000,-11000}};
inline double Smooth(double a,double b,double x) {
    double t=std::clamp((x-a)/(b-a),0.,1.);return t*t*(3-2*t);
}
inline Point Spline(Point a,Point b,Point c,Point d,double t) {
    auto f=[&](double A,double B,double C,double D){return .5*(2*B+(-A+C)*t+(2*A-5*B+4*C-D)*t*t+(-A+3*B-3*C+D)*t*t*t);};
    return {f(a.x,b.x,c.x,d.x),f(a.y,b.y,c.y,d.y)};
}
inline Point Route(double index) {
    double v=std::fmod(index,320.);if(v<0)v+=320.;int i=int(v/20);double t=std::fmod(v,20.)/20.;
    return Spline(Course[(i+15)%16],Course[i],Course[(i+1)%16],Course[(i+2)%16],t);
}
inline double SegmentDistance2(Point p,Point a,Point b) {
    double dx=b.x-a.x,dy=b.y-a.y;
    double t=std::clamp(((p.x-a.x)*dx+(p.y-a.y)*dy)/std::max(1.,dx*dx+dy*dy),0.,1.);
    return std::pow(p.x-a.x-dx*t,2)+std::pow(p.y-a.y-dy*t,2);
}
inline const std::array<Point,81>& RouteSamples() {
    static const auto result=[] {std::array<Point,81> p{};for(int i=0;i<=80;i++)p[i]=Route(i*4.);return p;}();return result;
}
inline double RouteDistance(Point p) {
    double d=1e30;const auto& r=RouteSamples();for(int i=0;i<80;i++)d=std::min(d,SegmentDistance2(p,r[i],r[i+1]));return std::sqrt(d);
}
inline const std::array<Point,50>& CanalSamples() {
    static const auto result=[] {
        constexpr Point a[]={{-6500,-6500},{-4000,-10200},{1700,-9700},{5500,-4300},{6500,3500}};
        constexpr Point b[]={{-3300,2000},{-8500,6900},{-6000,11100},{1700,11200},{6500,3500}};
        std::array<Point,50> p{};
        for(int branch=0;branch<2;branch++)for(int j=0;j<25;j++) {
            const Point* c=branch?b:a;double u=j/6.;int i=std::min(3,int(u));double t=std::min(1.,u-i);
            p[branch*25+j]=Spline(c[std::max(0,i-1)],c[i],c[i+1],c[std::min(4,i+2)],t);
        }return p;
    }();return result;
}
inline double CanalDistance(Point p) {
    double d=1e30;const auto& c=CanalSamples();for(int i=0;i<49;i++){if(i==24)continue;d=std::min(d,SegmentDistance2(p,c[i],c[i+1]));}return std::sqrt(d);
}
inline double RawHeight(Point p) {
    double h=-650;
    for(int i=0;i<5;i++) {
        const auto& s=Islands[i];double x=(p.x-s.center.x)/s.radius.x,y=(p.y-s.center.y)/s.radius.y;
        double a=std::atan2(y,x),coast=1+.12*std::sin(a*3+s.phase)+.06*std::sin(a*5-s.phase*2)+.025*std::sin(a*9+.4);
        double inland=1-std::sqrt(x*x+y*y)/coast;
        double z=inland<0?-100+inland*2400:-100+s.height*std::pow(inland,.95);
        if(inland>0) {
            z+=Smooth(0,.25,inland)*(90*std::sin(p.x*.0007+s.phase)*std::cos(p.y*.0005)+45*std::sin(p.x*.0011+p.y*.0008));
            if(i==0)z+=1250*std::exp(-std::pow((p.x+8700)/5000,2)-std::pow((p.y-8300)/4200,2));
        }
        h=std::max(h,z);
    }return h;
}
inline double Height(double x,double y) {
    Point p{x,y};double h=RawHeight(p);
    double d=RouteDistance(p),half=1350+180*std::sin(x*.00021+y*.00013)
        +140*std::sin(x*.0013+y*.0007)+75*std::sin(y*.0021-x*.0017);
    double bed=-310+25*std::sin(x*.0008)*std::cos(y*.0007);
    double bank=Smooth(half,half+1600,d);
    h=std::min(h,bed+(std::max(h,bed)-bed)*bank);
    // Broad planted floodplain: avoid carving tall, smooth walls into the river.
    double shelf=-110+std::max(0.,d-half)*.22;
    h=std::min(h,shelf+std::max(0.,h-shelf)*Smooth(4200,6500,d));
    double branch=CanalDistance(p),fade=Smooth(650,1900,branch);
    h=std::min(h,-230+(std::max(h,-230.)+230)*fade);
    double canalShelf=-90+std::max(0.,branch-650)*.23;
    h=std::min(h,canalShelf+std::max(0.,h-canalShelf)*Smooth(2800,4500,branch));
    return h;
}
}
