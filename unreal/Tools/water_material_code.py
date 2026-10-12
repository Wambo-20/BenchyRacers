"""Shader snippets; the large-wave spectrum is read from the physics header."""
import re

NOISE = r'''
struct SeaNoise {
 float hash(float2 p) {
  float3 q=frac(float3(p.x,p.y,p.x)*.1031);
  q+=dot(q,q.yzx+33.33);
  return frac((q.x+q.y)*q.z);
 }
 float value(float2 p) {
  float2 i=floor(p),f=frac(p); f=f*f*(3-2*f);
  return lerp(lerp(hash(i),hash(i+float2(1,0)),f.x),
              lerp(hash(i+float2(0,1)),hash(i+float2(1,1)),f.x),f.y);
 }
 float fbm(float2 p) {
  float total=0,weight=.55;
  for(int i=0;i<4;i++) {total+=value(p)*weight;p=float2(p.x*1.61-p.y*1.23,p.x*1.23+p.y*1.61)+float2(19.71,7.13);weight*=.48;}
  return total;
 }
};
SeaNoise noise;
'''

def sea_hlsl(root):
    text=(root/'Source/BenchyRacers/WaveMath.h').read_text()
    number=r'([-+0-9.eE]+)'
    spectrum=re.findall(r'\{\s*'+r'\s*,\s*'.join([number]*5)+r'\s*\}',text)
    assert len(spectrum)==12
    vectors=','.join('float4('+','.join(w[:4])+')' for w in spectrum)
    phases=','.join(w[4] for w in spectrum)
    return f'''
const float4 waves[12]={{{vectors}}};
const float phases[12]={{{phases}}};
float a=P.x*.00017+P.y*.00023+T*.08;
float b=-P.x*.00031+P.y*.00011-T*.11;
float warp=.55*sin(a)+.32*sin(b);
float wx=.55*cos(a)*.00017-.32*cos(b)*.00031;
float wy=.55*cos(a)*.00023+.32*cos(b)*.00011;
float h=0,dx=0,dy=0;
for(int i=0;i<12;i++) {{
 float4 w=waves[i];float e=P.x*.00012-P.y*.00009+T*.13+phases[i];
 float envelope=.78+.22*sin(e);
 float phase=dot(P.xy,w.xy)+T*w.z+phases[i]+warp;
 h+=sin(phase)*w.w*envelope;
 float c=cos(phase)*w.w*envelope,s=sin(phase)*w.w*.22*cos(e);
 dx+=c*(w.x+wx)+s*.00012;dy+=c*(w.y+wy)-s*.00009;
}}
'''
