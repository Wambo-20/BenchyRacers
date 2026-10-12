"""Run inside Unreal Editor: imports assets, builds materials and saves Sonnendeck."""
from pathlib import Path
import json
import sys
import unreal as ue

ROOT = Path(ue.Paths.project_dir()).resolve()
SOURCE = ROOT / "AssetSource"
sys.path.insert(0,str(ROOT / "Tools"))
from water_material_code import NOISE, sea_hlsl
TOOLS = ue.AssetToolsHelpers.get_asset_tools()
EDIT = ue.MaterialEditingLibrary
ASSETS = ue.get_editor_subsystem(ue.EditorAssetSubsystem)

def expression(material, cls, **props):
    node = EDIT.create_material_expression(material, cls)
    for key, value in props.items():
        node.set_editor_property(key, value)
    return node

def connect(a, b, pin="", output=""):
    if not EDIT.connect_material_expressions(a, output, b, pin):
        raise RuntimeError(f"Material connection failed: {a.get_name()} -> {b.get_name()}:{pin}")

def result(node, material, prop, output=""):
    if not EDIT.connect_material_property(node, output, prop):
        raise RuntimeError(f"Material property connection failed: {prop}")

def scalar(m, value):
    return expression(m, ue.MaterialExpressionConstant, r=value)

def vector(m, color):
    return expression(m, ue.MaterialExpressionConstant3Vector, constant=ue.LinearColor(*color, 1))

def custom(m, code, inputs, output=ue.CustomMaterialOutputType.CMOT_FLOAT3):
    node = expression(m, ue.MaterialExpressionCustom, code=code, output_type=output)
    specs = []
    for name in inputs:
        item = ue.CustomInput()
        item.set_editor_property("input_name", name)
        specs.append(item)
    node.set_editor_property("inputs", specs)
    for name, src in inputs.items():
        if isinstance(src, tuple):
            connect(src[0], node, name, src[1])
        else:
            connect(src, node, name)
    return node

def material(name):
    path = "/Game/Materials/" + name
    existing = ue.load_asset(path)
    if existing:
        # Setup owns these generated assets. Rebuild the graph on a repeated run.
        EDIT.delete_all_material_expressions(existing)
        return existing
    return TOOLS.create_asset(name, "/Game/Materials", ue.Material, ue.MaterialFactoryNew())

def finish(m):
    # Vegetation is spawned at runtime as HISM. Cook these shader variants explicitly.
    if m.get_name().startswith("M_Palm") or m.get_name().startswith("M_Scan_") or m.get_name() in ["M_CliffRock","M_JungleLeaves"]:
        m.set_editor_property("used_with_instanced_static_meshes", True)
    EDIT.recompile_material(m)
    ASSETS.save_loaded_asset(m)

def ocean_material():
    m = material("M_Ocean")
    m.set_editor_property("shading_model", ue.MaterialShadingModel.MSM_SINGLE_LAYER_WATER)
    m.set_editor_property("tangent_space_normal", False)
    m.set_editor_property("two_sided", True)
    p = expression(m, ue.MaterialExpressionWorldPosition)
    t = expression(m, ue.MaterialExpressionScalarParameter, parameter_name="SeaTime", default_value=0.)
    spectrum = sea_hlsl(ROOT)
    result(custom(m, spectrum+"return float3(0,0,h);", {"P": p, "T": t}), m, ue.MaterialProperty.MP_WORLD_POSITION_OFFSET)
    normal_code = spectrum+NOISE+"""
float2 q=P.xy*.0057+float2(T*.21,-T*.13);
float2 warp2=float2(noise.fbm(q*.19),noise.fbm(q*.23+17.4))*2.7;
q+=warp2;
float eps=.09;
dx+=(noise.fbm(q+float2(eps,0))-noise.fbm(q-float2(eps,0)))*.38;
dy+=(noise.fbm(q+float2(0,eps))-noise.fbm(q-float2(0,eps)))*.38;
return normalize(float3(-dx,-dy,1));
"""
    result(custom(m, normal_code, {"P": p, "T": t}), m, ue.MaterialProperty.MP_NORMAL)
    result(vector(m, (.012,.047,.055)), m, ue.MaterialProperty.MP_BASE_COLOR)
    result(custom(m, NOISE+"return .21+noise.fbm(P.xy*.0007)*.055;", {"P":p}, ue.CustomMaterialOutputType.CMOT_FLOAT1), m, ue.MaterialProperty.MP_ROUGHNESS)
    result(scalar(m, .08), m, ue.MaterialProperty.MP_OPACITY)
    water_nodes = [n for n in EDIT.get_material_expressions(m)
                   if isinstance(n, ue.MaterialExpressionSingleLayerWaterMaterialOutput)]
    water = water_nodes[0] if water_nodes else expression(m, ue.MaterialExpressionSingleLayerWaterMaterialOutput)
    for duplicate in water_nodes[1:]:
        EDIT.delete_material_expression(m, duplicate)
    connect(vector(m, (.0015,.008,.012)), water, "ScatteringCoefficients")
    connect(vector(m, (.018,.005,.003)), water, "AbsorptionCoefficients")
    connect(scalar(m, .3), water, "PhaseG")
    connect(vector(m, (.94,.98,1)), water, "ColorScaleBehindWater")
    finish(m)

def wake_material():
    m = material("M_Wake")
    m.set_editor_property("blend_mode", ue.BlendMode.BLEND_TRANSLUCENT)
    m.set_editor_property("two_sided", True)
    m.set_editor_property("shading_model", ue.MaterialShadingModel.MSM_DEFAULT_LIT)
    m.set_editor_property("translucency_lighting_mode", ue.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)
    uv = expression(m, ue.MaterialExpressionTextureCoordinate)
    p = expression(m, ue.MaterialExpressionWorldPosition)
    t = expression(m, ue.MaterialExpressionTime)
    v = expression(m, ue.MaterialExpressionVertexColor)
    foam = custom(m, NOISE+"""
float2 q=P.xy*.035+float2(T*.17,-T*.24);
float n=noise.fbm(q+float2(noise.value(q*.3),noise.value(q*.27+9))*2);
float edge=pow(saturate(1-abs(UV.x*2-1)),2.4);
float broken=smoothstep(.48,.73,n);
float head=smoothstep(0,.07,UV.y);
return edge*(.015+broken*.24)*A*head;
""", {"P": p, "T": t, "UV": uv, "A": (v, "A")}, ue.CustomMaterialOutputType.CMOT_FLOAT1)
    result(foam, m, ue.MaterialProperty.MP_OPACITY)
    result(vector(m, (.72,.83,.81)), m, ue.MaterialProperty.MP_BASE_COLOR)
    result(scalar(m, .95), m, ue.MaterialProperty.MP_ROUGHNESS)
    finish(m)

def filament_material():
    m = material("M_Filament")
    color = expression(m, ue.MaterialExpressionVectorParameter,
        parameter_name="FilamentColor", default_value=ue.LinearColor(1,.3,.025,1))
    galaxy = expression(m, ue.MaterialExpressionScalarParameter, parameter_name="Galaxy", default_value=0.)
    p = expression(m, ue.MaterialExpressionObjectPositionWS)
    # World-aligned fine specks; their highlights change with lighting and view.
    world = expression(m, ue.MaterialExpressionWorldPosition)
    glitter = custom(m, """
float3 q=floor((P-O)*.7);
float h=frac(sin(dot(q,float3(12.9898,78.233,37.719)))*43758.5453);
return step(.995,h)*G;
""", {"P": world, "O": p, "G": galaxy}, ue.CustomMaterialOutputType.CMOT_FLOAT1)
    result(color, m, ue.MaterialProperty.MP_BASE_COLOR)
    roughness = custom(m, "return lerp(.62,.12,S);", {"S": glitter}, ue.CustomMaterialOutputType.CMOT_FLOAT1)
    result(roughness, m, ue.MaterialProperty.MP_ROUGHNESS)
    result(glitter, m, ue.MaterialProperty.MP_METALLIC)
    finish(m)

def terrain_material():
    m = material("M_Terrain")
    weights = expression(m, ue.MaterialExpressionVertexColor)
    uv = expression(m, ue.MaterialExpressionTextureCoordinate)
    position = expression(m, ue.MaterialExpressionWorldPosition)
    surface_normal = expression(m, ue.MaterialExpressionVertexNormalWS)
    samples = {}
    normal_samples = {}
    for channel, name in [("R","sand_color"),("G","grass_color"),("B","rock_color")]:
        tex = ue.load_asset("/Game/Nature/Textures/" + name)
        if tex is None:
            raise RuntimeError("Missing terrain texture: " + name)
        sample = expression(m, ue.MaterialExpressionTextureSample, texture=tex)
        connect(uv, sample, "UVs")
        if channel == "B":
            projections={"P":position,"N":surface_normal}
            for axis,code in [("X","return P.yz*.006;"),("Y","return P.xz*.006;"),("Z","return P.xy*.006;")]:
                coords=custom(m,code,{"P":position},ue.CustomMaterialOutputType.CMOT_FLOAT2)
                projected=expression(m,ue.MaterialExpressionTextureSample,
                    texture=ue.load_asset("/Game/Nature/ScanTextures/boulder_01_color"))
                connect(coords,projected,"UVs");projections[axis]=(projected,"RGB")
            sample=custom(m,NOISE+"""
float3 w=pow(abs(N),4);w/=max(.001,w.x+w.y+w.z);
float macro=.8+noise.fbm(P.xy*.00037)*.3;
return (X*w.x+Y*w.y+Z*w.z)*macro;
""",projections)
        if channel == "G":
            tint = expression(m, ue.MaterialExpressionMultiply)
            connect(sample, tint, "A")
            connect(vector(m, (.55,1.5,.5)), tint, "B")
            sample = tint
        channels = dict(r=False, g=False, b=False, a=False)
        channels[channel.lower()] = True
        mask = expression(m, ue.MaterialExpressionComponentMask, **channels)
        connect(weights, mask)
        mul = expression(m, ue.MaterialExpressionMultiply)
        connect(sample, mul, "A"); connect(mask, mul, "B")
        samples[channel] = mul
        normal_tex = ue.load_asset("/Game/Nature/Textures/" + name.replace("_color", "_normal"))
        normal = expression(m, ue.MaterialExpressionTextureSample, texture=normal_tex,
            sampler_type=ue.MaterialSamplerType.SAMPLERTYPE_NORMAL)
        connect(uv, normal, "UVs")
        if channel == "B":
            normal=vector(m,(0,0,1))
        normal_mul = expression(m, ue.MaterialExpressionMultiply)
        connect(normal, normal_mul, "A"); connect(mask, normal_mul, "B")
        normal_samples[channel] = normal_mul
    add = expression(m, ue.MaterialExpressionAdd)
    connect(samples["R"], add, "A"); connect(samples["G"], add, "B")
    all_colors = expression(m, ue.MaterialExpressionAdd)
    connect(add, all_colors, "A"); connect(samples["B"], all_colors, "B")
    result(all_colors, m, ue.MaterialProperty.MP_BASE_COLOR)
    normal_add = expression(m, ue.MaterialExpressionAdd)
    connect(normal_samples["R"], normal_add, "A"); connect(normal_samples["G"], normal_add, "B")
    normal_sum = expression(m, ue.MaterialExpressionAdd)
    connect(normal_add, normal_sum, "A"); connect(normal_samples["B"], normal_sum, "B")
    result(custom(m, "return normalize(N);", {"N": normal_sum}), m, ue.MaterialProperty.MP_NORMAL)
    result(scalar(m, .88), m, ue.MaterialProperty.MP_ROUGHNESS)
    result(scalar(m, .15), m, ue.MaterialProperty.MP_SPECULAR)
    finish(m)

def palm_materials():
    m = material("M_PalmLeaves")
    m.set_editor_property("two_sided", True)
    m.set_editor_property("shading_model", ue.MaterialShadingModel.MSM_TWO_SIDED_FOLIAGE)
    result(vector(m, (.07,.24,.035)), m, ue.MaterialProperty.MP_BASE_COLOR)
    result(vector(m, (.008,.025,.002)), m, ue.MaterialProperty.MP_EMISSIVE_COLOR)
    result(vector(m, (.12,.35,.025)), m, ue.MaterialProperty.MP_SUBSURFACE_COLOR)
    result(scalar(m, .72), m, ue.MaterialProperty.MP_ROUGHNESS)
    p = expression(m, ue.MaterialExpressionWorldPosition)
    origin = expression(m, ue.MaterialExpressionObjectPositionWS)
    t = expression(m, ue.MaterialExpressionTime)
    sway = custom(m, """
float bend=pow(saturate((P.z-O.z+600)/1200),2);
return float3(sin(T*1.3+P.y*.001)*12,cos(T*.9+P.x*.001)*8,0)*bend;
""", {"P": p, "O": origin, "T": t})
    result(sway, m, ue.MaterialProperty.MP_WORLD_POSITION_OFFSET)
    finish(m)
    m = material("M_PalmBark")
    position = expression(m, ue.MaterialExpressionWorldPosition)
    uv = custom(m, "return float2(P.x*.004,P.z*.004);", {"P": position}, ue.CustomMaterialOutputType.CMOT_FLOAT2)
    texture = expression(m, ue.MaterialExpressionTextureSample, texture=ue.load_asset("/Game/Nature/Textures/bark_color"))
    connect(uv, texture, "UVs")
    result(texture, m, ue.MaterialProperty.MP_BASE_COLOR)
    result(scalar(m, .9), m, ue.MaterialProperty.MP_ROUGHNESS)
    finish(m)
    m = material("M_Gate")
    result(vector(m, (.03,.65,.55)), m, ue.MaterialProperty.MP_BASE_COLOR)
    result(scalar(m, .35), m, ue.MaterialProperty.MP_ROUGHNESS)
    finish(m)

def jungle_materials():
    m=material("M_CliffRock")
    m.set_editor_property("blend_mode",ue.BlendMode.BLEND_OPAQUE)
    m.set_editor_property("shading_model",ue.MaterialShadingModel.MSM_DEFAULT_LIT)
    m.set_editor_property("two_sided",False)
    p=expression(m,ue.MaterialExpressionWorldPosition)
    n=expression(m,ue.MaterialExpressionVertexNormalWS)
    inputs={"P":p,"N":n}
    for axis,code in [("X","return P.yz*.006;"),("Y","return P.xz*.006;"),("Z","return P.xy*.006;")]:
        uv=custom(m,code,{"P":p},ue.CustomMaterialOutputType.CMOT_FLOAT2)
        sample=expression(m,ue.MaterialExpressionTextureSample,texture=ue.load_asset("/Game/Nature/ScanTextures/boulder_01_color"))
        connect(uv,sample,"UVs");inputs[axis]=(sample,"RGB")
    color=custom(m,NOISE+"""
float3 w=pow(abs(N),4);w/=max(.001,w.x+w.y+w.z);
float macro=.80+noise.fbm(P.xy*.00037)*.3;
float3 rock=(X*w.x+Y*w.y+Z*w.z)*macro;
float moss=smoothstep(.35,.88,N.z)*smoothstep(.3,.8,noise.fbm(P.xy*.0031))*.55;
return lerp(rock,float3(.035,.075,.019),moss);
""",inputs)
    result(color,m,ue.MaterialProperty.MP_BASE_COLOR)
    result(scalar(m,.91),m,ue.MaterialProperty.MP_ROUGHNESS)
    result(scalar(m,0),m,ue.MaterialProperty.MP_METALLIC)
    result(scalar(m,.12),m,ue.MaterialProperty.MP_SPECULAR)
    result(scalar(m,1),m,ue.MaterialProperty.MP_OPACITY)
    finish(m)
    m=material("M_JungleLeaves")
    m.set_editor_property("blend_mode",ue.BlendMode.BLEND_OPAQUE)
    m.set_editor_property("two_sided",True)
    m.set_editor_property("shading_model",ue.MaterialShadingModel.MSM_TWO_SIDED_FOLIAGE)
    rand=expression(m,ue.MaterialExpressionPerInstanceRandom)
    color=custom(m,"return lerp(float3(.035,.13,.017),float3(.085,.25,.035),R);",{"R":rand})
    result(color,m,ue.MaterialProperty.MP_BASE_COLOR)
    result(vector(m,(.12,.29,.035)),m,ue.MaterialProperty.MP_SUBSURFACE_COLOR)
    result(scalar(m,.83),m,ue.MaterialProperty.MP_ROUGHNESS)
    p=expression(m,ue.MaterialExpressionWorldPosition);t=expression(m,ue.MaterialExpressionTime)
    wind=custom(m,"float bend=saturate(P.z/7000);return float3(sin(T*.73+P.y*.002)*18,cos(T*.91+P.x*.0013)*12,0)*bend;",{"P":p,"T":t})
    result(wind,m,ue.MaterialProperty.MP_WORLD_POSITION_OFFSET)
    finish(m)

def waterfall_materials():
    for name in ["M_Waterfall","M_FallFoam","M_FallMist"]:
        m=material(name)
        m.set_editor_property("blend_mode",ue.BlendMode.BLEND_TRANSLUCENT)
        m.set_editor_property("two_sided",True)
        m.set_editor_property("shading_model",ue.MaterialShadingModel.MSM_UNLIT if name=="M_FallMist" else ue.MaterialShadingModel.MSM_DEFAULT_LIT)
        m.set_editor_property("translucency_lighting_mode",ue.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)
        uv=expression(m,ue.MaterialExpressionTextureCoordinate)
        p=expression(m,ue.MaterialExpressionWorldPosition)
        t=expression(m,ue.MaterialExpressionScalarParameter,parameter_name="SeaTime",default_value=0.)
        if name=="M_Waterfall":
            opacity=custom(m,NOISE+"""
float2 q=float2(UV.x*42,sqrt(saturate(UV.y))*9-T*2.5);
q.x+=noise.fbm(float2(UV.x*9,UV.y*2-T*.3))*3;
float streams=noise.fbm(q);
float edge=smoothstep(0,.07,UV.x)*(1-smoothstep(.93,1,UV.x));
return edge*smoothstep(.35,.67,streams)*.78*(1-smoothstep(.96,1,UV.y));
""",{"UV":uv,"T":t},ue.CustomMaterialOutputType.CMOT_FLOAT1)
            wobble=custom(m,NOISE+"return float3((noise.value(float2(UV.x*14,T*.7))-0.5)*55,noise.value(float2(UV.y*8-T*.6,UV.x*9))*35,0)*UV.y;",{"UV":uv,"T":t})
            result(wobble,m,ue.MaterialProperty.MP_WORLD_POSITION_OFFSET)
            result(vector(m,(.64,.78,.79)),m,ue.MaterialProperty.MP_BASE_COLOR)
            result(vector(m,(2600,3000,3100)),m,ue.MaterialProperty.MP_EMISSIVE_COLOR)
            result(scalar(m,.38),m,ue.MaterialProperty.MP_ROUGHNESS)
        elif name=="M_FallFoam":
            opacity=custom(m,NOISE+"""
float2 d=(UV-float2(.5,.45))*float2(2,2.8);
float edge=saturate(1-dot(d,d));
float n=noise.fbm(P.xy*.025+float2(T*.8,T*.3));
return edge*edge*smoothstep(.2,.7,n)*.68;
""",{"UV":uv,"P":p,"T":t},ue.CustomMaterialOutputType.CMOT_FLOAT1)
            result(custom(m,sea_hlsl(ROOT)+"return float3(0,0,h);",{"P":p,"T":t}),m,ue.MaterialProperty.MP_WORLD_POSITION_OFFSET)
            result(vector(m,(.8,.9,.88)),m,ue.MaterialProperty.MP_BASE_COLOR)
            result(vector(m,(1000,1150,1160)),m,ue.MaterialProperty.MP_EMISSIVE_COLOR)
            result(scalar(m,.95),m,ue.MaterialProperty.MP_ROUGHNESS)
        else:
            opacity=custom(m,NOISE+"""
float2 d=(UV-.5)*2;
float edge=saturate(1-dot(d,d));
return edge*edge*noise.fbm(float2(UV.x*4+T*.12,UV.y*5+T*.3))*.23;
""",{"UV":uv,"T":t},ue.CustomMaterialOutputType.CMOT_FLOAT1)
            result(vector(m,(2200,2500,2600)),m,ue.MaterialProperty.MP_EMISSIVE_COLOR)
            drift=custom(m,"return float3(sin(T*.37+P.x*.003)*100,cos(T*.43+P.x*.001)*150,sin(T*.61+P.x*.002)*110);",{"P":p,"T":t})
            result(drift,m,ue.MaterialProperty.MP_WORLD_POSITION_OFFSET)
        result(opacity,m,ue.MaterialProperty.MP_OPACITY)
        finish(m)

def import_assets():
    manifest = json.loads((SOURCE / "manifest.json").read_text(encoding="utf-8"))
    if "-BenchyPalmOnly" in ue.SystemLibrary.get_command_line():
        manifest = [entry for entry in manifest if entry["name"] == "SM_Palm"]
    if "-BenchyMasksOnly" in ue.SystemLibrary.get_command_line():
        manifest = [entry for entry in manifest if entry["name"].endswith("_opacity")]
    tasks = []
    for entry in manifest:
        path = SOURCE / entry["file"]
        if not path.is_file():
            raise RuntimeError("Missing source file: " + str(path))
        task = ue.AssetImportTask()
        task.filename = str(path)
        task.destination_path = entry["destination"]
        task.destination_name = entry["name"]
        task.automated = True
        task.replace_existing = True
        task.save = True
        if path.suffix.lower() == ".fbx":
            settings = ue.FbxImportUI()
            settings.import_mesh = True
            settings.import_as_skeletal = False
            settings.mesh_type_to_import = ue.FBXImportType.FBXIT_STATIC_MESH
            settings.import_materials = entry["name"] != "SM_Benchy"
            settings.import_textures = settings.import_materials
            settings.static_mesh_import_data.combine_meshes = True
            settings.static_mesh_import_data.auto_generate_collision = True
            settings.static_mesh_import_data.generate_lightmap_u_vs = False
            settings.static_mesh_import_data.convert_scene = True
            settings.static_mesh_import_data.convert_scene_unit = True
            task.options = settings
            task.factory = ue.FbxFactory()
        tasks.append(task)
    TOOLS.import_asset_tasks(tasks)
    for entry, task in zip(manifest, tasks):
        expected = entry["destination"] + "/" + entry["name"]
        if not ASSETS.does_asset_exist(expected):
            raise RuntimeError(f"Import did not create {expected}: {task.imported_object_paths}")
        if entry["name"].endswith("_normal"):
            tex = ue.load_asset(expected)
            tex.set_editor_property("compression_settings", ue.TextureCompressionSettings.TC_NORMALMAP)
            tex.set_editor_property("srgb", False)
            tex.set_editor_property("flip_green_channel", True)
            ASSETS.save_loaded_asset(tex)
        if entry["name"] == "WakeRunner":
            sound = ue.load_asset(expected)
            sound.set_editor_property("looping", True)
            ASSETS.save_loaded_asset(sound)
        if entry["name"] == "SM_Palm":
            mesh = ue.load_asset(expected)
            for slot in mesh.get_editor_property("static_materials"):
                leaf_material = slot.material_interface
                if isinstance(leaf_material, ue.Material) and "leaves" in leaf_material.get_name().lower():
                    leaf_material.set_editor_property("two_sided", True)
                    p = expression(leaf_material, ue.MaterialExpressionWorldPosition)
                    origin = expression(leaf_material, ue.MaterialExpressionObjectPositionWS)
                    t = expression(leaf_material, ue.MaterialExpressionTime)
                    sway = custom(leaf_material, """
float bend=pow(saturate((P.z-O.z+600)/1200),2);
return float3(sin(T*1.3+P.y*.001)*12,cos(T*.9+P.x*.001)*8,0)*bend;
""", {"P": p, "O": origin, "T": t})
                    result(sway, leaf_material, ue.MaterialProperty.MP_WORLD_POSITION_OFFSET)
                    finish(leaf_material)

def scan_materials():
    assignments = json.loads((SOURCE / "scan-materials.json").read_text())
    report = {}
    for mesh_name, entries in assignments.items():
        mesh = ue.load_asset("/Game/Nature/"+mesh_name)
        slots = mesh.get_editor_property("static_materials")
        if len(slots) != len(entries):
            raise RuntimeError("Unexpected material slot count for "+mesh_name)
        report[mesh_name] = []
        for index, entry in enumerate(entries):
            if str(slots[index].material_slot_name) != entry["slot"]:
                raise RuntimeError(f"Material slot mismatch: {mesh_name}:{index}")
            m = material(entry["material"])
            m.set_editor_property("blend_mode", ue.BlendMode.BLEND_OPAQUE)
            m.set_editor_property("shading_model", ue.MaterialShadingModel.MSM_DEFAULT_LIT)
            m.set_editor_property("two_sided", True)
            foliage = any(word in entry["slot"] for word in ["leaves", "fern"])
            if foliage:
                m.set_editor_property("two_sided", True)
                m.set_editor_property("blend_mode", ue.BlendMode.BLEND_MASKED)
                m.set_editor_property("opacity_mask_clip_value", .35)
                m.set_editor_property("shading_model", ue.MaterialShadingModel.MSM_TWO_SIDED_FOLIAGE)
            color = expression(m, ue.MaterialExpressionTextureSample,
                texture=ue.load_asset("/Game/Nature/ScanTextures/"+entry["textures"]["color"]))
            result(color, m, ue.MaterialProperty.MP_BASE_COLOR)
            if foliage:
                if "opacity" not in entry["textures"]:
                    raise RuntimeError("Foliage opacity mask missing: "+entry["slot"])
                mask = expression(m, ue.MaterialExpressionTextureSample,
                    texture=ue.load_asset("/Game/Nature/ScanTextures/"+entry["textures"]["opacity"]))
                result(mask, m, ue.MaterialProperty.MP_OPACITY_MASK, "R")
                result(vector(m, (.12,.28,.045)), m, ue.MaterialProperty.MP_SUBSURFACE_COLOR)
                # Approximate the small amount of light transmitted through dense crowns.
                bounce = custom(m, "return C*.06;", {"C": (color, "RGB")})
                result(bounce, m, ue.MaterialProperty.MP_EMISSIVE_COLOR)
            if "normal" in entry["textures"]:
                normal = expression(m, ue.MaterialExpressionTextureSample,
                    texture=ue.load_asset("/Game/Nature/ScanTextures/"+entry["textures"]["normal"]),
                    sampler_type=ue.MaterialSamplerType.SAMPLERTYPE_NORMAL)
                result(normal, m, ue.MaterialProperty.MP_NORMAL)
            result(scalar(m, .8), m, ue.MaterialProperty.MP_ROUGHNESS)
            finish(m)
            slots[index].material_interface = m
            report[mesh_name].append({"slot": entry["slot"], "material": m.get_path_name()})
        mesh.set_editor_property("static_materials", slots)
        ASSETS.save_loaded_asset(mesh)
    return report

def create_level():
    levels = ue.get_editor_subsystem(ue.LevelEditorSubsystem)
    actors = ue.get_editor_subsystem(ue.EditorActorSubsystem)
    target = "/Game/Maps/Sonnendeck"
    # Never silently overwrite a level somebody has edited.
    if ASSETS.does_asset_exist(target):
        ue.log("Sonnendeck already exists; retaining the edited level.")
        return
    if not levels.new_level(target):
        raise RuntimeError("Could not create Sonnendeck level")
    sun = actors.spawn_actor_from_class(ue.DirectionalLight, ue.Vector(0,0,12000), ue.Rotator(-38,-45,0))
    light = sun.get_component_by_class(ue.DirectionalLightComponent)
    light.set_editor_property("mobility", ue.ComponentMobility.MOVABLE)
    light.set_editor_property("intensity", 12000.)
    light.set_editor_property("atmosphere_sun_light", True)
    light.set_editor_property("light_color", ue.Color(255,240,217,255))
    actors.spawn_actor_from_class(ue.SkyAtmosphere, ue.Vector())
    sky = actors.spawn_actor_from_class(ue.SkyLight, ue.Vector(0,0,8000))
    sky_component = sky.get_component_by_class(ue.SkyLightComponent)
    sky_component.set_editor_property("mobility", ue.ComponentMobility.MOVABLE)
    sky_component.set_editor_property("real_time_capture", True)
    fog = actors.spawn_actor_from_class(ue.ExponentialHeightFog, ue.Vector(0,0,-600))
    fog_component = fog.get_component_by_class(ue.ExponentialHeightFogComponent)
    fog_component.set_editor_property("fog_density", .007)
    fog_component.set_volumetric_fog(True)
    actors.spawn_actor_from_class(ue.PlayerStart, ue.Vector(-8500,9000,180))
    levels.save_current_level()

def update_scenery():
    levels = ue.get_editor_subsystem(ue.LevelEditorSubsystem)
    actors = ue.get_editor_subsystem(ue.EditorActorSubsystem)
    if not levels.load_level("/Game/Maps/Sonnendeck"):
        raise RuntimeError("Could not load Sonnendeck for scenery update")
    scene_actors = actors.get_all_level_actors()
    clouds = next((a for a in scene_actors if isinstance(a, ue.VolumetricCloud)), None)
    if clouds is None:
        clouds = actors.spawn_actor_from_class(ue.VolumetricCloud, ue.Vector())
        clouds.set_actor_label("Sonnendeck – bewegte Wolken")
    component = clouds.get_component_by_class(ue.VolumetricCloudComponent)
    cloud_material = ue.load_asset("/Engine/EngineSky/VolumetricClouds/m_SimpleVolumetricCloud_Inst")
    if cloud_material is None:
        raise RuntimeError("Built-in cloud material missing")
    component.set_material(cloud_material)
    component.set_layer_bottom_altitude(1.4)
    component.set_layer_height(2.0)
    for actor in scene_actors:
        if isinstance(actor, ue.SkyLight):
            light = actor.get_component_by_class(ue.SkyLightComponent)
            light.set_intensity(1.7)
        if isinstance(actor, ue.DirectionalLight):
            light = actor.get_component_by_class(ue.DirectionalLightComponent)
            light.set_editor_property("cast_cloud_shadows", True)
            light.set_editor_property("cloud_shadow_strength", .35)
    levels.save_current_level()

def main():
    if "-BenchyMaterialsOnly" not in ue.SystemLibrary.get_command_line():
        import_assets()
    ocean_material(); wake_material(); filament_material(); terrain_material()
    palm_materials(); jungle_materials(); waterfall_materials()
    assignments = scan_materials()
    create_level()
    if "-BenchyUpdateScenery" in ue.SystemLibrary.get_command_line():
        update_scenery()
    report = {"status": "prepared", "engine": ue.SystemLibrary.get_engine_version(),
        "map": "/Game/Maps/Sonnendeck", "runtime_tested": False}
    palm = ue.load_asset("/Game/Nature/SM_Palm")
    report["palm_material_slots"] = [str(s.material_slot_name) for s in palm.get_editor_property("static_materials")]
    report["scan_material_assignments"] = assignments
    (ROOT / "Saved").mkdir(exist_ok=True)
    (ROOT / "Saved/Preparation.json").write_text(json.dumps(report, indent=2))
    ue.log("BENCHY_UNREAL_PREPARED — build and play-test still required")

if __name__ == "__main__":
    main()
