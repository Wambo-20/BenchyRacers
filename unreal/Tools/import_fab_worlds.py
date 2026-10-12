"""Import the user's acquired Fab originals and create independent game materials.
Never changes the launcher cache. Does not regenerate filament, ocean or base maps.
"""
from pathlib import Path
import sys,json
import unreal as ue
root=Path(ue.Paths.project_dir()).resolve();sys.path.insert(0,str(root/'Tools'))
from bootstrap_unreal import material,expression,connect,result,custom,scalar,finish,NOISE
assets=ue.EditorAssetLibrary;tools=ue.AssetToolsHelpers.get_asset_tools()
library=Path('C:/ProgramData/Epic/EpicGamesLauncher/VaultCache/FabLibrary')
sources={'Snow':('6ac5c478',False),'Ice':('198d6561',True),'Nordic':('35d695d8',True),'Wood':('1f86b760',False),'Rust':('6e61dcd3',False)}
imported={};report={'source':'User-acquired Fab library originals; source assets not redistributed separately','assets':[]}
for name,(short_id,mesh) in sources.items():
    folder=next(library.glob('*-'+short_id))
    files={p.name.split('_')[-1].split('.')[0]:p for p in folder.rglob('*4K_*.jpg')}
    assert all(k in files for k in ['BaseColor','Normal','Roughness']),name
    imported[name]={}
    for key in ['BaseColor','Normal','Roughness','AO']:
        if key not in files:continue
        path=f'/Game/FabWorld/Textures/T_{name}_{key}';tex=ue.load_asset(path)
        if not tex:
            task=ue.AssetImportTask();task.filename=str(files[key]);task.destination_path='/Game/FabWorld/Textures';task.destination_name='T_'+name+'_'+key;task.automated=True;task.save=True
            tools.import_asset_tasks([task]);tex=ue.load_asset(path)
        assert tex,path
        tex.set_editor_property('srgb',key=='BaseColor');tex.set_editor_property('virtual_texture_streaming',False)
        tex.set_editor_property('max_texture_size',4096 if name in ['Snow','Nordic','Ice'] else 2048)
        if key=='Normal':tex.set_editor_property('compression_settings',ue.TextureCompressionSettings.TC_NORMALMAP)
        assets.save_loaded_asset(tex);imported[name][key]=tex
    if mesh:
        fbx=next(folder.rglob('*High.fbx'));path=f'/Game/FabWorld/Meshes/SM_{name}Cliff'
        model=ue.load_asset(path)
        if not model:
            task=ue.AssetImportTask();task.filename=str(fbx);task.destination_path='/Game/FabWorld/Meshes';task.destination_name='SM_'+name+'Cliff';task.automated=True;task.save=True
            opt=ue.FbxImportUI();opt.import_mesh=True;opt.import_as_skeletal=False;opt.import_materials=False;opt.import_textures=False
            opt.mesh_type_to_import=ue.FBXImportType.FBXIT_STATIC_MESH
            opt.static_mesh_import_data.combine_meshes=True;opt.static_mesh_import_data.auto_generate_collision=True
            opt.static_mesh_import_data.convert_scene=True;opt.static_mesh_import_data.convert_scene_unit=True
            task.options=opt;task.factory=ue.FbxFactory();tools.import_asset_tasks([task]);model=ue.load_asset(path)
        assert model,path
        settings=model.get_editor_property('nanite_settings');settings.set_editor_property('enabled',True);model.set_editor_property('nanite_settings',settings)
        imported[name]['mesh']=model
    report['assets'].append({'name':name,'textures':[t.get_path_name() for k,t in imported[name].items() if k!='mesh'],'mesh':imported[name]['mesh'].get_path_name() if mesh else None})

def sample(m,tex,uv=None):
    normal=tex.get_editor_property('compression_settings')==ue.TextureCompressionSettings.TC_NORMALMAP
    n=expression(m,ue.MaterialExpressionTextureSample,texture=tex,sampler_type=ue.MaterialSamplerType.SAMPLERTYPE_NORMAL if normal else ue.MaterialSamplerType.SAMPLERTYPE_COLOR if tex.get_editor_property('srgb') else ue.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
    if uv:connect(uv,n,'UVs')
    return n

def scan_material(name,tex,snow=False,ice=False,world_uv=False):
    m=material(name);m.set_editor_property('blend_mode',ue.BlendMode.BLEND_OPAQUE)
    m.set_editor_property('two_sided',name in ['M_Fab_Nordic','M_Fab_Ice'])
    m.set_editor_property('used_with_instanced_static_meshes',True);m.set_editor_property('used_with_nanite',True)
    p=expression(m,ue.MaterialExpressionWorldPosition);n=expression(m,ue.MaterialExpressionVertexNormalWS)
    amount=expression(m,ue.MaterialExpressionScalarParameter,parameter_name='SnowAmount',default_value=1. if snow else 0.)
    uv=custom(m,'float3 A=abs(N); return (A.z>.6 ? P.xy : A.x>A.y ? P.zy : P.xz)*.004;',{'P':p,'N':n},ue.CustomMaterialOutputType.CMOT_FLOAT2) if world_uv else None
    base=sample(m,tex['BaseColor'],uv);normal=sample(m,tex['Normal'],uv)
    color=custom(m,NOISE+'''
float mask=smoothstep(.35,.9,N.z)*Amount*(.8+.2*noise.fbm(P.xy*.001));
float3 body=C;
return lerp(body,float3(.78,.86,.91),mask);
''',{'C':(base,'RGB'),'P':p,'N':n,'Amount':amount})
    result(color,m,ue.MaterialProperty.MP_BASE_COLOR)
    result(custom(m,'return normalize(float3(N.xy*.55,max(.15,N.z)));',{'N':normal}),m,ue.MaterialProperty.MP_NORMAL)
    rough=sample(m,tex['Roughness'],uv)
    result(custom(m,'return clamp(R,.3,.9);' if ice else 'return max(.63,R);',{'R':(rough,'G' if name.startswith('M_Fab_Rock') else 'R')},ue.CustomMaterialOutputType.CMOT_FLOAT1),m,ue.MaterialProperty.MP_ROUGHNESS)
    result(scalar(m,.1 if name.endswith('Rust') else 0),m,ue.MaterialProperty.MP_METALLIC)
    result(scalar(m,.3 if ice else .22),m,ue.MaterialProperty.MP_SPECULAR)
    finish(m);return m

for name in ['Nordic','Ice','Wood','Rust']:
    m=scan_material('M_Fab_'+name,imported[name],ice=name=='Ice',world_uv=name in ['Wood','Rust'])
    if 'mesh' in imported[name]:
        mesh=imported[name]['mesh']
        for i in range(mesh.get_num_sections(0)):mesh.set_material(i,m)
        assets.save_loaded_asset(mesh)

for i in range(1,8):
    package=f'/Game/Rock_Collection_04/Meshes/Rock_{i:02d}'
    tex={}
    for key,suffix in [('BaseColor','A'),('Normal','N'),('Roughness','ORM')]:
        t=ue.load_asset(f'{package}/Textures/T_Rock_{i:02d}_{suffix}');assert t
        t.set_editor_property('max_texture_size',2048);t.set_editor_property('virtual_texture_streaming',False);assets.save_loaded_asset(t);tex[key]=t
    # The ORM texture's green channel carries roughness.
    m=scan_material(f'M_Fab_Rock{i:02d}',tex)
    mesh=ue.load_asset(f'{package}/StaticMeshes/SM_Rock_{i:02d}');assert mesh
    settings=mesh.get_editor_property('nanite_settings');settings.set_editor_property('enabled',True);mesh.set_editor_property('nanite_settings',settings)
    for s in range(mesh.get_num_sections(0)):mesh.set_material(s,m)
    assets.save_loaded_asset(mesh)
    report['assets'].append({'name':f'Rock{i:02d}','mesh':mesh.get_path_name(),'textures':[t.get_path_name() for t in tex.values()]})

m=material('M_FrostTerrain');p=expression(m,ue.MaterialExpressionWorldPosition);n=expression(m,ue.MaterialExpressionVertexNormalWS)
uv=custom(m,'return P.xy*.001;',{'P':p},ue.CustomMaterialOutputType.CMOT_FLOAT2)
snow=sample(m,imported['Snow']['BaseColor'],uv);rock=sample(m,imported['Nordic']['BaseColor'],uv)
result(custom(m,NOISE+'''
float cover=smoothstep(.42,.82,N.z)*smoothstep(-5,100,P.z);
float patch=.88+.12*noise.fbm(P.xy*.00035);
return lerp(R*.68,S*float3(.94,1,1.02)*patch,cover);
''',{'P':p,'N':n,'S':(snow,'RGB'),'R':(rock,'RGB')}),m,ue.MaterialProperty.MP_BASE_COLOR)
normal=sample(m,imported['Snow']['Normal'],uv)
result(custom(m,'return normalize(float3(N.xy*.42,max(.15,N.z)));',{'N':normal}),m,ue.MaterialProperty.MP_NORMAL)
result(scalar(m,.9),m,ue.MaterialProperty.MP_ROUGHNESS);result(scalar(m,.2),m,ue.MaterialProperty.MP_SPECULAR);finish(m)

# Winter variants of the existing scanned vegetation, retaining its alpha and UVs.
bindings=json.loads((root/'Validation/IslandBindings.json').read_text())
for path,row in bindings.items():
    original='M_Island_'+path.split('.')[-1].removeprefix('MI_');name='M_Winter_'+original
    tex=row['textures'];leaf='/3D_Plants/' in path and not any(s in path for s in ['Trunk','Tileable'])
    m=material(name);m.set_editor_property('used_with_instanced_static_meshes',True);m.set_editor_property('used_with_nanite',True)
    m.set_editor_property('blend_mode',ue.BlendMode.BLEND_MASKED if leaf else ue.BlendMode.BLEND_OPAQUE);m.set_editor_property('two_sided',leaf)
    m.set_editor_property('opacity_mask_clip_value',.4);m.set_editor_property('shading_model',ue.MaterialShadingModel.MSM_TWO_SIDED_FOLIAGE if leaf else ue.MaterialShadingModel.MSM_DEFAULT_LIT)
    base=sample(m,ue.load_asset('/Game/IslandLook/Textures/'+ue.load_asset(tex['Albedo']).get_name()))
    n=expression(m,ue.MaterialExpressionVertexNormalWS);p=expression(m,ue.MaterialExpressionWorldPosition)
    colour=custom(m,NOISE+'''
float coat=smoothstep(.0,.8,N.z)*(.65+.25*noise.fbm(P.xy*.001));
return lerp(C*float3(.5,.58,.57),float3(.7,.8,.85),coat);
''',{'C':(base,'RGB'),'P':p,'N':n})
    result(colour,m,ue.MaterialProperty.MP_BASE_COLOR);result(scalar(m,.92),m,ue.MaterialProperty.MP_ROUGHNESS)
    if leaf:
        result(base,m,ue.MaterialProperty.MP_OPACITY_MASK,'A');result(custom(m,'return C*.35;',{'C':colour}),m,ue.MaterialProperty.MP_SUBSURFACE_COLOR)
    if 'Normal' in tex:result(sample(m,ue.load_asset('/Game/IslandLook/Textures/'+ue.load_asset(tex['Normal']).get_name())),m,ue.MaterialProperty.MP_NORMAL)
    finish(m)

# Recolour the existing animated ocean only for Frostfjord; retain its wave graph.
m=ue.load_asset('/Game/Materials/M_Ocean');frost=expression(m,ue.MaterialExpressionScalarParameter,parameter_name='FrostWater',default_value=0.)
result(custom(m,'return lerp(float3(.012,.047,.055),float3(.009,.018,.031),F);',{'F':frost}),m,ue.MaterialProperty.MP_BASE_COLOR)
for node in ue.MaterialEditingLibrary.get_material_expressions(m):
    if isinstance(node,ue.MaterialExpressionSingleLayerWaterMaterialOutput):
        connect(custom(m,'return lerp(float3(.0015,.008,.012),float3(.001,.003,.007),F);',{'F':frost}),node,'ScatteringCoefficients')
        connect(custom(m,'return lerp(float3(.018,.005,.003),float3(.028,.012,.006),F);',{'F':frost}),node,'AbsorptionCoefficients')
finish(m)

(root/'Validation/FabWorldImport.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
ue.log('BENCHY_FAB_WORLDS_READY')
