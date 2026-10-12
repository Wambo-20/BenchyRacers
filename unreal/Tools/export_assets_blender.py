"""Convert existing CC0 assets to UE-friendly centimetre FBX files with textures."""
import bpy
import math
import json
import sys
from pathlib import Path
from mathutils import Vector, Matrix

args = sys.argv[sys.argv.index("--") + 1:]
source, destination = (Path(p).resolve() for p in args)
destination.mkdir(parents=True, exist_ok=True)

def clear():
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)
    for material in list(bpy.data.materials):
        bpy.data.materials.remove(material)
    for mesh in list(bpy.data.meshes):
        if mesh.users == 0:
            bpy.data.meshes.remove(mesh)
    for image in list(bpy.data.images):
        if image.users == 0:
            bpy.data.images.remove(image)
    # glTF import compensates for the current Blender scene's unit scale.
    # Reset it before every import, then declare centimetres only on export.
    bpy.context.scene.unit_settings.scale_length = 1.0

def export(name):
    meshes = [o for o in bpy.context.scene.objects if o.type == "MESH"]
    assert meshes, name
    # Apply glTF node transforms, then combine the parts with their materials.
    for o in meshes:
        matrix = o.matrix_world.copy()
        o.data.transform(matrix)
        o.matrix_world = Matrix.Identity(4)
    for o in bpy.context.scene.objects:
        o.select_set(o in meshes)
    bpy.context.view_layer.objects.active = meshes[0]
    bpy.ops.object.join()
    obj = bpy.context.object
    obj.name = name
    if name == "SM_Benchy":
        # The Godot model's forward -Z corresponds to Blender forward +Y.
        obj.data.transform(Matrix.Rotation(-math.pi / 2, 4, "Z"))
    vertices = [v.co for v in obj.data.vertices]
    lo = Vector(tuple(min(p[a] for p in vertices) for a in range(3)))
    hi = Vector(tuple(max(p[a] for p in vertices) for a in range(3)))
    offset = Vector(((lo.x + hi.x) / 2, (lo.y + hi.y) / 2, lo.z))
    for v in obj.data.vertices:
        v.co -= offset
    # Store actual centimetres; FBX scene metadata also declares centimetres.
    for v in obj.data.vertices:
        v.co *= 100
    bpy.context.scene.unit_settings.system = "METRIC"
    bpy.context.scene.unit_settings.scale_length = .01
    texture_dir = destination / "EmbeddedTextures" / name
    texture_dir.mkdir(parents=True, exist_ok=True)
    for i, image in enumerate(bpy.data.images):
        if image.users and image.type == "IMAGE":
            image.filepath_raw = str(texture_dir / f"{name}_{i}_{bpy.path.clean_name(image.name)}.png")
            image.file_format = "PNG"
            image.save()
    bpy.ops.export_scene.fbx(filepath=str(destination / (name + ".fbx")),
        use_selection=True, object_types={"MESH"}, axis_forward="X", axis_up="Z",
        apply_unit_scale=True, apply_scale_options="FBX_SCALE_UNITS",
        bake_anim=False, use_mesh_modifiers=True, path_mode="COPY", embed_textures=True)
    dimensions = tuple((hi - lo) * 100)
    print("EXPORTED", name, "cm bounds", dimensions, flush=True)
    return {"name": name, "dimensions_cm": dimensions, "polygons": len(obj.data.polygons)}

report = []
for filename, name in [("benchy.glb", "SM_Benchy"), ("scans/boulder_01.glb", "SM_Boulder"),
    ("scans/coast_rocks_02.glb", "SM_CoastRocks"), ("scans/island_tree_02.glb", "SM_Tree"),
    ("scans/fern_02.glb", "SM_Fern")]:
    clear()
    bpy.ops.import_scene.gltf(filepath=str(source / filename))
    report.append(export(name))

clear()
def colored_material(name, color):
    m = bpy.data.materials.new(name)
    m.diffuse_color = (*color, 1)
    m.use_nodes = True
    bsdf = m.node_tree.nodes.get("Principled BSDF")
    bsdf.inputs["Base Color"].default_value = (*color, 1)
    bsdf.inputs["Roughness"].default_value = .85
    return m
bark = colored_material("PalmBark", (.19,.12,.055))
leaf = colored_material("PalmLeaves", (.07,.23,.025))
# Curved trunk with rings and individual drooping leaflets.
verts, faces, mats = [], [], []
for j in range(29):
    t = j / 28
    center = Vector((math.sin(t*1.2)*1.8, t*t*.7, t*12))
    radius = .18*(1-t*.45)*(1+.04*math.sin(j*2.1))
    for k in range(12):
        a = k*math.tau/12
        verts.append(tuple(center+Vector((math.cos(a)*radius, math.sin(a)*radius, 0))))
for j in range(28):
    for k in range(12):
        faces.append((j*12+k,j*12+(k+1)%12,(j+1)*12+(k+1)%12,(j+1)*12+k));mats.append(0)
top = Vector((math.sin(1.2)*1.8,.7,12))
for branch in range(18):
    angle = branch*math.tau/18
    outward = Vector((math.cos(angle), math.sin(angle), 0))
    side = Vector((-math.sin(angle),math.cos(angle),0))
    for j in range(1,29):
        t = j/29
        # Broad, overlapping folded leaflets keep the crown readable at racing distance.
        mid = top + outward*(t*4.6)+Vector((0,0,math.sin(t*math.pi)*1.1-t*t*(1.1+branch%3*.3)))
        length = math.sin(t*math.pi)*1.45+.1
        for sign in [-1,1]:
            a = mid-outward*.15
            b = mid+outward*.15
            tip = mid+side*sign*length-outward*.18+Vector((0,0,-.35*t))
            fold = (a+b+tip)/3+Vector((0,0,.1))
            idx = len(verts);verts += [tuple(a),tuple(b),tuple(tip),tuple(fold)]
            faces.extend([(idx,idx+1,idx+3),(idx+1,idx+2,idx+3),(idx+2,idx,idx+3)]);mats.extend([1,1,1])
mesh = bpy.data.meshes.new("Palm");mesh.from_pydata(verts,[],faces);mesh.update()
obj = bpy.data.objects.new("Palm",mesh);bpy.context.collection.objects.link(obj)
mesh.materials.append(bark);mesh.materials.append(leaf)
for p, material_index in zip(mesh.polygons,mats):
    p.material_index=material_index;p.use_smooth=True
report.append(export("SM_Palm"))
(destination / "conversion-report.json").write_text(json.dumps(report, indent=2))
