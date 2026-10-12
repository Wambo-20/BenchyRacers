"""Object-locked FDM layers, sparse coloured luminous stars, and phosphor glow."""
import sys
from pathlib import Path
import unreal as ue
sys.path.insert(0, str(Path(ue.Paths.project_dir()) / 'Tools'))
from bootstrap_unreal import material, expression, connect, result, custom, scalar, finish

m=material('M_Filament')
m.set_editor_property('blend_mode',ue.BlendMode.BLEND_MASKED)
m.set_editor_property('tangent_space_normal',False)
m.set_editor_property('used_with_instanced_static_meshes',True)
m.set_editor_property('used_with_nanite',True)
color=expression(m,ue.MaterialExpressionVectorParameter,parameter_name='FilamentColor',default_value=ue.LinearColor(1,.3,.025,1))
printed=expression(m,ue.MaterialExpressionScalarParameter,parameter_name='Printed',default_value=0.)
pitch=expression(m,ue.MaterialExpressionScalarParameter,parameter_name='LayerPitch',default_value=1.)
galaxy=expression(m,ue.MaterialExpressionScalarParameter,parameter_name='Galaxy',default_value=0.)
glow=expression(m,ue.MaterialExpressionScalarParameter,parameter_name='Glow',default_value=0.)
glow_color=expression(m,ue.MaterialExpressionVectorParameter,parameter_name='GlowColor',default_value=ue.LinearColor(.1,.75,.25,1))
progress=expression(m,ue.MaterialExpressionScalarParameter,parameter_name='PrintProgress',default_value=1.)
bottom=expression(m,ue.MaterialExpressionScalarParameter,parameter_name='PrintMin',default_value=-100.)
top=expression(m,ue.MaterialExpressionScalarParameter,parameter_name='PrintMax',default_value=100.)
raw_local=expression(m,ue.MaterialExpressionPreSkinnedPosition)
raw_normal=expression(m,ue.MaterialExpressionPreSkinnedNormal)
local=expression(m,ue.MaterialExpressionVertexInterpolator)
local_normal=expression(m,ue.MaterialExpressionVertexInterpolator)
connect(raw_local,local)
connect(raw_normal,local_normal)
result(custom(m,'return Progress>=.9999 ? 1 : step(L.z,lerp(Bottom,Top,Progress));',
    {'L':local,'Progress':progress,'Bottom':bottom,'Top':top},ue.CustomMaterialOutputType.CMOT_FLOAT1),m,ue.MaterialProperty.MP_OPACITY_MASK)
world=expression(m,ue.MaterialExpressionWorldPosition)
normal=expression(m,ue.MaterialExpressionVertexNormalWS)
view=expression(m,ue.MaterialExpressionCameraVectorWS)
time=expression(m,ue.MaterialExpressionTime)

layer_code='''
float p=max(Pitch,.00001);
float z=L.z/p, top=(L.x+L.y*.18)/p;
float az=1-smoothstep(.65,1.8,fwidth(z));
float at=1-smoothstep(.65,1.8,fwidth(top));
float t=smoothstep(.65,.95,abs(LN.z));
return Printed*.12*lerp(cos(z*6.283185)*az,cos(top*6.283185)*at*.3,t);
'''
height=custom(m,layer_code,{'L':local,'LN':local_normal,'Pitch':pitch,'Printed':printed},ue.CustomMaterialOutputType.CMOT_FLOAT1)
result(custom(m,'''
float3 dx=ddx(P),dy=ddy(P),n=normalize(N);
float3 r1=cross(dy,n),r2=cross(n,dx);
float det=dot(dx,r1);
return normalize(abs(det)*n-sign(det)*(ddx(H)*r1+ddy(H)*r2)+n*.000001);
''',{'P':world,'N':normal,'H':height}),m,ue.MaterialProperty.MP_NORMAL)

flake=custom(m,'''
struct Stars {
 float hash(float2 p){return frac(sin(dot(p,float2(127.1,311.7)))*43758.5453);}
 float4 spot(float2 uv,float seed,float3 n,float3 v,float t){
  float2 cell=floor(uv),pos=frac(uv);
  cell+=seed;
  float h=hash(cell),h2=hash(cell+41.7);
  float2 c=.2+.6*float2(hash(cell+17.2),hash(cell+63.8));
  float radius=.065+.035*h2,aa=max(.018,max(fwidth(uv.x),fwidth(uv.y))*.55);
  // Keep tiny stars antialiased without expanding them into pale grey blotches.
  float size=max(radius,aa),d=length(pos-c);
  float core=(1-smoothstep(size*.25,size+aa,d))*pow(radius/size,2)*step(.98,h);
  float hue=hash(cell+83.1);
  float3 tint=hue<.55 ? float3(.52,.76,1) : (hue<.86 ? float3(.85,.38,1) : float3(1,.72,.36));
  float3 micro=normalize(n+float3(sin(h2*74.1),sin(h2*103.7),cos(h2*69.5))*.38);
  float glint=pow(saturate(dot(micro,normalize(v+float3(.25,-.35,.9)))),18);
  float twinkle=pow(.5+.5*sin(t*(1.7+h2*2.2)+hue*628.3),6);
  return float4(tint*core*(.22+.78*twinkle)*(.45+1.4*glint),core);
 }
};
Stars f;float3 q=L/max(Pitch,.00001)*.30;
float3 w=pow(abs(LN),6);w/=max(.001,w.x+w.y+w.z);
float footprint=max(max(fwidth(q.x),fwidth(q.y)),fwidth(q.z));
return (f.spot(q.yz,0,N,V,T)*w.x+f.spot(q.xz,97.3,N,V,T)*w.y+f.spot(q.xy,213.7,N,V,T)*w.z)*(1-smoothstep(1.2,2.5,footprint))*G;
''',{'L':local,'LN':local_normal,'Pitch':pitch,'G':galaxy,'N':normal,'V':view,'T':time},ue.CustomMaterialOutputType.CMOT_FLOAT4)
result(custom(m,'''
float z=L.z/max(Pitch,.00001);
float bands=Printed*.045*cos(z*6.283185)*(1-smoothstep(.65,1.8,fwidth(z)));
return C*(1-bands);
''',{'L':local,'Pitch':pitch,'Printed':printed,'C':color}),m,ue.MaterialProperty.MP_BASE_COLOR)
result(custom(m,'return lerp(.62+Printed*.09,.24,F.a);',{'Printed':printed,'F':flake},ue.CustomMaterialOutputType.CMOT_FLOAT1),m,ue.MaterialProperty.MP_ROUGHNESS)
result(custom(m,'return F.a*.35;',{'F':flake},ue.CustomMaterialOutputType.CMOT_FLOAT1),m,ue.MaterialProperty.MP_METALLIC)
result(scalar(m,.4),m,ue.MaterialProperty.MP_SPECULAR)
result(custom(m,'''
float z=L.z/max(Pitch,.00001);
float layers=1-.12*cos(z*6.283185)*(1-smoothstep(.65,1.8,fwidth(z)));
float form=.65+.35*saturate(dot(normalize(N),normalize(V)));
return F.rgb*120000 + GlowColor*Glow*400*layers*form;
''',{'L':local,'Pitch':pitch,'N':normal,'V':view,'T':time,'F':flake,'Glow':glow,'GlowColor':glow_color}),m,ue.MaterialProperty.MP_EMISSIVE_COLOR)
finish(m)
ue.log('BENCHY_PRINTED_FILAMENT_READY')
