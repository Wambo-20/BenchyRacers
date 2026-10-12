#pragma once
#include "TrackLayout.h"
#include "RaceRules.h"
#include <vector>

namespace BenchyCourse {
using Point=BenchyIsland::Point;
constexpr double HalfWidth=1100.;
constexpr double GateAllowance=150.; // Allow the hull to overlap the marked edge.
inline Point Sub(Point A,Point B){return {A.x-B.x,A.y-B.y};}
inline double Dot(Point A,Point B){return A.x*B.x+A.y*B.y;}
inline Point Direction(int Track,double Index){
    Point D=Sub(BenchyTracks::Route(Track,Index+.1),BenchyTracks::Route(Track,Index-.1));
    double N=std::hypot(D.x,D.y);return {D.x/N,D.y/N};
}
inline const std::array<Point,1281>& Samples(int Track){
    static const auto All=[] {std::array<std::array<Point,1281>,3> A{};for(int T=0;T<3;T++)for(int I=0;I<=1280;I++)A[T][I]=BenchyTracks::Route(T,I*.25);return A;}();
    return All[Track];
}
struct Projection {double Fraction=0,Index=0,Distance=0;};
inline Projection ProjectSector(int Track,int Passed,Point Position){
    const int Start=(Passed%16)*80;const auto& Points=Samples(Track);
    double Best=1e30,Along=0,Length=0,Index=Start*.25;
    for(int I=Start;I<Start+80;I++){
        Point D=Sub(Points[I+1],Points[I]),Q=Sub(Position,Points[I]);
        double L2=Dot(D,D),L=std::sqrt(L2),U=std::clamp(Dot(Q,D)/std::max(1.,L2),0.,1.);
        Point Offset={Q.x-D.x*U,Q.y-D.y*U};double Dist2=Dot(Offset,Offset);
        if(Dist2<Best){Best=Dist2;Along=Length+U*L;Index=(I+U)*.25;}
        Length+=L;
    }
    return {std::clamp(Along/std::max(1.,Length),0.,.9999),Index,std::sqrt(Best)};
}
enum class Crossing { None, Valid, Outside };
inline bool BeyondPendingGate(int Track,int Passed,Point Position){
    int Next=BenchyRules::Next(Passed);Point Delta=Sub(Position,BenchyTracks::Route(Track,Next*20.));
    return Dot(Delta,Direction(Track,Next*20.))>300 && ProjectSector(Track,Passed,Position).Fraction>.995;
}
inline Crossing CrossCheckpoint(int Track,int Next,Point Previous,Point Position){
    Point Gate=BenchyTracks::Route(Track,Next*20.),F=Direction(Track,Next*20.);
    Point A=Sub(Previous,Gate),B=Sub(Position,Gate);double Before=Dot(A,F),After=Dot(B,F);
    // A swept, forward crossing counts across the complete marked lane, even
    // if one frame jumps over the line. Backward crossings never advance a lap.
    if(Before>=0||After<0)return Crossing::None;
    double U=-Before/(After-Before);Point At={A.x+(B.x-A.x)*U,A.y+(B.y-A.y)*U};
    return std::abs(-At.x*F.y+At.y*F.x)<=HalfWidth+GateAllowance?Crossing::Valid:Crossing::Outside;
}
inline std::vector<double> MarkerIndices(int Track){
    std::vector<double> Result;
    const auto& P=Samples(Track);
    for(int Sector=0;Sector<16;Sector++){
        int Begin=Sector*80,End=Begin+80;double Length=0;
        for(int I=Begin;I<End;I++){auto D=Sub(P[I+1],P[I]);Length+=std::hypot(D.x,D.y);}
        // Keep every checkpoint legible, but let the riverbanks do most of the
        // guiding inside channels. Extra pairs belong on broad water and bends.
        const auto Mid=BenchyTracks::Route(Track,Sector*20.+10),MidDirection=Direction(Track,Sector*20.+10);
        bool OpenWater=true;
        for(double Side:{-3100.,3100.})OpenWater&=BenchyTracks::Height(Track,Mid.x-MidDirection.y*Side,Mid.y+MidDirection.x*Side)<-50;
        const auto Entry=Direction(Track,Sector*20.+4),Exit=Direction(Track,Sector*20.+16);
        const bool Bend=Dot(Entry,Exit)<std::cos(.65);
        int Count=OpenWater?std::max(1,int(std::ceil(Length/6500.))):1;
        if(Bend&&Length>6500)Count=std::max(Count,2);
        double Spacing=Length/Count,Travel=0;int Added=0;
        for(int I=Begin;I<End&&Added<Count;I++){
            auto D=Sub(P[I+1],P[I]);double L=std::hypot(D.x,D.y);
            while(Added<Count&&Added*Spacing<=Travel+L){Result.push_back((I+(Added*Spacing-Travel)/L)*.25);Added++;}
            Travel+=L;
        }
    }
    return Result;
}
}
