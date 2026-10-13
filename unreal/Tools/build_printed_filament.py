"""Object-locked FDM layers, sparse coloured luminous stars, and phosphor glow."""
import sys
import struct
import zlib
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
blank='/Game/Materials/T_LogoBlank'
if not ue.load_asset(blank):
    png=Path(ue.Paths.project_saved_dir())/'LogoBlank.png'
    def chunk(kind,data):
        return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data)&0xffffffff)
    png.write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',1,1,8,6,0,0,0))+chunk(b'IDAT',zlib.compress(b'\x00\x00\x00\x00\x00'))+chunk(b'IEND',b''))
    task=ue.AssetImportTask();task.filename=str(png);task.destination_path='/Game/Materials';task.destination_name='T_LogoBlank';task.automated=True;task.save=True
    ue.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
logo_image=expression(m,ue.MaterialExpressionTextureSampleParameter2D,parameter_name='LogoImage',texture=ue.load_asset(blank))
logo_enabled=expression(m,ue.MaterialExpressionScalarParameter,parameter_name='LogoEnabled',default_value=0.)
logo_side=expression(m,ue.MaterialExpressionScalarParameter,parameter_name='LogoSide',default_value=0.)
logo_aspect=expression(m,ue.MaterialExpressionScalarParameter,parameter_name='LogoAspect',default_value=1.)
logo_min=expression(m,ue.MaterialExpressionVectorParameter,parameter_name='LogoMin',default_value=ue.LinearColor(-1,-1,-1,1))
logo_max=expression(m,ue.MaterialExpressionVectorParameter,parameter_name='LogoMax',default_value=ue.LinearColor(1,1,1,1))
logo_place=expression(m,ue.MaterialExpressionVectorParameter,parameter_name='LogoPlace',default_value=ue.LinearColor(.5,.75,.34,0))
logo_uv=custom(m,'''
float3 size=max(Max-Min,float3(.0001,.0001,.0001));float3 q=(L-Min)/size;
if(Side>4.5){
 bool rear=LN.x<-.45&&q.x<.13;bool deck=LN.z>.55;
 float2 p=rear?float2(q.y,1-q.z):deck?float2(q.x,1-q.y):LN.y<0?float2(1-q.x,1-q.z):float2(q.x,1-q.z);
 float2 tile=rear?float2(1,1):deck?float2(0,1):LN.y<0?float2(0,0):float2(1,0);
 return (clamp(p,.001,.999)+tile)*.5;
}
bool top=Side>1.5&&Side<2.5;bool right=(Side>.5&&Side<1.5)||(Side>2.5&&LN.y>0);
float2 uv=top?float2(q.x,1-q.y):float2(q.x,1-q.z);
float2 d=(uv-Place.xy)*float2(size.x,top?size.y:size.z);if(!top&&!right)d.x=-d.x;
float c=cos(Place.a),s=sin(Place.a);d=float2(c*d.x+s*d.y,-s*d.x+c*d.y);
float width=max(.0001,Place.z*size.x);
return d/float2(width,width/max(.001,Aspect))+.5;
''',{'L':local,'LN':local_normal,'Min':logo_min,'Max':logo_max,'Side':logo_side,'Place':(logo_place,'RGBA'),'Aspect':logo_aspect},ue.CustomMaterialOutputType.CMOT_FLOAT2)
connect(logo_uv,logo_image,'UVs')
logo_mask=custom(m,'''
if(Side>4.5){
 float3 q=(L-Min)/max(Max-Min,float3(.0001,.0001,.0001));
 bool rear=LN.x<-.45&&q.x<.13;bool deck=LN.z>.55;
 float outward=step(0,(q.y-.5)*LN.y)*step(.18,abs(q.y-.5));
 float face=rear||deck?1:step(.45,abs(LN.y))*outward;
 return saturate(A)*Enabled*face;
}
bool top=Side>1.5&&Side<2.5;float facing=top?LN.z:Side>2.5?abs(LN.y):Side<.5?-LN.y:LN.y;
float qy=(L.y-Min.y)/max(.0001,Max.y-Min.y);
float outward=Side>2.5?step(0,(qy-.5)*LN.y):Side<.5?step(qy,.5):step(.5,qy);
float depth=abs(qy-.5);float surface=step(Place.y>.6?.26:.08,depth);
float inside=step(0,UV.x)*step(UV.x,1)*step(0,UV.y)*step(UV.y,1);
return saturate(A)*Enabled*step(.45,facing)*inside*(top?1:outward*surface);
''',{'A':(logo_image,'A'),'Enabled':logo_enabled,'Side':logo_side,'UV':logo_uv,'LN':local_normal,'L':local,'Min':logo_min,'Max':logo_max,'Place':(logo_place,'RGBA')},ue.CustomMaterialOutputType.CMOT_FLOAT1)
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
return lerp(C,Logo,Mask)*(1-bands);
''',{'L':local,'Pitch':pitch,'Printed':printed,'C':color,'Logo':(logo_image,'RGB'),'Mask':logo_mask}),m,ue.MaterialProperty.MP_BASE_COLOR)
result(custom(m,'return lerp(.62+Printed*.09,.24,F.a);',{'Printed':printed,'F':flake},ue.CustomMaterialOutputType.CMOT_FLOAT1),m,ue.MaterialProperty.MP_ROUGHNESS)
result(custom(m,'return F.a*.35;',{'F':flake},ue.CustomMaterialOutputType.CMOT_FLOAT1),m,ue.MaterialProperty.MP_METALLIC)
result(scalar(m,.4),m,ue.MaterialProperty.MP_SPECULAR)
result(custom(m,'''
float z=L.z/max(Pitch,.00001);
float layers=1-.12*cos(z*6.283185)*(1-smoothstep(.65,1.8,fwidth(z)));
float form=.65+.35*saturate(dot(normalize(N),normalize(V)));
return (F.rgb*120000 + GlowColor*Glow*400*layers*form)*(1-Mask);
''',{'L':local,'Pitch':pitch,'N':normal,'V':view,'T':time,'F':flake,'Glow':glow,'GlowColor':glow_color,'Mask':logo_mask}),m,ue.MaterialProperty.MP_EMISSIVE_COLOR)
finish(m)
ue.log('BENCHY_PRINTED_FILAMENT_READY')
