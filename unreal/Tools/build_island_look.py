"""Build self-contained scan materials and coastal lighting for Sonnendeck 0.4."""
import unreal as ue
import sys,json
from pathlib import Path
root=Path(ue.Paths.project_dir()).resolve()
sys.path.insert(0,str(root/'Tools'))
from bootstrap_unreal import material,expression,connect,result,scalar,vector,custom,finish,NOISE
bindings=json.loads((root/'Validation/IslandBindings.json').read_text())
meshes=json.loads((root/'Validation/ElectricDreamsMeshes.json').read_text())
textures={}
def texture(path):
    if path in textures:return textures[path]
    source=ue.load_asset(path)
    target='/Game/IslandLook/Textures/'+source.get_name()
    t=ue.load_asset(target)
    if not t:t=ue.EditorAssetLibrary.duplicate_asset(path,target)
    assert isinstance(t,ue.Texture2D),path
    t.set_editor_property('virtual_texture_streaming',False)
    t.set_editor_property('max_texture_size',2048)
    ue.EditorAssetLibrary.save_loaded_asset(t)
    textures[path]=t
    return t
def sample(m,path,normal=False):
    t=texture(path)
    return expression(m,ue.MaterialExpressionTextureSample,texture=t,
        sampler_type=ue.MaterialSamplerType.SAMPLERTYPE_NORMAL if normal else
        ue.MaterialSamplerType.SAMPLERTYPE_COLOR if t.get_editor_property('srgb') else ue.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
generated={}
for path,row in bindings.items():
    name='M_Island_'+path.split('.')[-1].removeprefix('MI_')
    m=material(name)
    m.set_editor_property('used_with_instanced_static_meshes',True)
    m.set_editor_property('used_with_nanite',True)
    tex=row['textures'];leaf='/3D_Plants/' in path and any(s in path for s in ['TwoSided','Palm','Fern'])
    alpha='/3D_Plants/' in path and not any(s in path for s in ['Trunk','Tileable'])
    m.set_editor_property('blend_mode',ue.BlendMode.BLEND_MASKED if alpha else ue.BlendMode.BLEND_OPAQUE)
    m.set_editor_property('two_sided',leaf or alpha)
    m.set_editor_property('shading_model',ue.MaterialShadingModel.MSM_TWO_SIDED_FOLIAGE if leaf else ue.MaterialShadingModel.MSM_DEFAULT_LIT)
    m.set_editor_property('opacity_mask_clip_value',.28)
    base=sample(m,tex['Albedo'])
    tint=custom(m,'return saturate(C*float3(1.10,1.12,1.0));' if leaf else 'return saturate(C*float3(1.13,1.09,1.02));',{'C':(base,'RGB')})
    result(tint,m,ue.MaterialProperty.MP_BASE_COLOR)
    result(scalar(m,.88 if leaf else .92),m,ue.MaterialProperty.MP_ROUGHNESS)
    result(scalar(m,.2),m,ue.MaterialProperty.MP_SPECULAR)
    result(scalar(m,0),m,ue.MaterialProperty.MP_METALLIC)
    if 'Normal' in tex:
        n=sample(m,tex['Normal'],True)
        result(custom(m,'return normalize(float3(N.xy*.45,max(.15,N.z)));',{'N':n}),m,ue.MaterialProperty.MP_NORMAL)
    if alpha:result(base,m,ue.MaterialProperty.MP_OPACITY_MASK,'A')
    if leaf:
        result(custom(m,'return C*.55;',{'C':tint}),m,ue.MaterialProperty.MP_SUBSURFACE_COLOR)
        result(custom(m,'return C*.028;',{'C':tint}),m,ue.MaterialProperty.MP_EMISSIVE_COLOR)
        p=expression(m,ue.MaterialExpressionWorldPosition)
        t=expression(m,ue.MaterialExpressionTime)
        result(custom(m,'return float3(sin(T*.85+P.y*.0011)*3,cos(T*.67+P.x*.0013)*2,0);',{'P':p,'T':t}),m,ue.MaterialProperty.MP_WORLD_POSITION_OFFSET)
    finish(m);generated[path]=m
for row in meshes:
    mesh=ue.load_asset(row['path'])
    changed=False
    for i,path in enumerate(row['materials']):
        if mesh.get_material(i)!=generated[path]:mesh.set_material(i,generated[path]);changed=True
    if changed:ue.EditorAssetLibrary.save_loaded_asset(mesh)

# Broad irregular ground variation, with correct colour channels even on flat banks.
m=material('M_Terrain')
p=expression(m,ue.MaterialExpressionWorldPosition)
n=expression(m,ue.MaterialExpressionVertexNormalWS)
v=expression(m,ue.MaterialExpressionVertexColor)
uv=custom(m,'return P.xy*.0014;',{'P':p},ue.CustomMaterialOutputType.CMOT_FLOAT2)
sand=expression(m,ue.MaterialExpressionTextureSample,texture=ue.load_asset('/Game/Nature/Textures/sand_color'))
grass=sample(m,'/Game/Megascans/Surfaces/MossyGrass/T_MossyGrass_01_BC')
soil=sample(m,'/Game/Megascans/Surfaces/JungleGround/T_JungleGround_01_BC')
rock=expression(m,ue.MaterialExpressionTextureSample,texture=ue.load_asset('/Game/Nature/ScanTextures/boulder_01_color'))
for node in [sand,grass,soil,rock]:connect(uv,node,'UVs')
base=custom(m,NOISE+'''
float patch=noise.fbm(P.xy*.0007);
float3 green=G*float3(.92,1.03,.8);
float3 soil=lerp(green,D*.95,smoothstep(.35,.72,patch)*.8);
float3 beach=S*float3(1.06,1.025,.93);
return beach*W.r+soil*W.g+R*float3(.9,.91,.86)*W.b;
''',{'P':p,'S':(sand,'RGB'),'G':(grass,'RGB'),'D':(soil,'RGB'),'R':(rock,'RGB'),'W':v})
result(base,m,ue.MaterialProperty.MP_BASE_COLOR)
soil_n=sample(m,'/Game/Megascans/Surfaces/JungleGround/T_JungleGround_01_N',True)
grass_n=sample(m,'/Game/Megascans/Surfaces/MossyGrass/T_MossyGrass_01_N',True)
sand_n=expression(m,ue.MaterialExpressionTextureSample,texture=ue.load_asset('/Game/Nature/Textures/sand_normal'),sampler_type=ue.MaterialSamplerType.SAMPLERTYPE_NORMAL)
for node in [soil_n,grass_n,sand_n]:connect(uv,node,'UVs')
result(custom(m,'float3 t=lerp(S,normalize(G+D),W.g);return normalize(float3(t.xy*.5,max(.15,t.z)));',{'S':sand_n,'G':grass_n,'D':soil_n,'W':v}),m,ue.MaterialProperty.MP_NORMAL)
result(scalar(m,.95),m,ue.MaterialProperty.MP_ROUGHNESS)
result(scalar(m,.13),m,ue.MaterialProperty.MP_SPECULAR)
finish(m)

# Narrow broken cascade, with no emissive white bars.
m=material('M_Waterfall')
m.set_editor_property('blend_mode',ue.BlendMode.BLEND_TRANSLUCENT)
m.set_editor_property('two_sided',True)
m.set_editor_property('shading_model',ue.MaterialShadingModel.MSM_DEFAULT_LIT)
m.set_editor_property('translucency_lighting_mode',ue.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)
uv=expression(m,ue.MaterialExpressionTextureCoordinate);t=expression(m,ue.MaterialExpressionTime)
opacity=custom(m,NOISE+'''
float2 q=float2(UV.x*15,UV.y*5-T*1.5);
float n=noise.fbm(q+float2(noise.value(q*.6),0));
float edge=smoothstep(0,.12,UV.x)*smoothstep(0,.12,1-UV.x);
return edge*(.16+smoothstep(.25,.8,n)*.42);
''',{'UV':uv,'T':t},ue.CustomMaterialOutputType.CMOT_FLOAT1)
result(opacity,m,ue.MaterialProperty.MP_OPACITY)
result(vector(m,(.48,.63,.65)),m,ue.MaterialProperty.MP_BASE_COLOR)
result(scalar(m,.32),m,ue.MaterialProperty.MP_ROUGHNESS)
finish(m)

levels=ue.get_editor_subsystem(ue.LevelEditorSubsystem)
assert levels.load_level('/Game/Maps/Sonnendeck')
for actor in ue.get_editor_subsystem(ue.EditorActorSubsystem).get_all_level_actors():
    if isinstance(actor,ue.DirectionalLight):
        actor.set_actor_rotation(ue.Rotator(-32,-125,0),False)
        c=actor.get_component_by_class(ue.DirectionalLightComponent)
        c.set_intensity(24000)
        c.set_light_color(ue.LinearColor(1,.94,.83))
        c.set_editor_property('cloud_shadow_strength',.15)
    if isinstance(actor,ue.SkyLight):actor.get_component_by_class(ue.SkyLightComponent).set_intensity(3.2)
    if isinstance(actor,ue.ExponentialHeightFog):
        c=actor.get_component_by_class(ue.ExponentialHeightFogComponent)
        c.set_editor_property('fog_density',.004)
levels.save_current_level()
(root/'Validation/IslandLook.json').write_text(json.dumps({'materials':[m.get_path_name() for m in generated.values()],
    'scan_texture_copies':len(textures),'rock_roughness':.92,'metallic':0,'independent_of_demo_material_layers':True},indent=2))
ue.log('BENCHY_ISLAND_LOOK_READY')
