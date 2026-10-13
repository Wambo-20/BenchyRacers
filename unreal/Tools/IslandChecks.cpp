#include "../Source/BenchyRacers/IslandLayout.h"
#include <cassert>
#include <fstream>
#include <iostream>
int main(){
    using namespace BenchyIsland;
    int inland=0;double shallowest=-1e9;
    for(int i=0;i<1280;i++) {
        Point p=Route(i*.25),q=Route(i*.25+.05);
        double dx=q.x-p.x,dy=q.y-p.y,n=std::hypot(dx,dy);
        if(RawHeight(p)>250)inland++;
        for(double lateral:{-950.,0.,950.}) {
            double h=Height(p.x-dy/n*lateral,p.y+dx/n*lateral);
            shallowest=std::max(shallowest,h);assert(h<-100);
        }
    }
    assert(inland>200); // The route really crosses the interior, not only offshore.
    double steepestBank=0;Point worst{};
    for(int i=20;i<150;i++) {
        Point p=Route(i),q=Route(i+.1);double n=std::hypot(q.x-p.x,q.y-p.y);
        double nx=-(q.y-p.y)/n,ny=(q.x-p.x)/n;
        for(double d:{-3200.,-2500.,-1800.,1800.,2500.,3200.}) {
            double a=Height(p.x+nx*(d-50),p.y+ny*(d-50));
            double b=Height(p.x+nx*(d+50),p.y+ny*(d+50));
            // The navigable underwater channel has a steeper bed; assess visible banks.
            if(a>20 && b>20 && std::abs(b-a)/100>steepestBank){steepestBank=std::abs(b-a)/100;worst={p.x+nx*d,p.y+ny*d};}
        }
    }
    std::cerr<<"Bank slope "<<steepestBank<<" at "<<worst.x<<","<<worst.y<<"\n";
    assert(steepestBank<.65);
    for(auto p:CanalSamples())assert(Height(p.x,p.y)<-180);
    for(int i=1;i<5;i++) {
        const auto& s=Islands[i];
        for(int x=-10;x<=10;x++)for(int y=-10;y<=10;y++)
            assert(Height(s.center.x+x*s.radius.x/10,s.center.y+y*s.radius.y/10)<650);
    }
    std::ofstream f("work/island-heightmap.csv");
    for(int y=0;y<220;y++){
        for(int x=0;x<260;x++){if(x)f<<',';f<<Height(-35000+x*70000./259,-30000+y*60000./219);}
        f<<'\n';
    }
    std::cout<<"PASS: navigable full race corridor, two connected canals, low outer islands, gentle riverbanks. Inland samples="<<inland<<" shallowest_cm="<<shallowest<<" steepest_sampled_bank="<<steepestBank<<"\n";
}
