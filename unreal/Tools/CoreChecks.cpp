#include "../Source/BenchyRacers/WaveMath.h"
#include <cassert>
#include <iostream>
int main() {
    for(double x=-20000;x<=20000;x+=1731) for(double y=-20000;y<=20000;y+=2307)
        for(double t=0;t<5;t+=.41) {
            double h=BenchySea::Height(x,y,t),dx,dy;
            assert(std::isfinite(h) && std::abs(h)<=60);
            BenchySea::Gradient(x,y,t,dx,dy);
            double eps=.01;
            assert(std::abs(dx-(BenchySea::Height(x+eps,y,t)-BenchySea::Height(x-eps,y,t))/(2*eps))<1e-6);
            assert(std::abs(dy-(BenchySea::Height(x,y+eps,t)-BenchySea::Height(x,y-eps,t))/(2*eps))<1e-6);
        }
    double mass=320,gravity=980;
    double force=4*mass*gravity*.25*BenchySea::BuoyancyFraction(-34,0);
    assert(std::abs(force-mass*gravity)<1e-8);
    assert(BenchySea::BuoyancyFraction(20,0)==0);
    assert(BenchySea::BuoyancyFraction(-500,0)==3);
    assert(BenchySea::BuoyancyFraction(-20,0)<1);
    assert(BenchySea::BuoyancyFraction(-50,0)>1);
    assert(std::abs(BenchySea::Height(100,200,0)-BenchySea::Height(100,200,1))>1);
    std::cout << "PASS: shared wave heights, analytic normals, moving waves, buoyancy equilibrium\n";
}
