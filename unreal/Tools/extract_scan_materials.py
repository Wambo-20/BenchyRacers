"""Extract canonical PBR textures and slot assignments from the converted FBX assets."""
import bpy
import json
import re
import sys
from pathlib import Path

root = Path(sys.argv[sys.argv.index("--")+1])
destination = root / "Textures/Scans"
destination.mkdir(parents=True, exist_ok=True)
report = {}
imports = []
seen = set()

def image_from_socket(socket, visited=None):
    visited = set() if visited is None else visited
    for link in socket.links:
        node = link.from_node
        if node.name in visited:
            continue
        visited.add(node.name)
        if node.type == "TEX_IMAGE" and node.image:
            return node.image
        for child in node.inputs:
            image = image_from_socket(child, visited)
            if image:
                return image
    return None

for mesh_name in ["SM_Boulder", "SM_CoastRocks", "SM_Tree", "SM_Fern"]:
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)
    for material in list(bpy.data.materials):
        bpy.data.materials.remove(material)
    bpy.ops.import_scene.fbx(filepath=str(root / "Meshes" / (mesh_name+".fbx")))
    obj = next(o for o in bpy.context.scene.objects if o.type == "MESH")
    slots = []
    for slot in obj.material_slots:
        material = slot.material
        key = re.sub(r"[^A-Za-z0-9_]", "_", material.name)
        shader = next(n for n in material.node_tree.nodes if n.type == "BSDF_PRINCIPLED")
        info = {"slot": material.name, "material": "M_Scan_"+key, "textures": {}}
        for channel, socket in [("color", "Base Color"), ("normal", "Normal"), ("rough", "Roughness")]:
            image = image_from_socket(shader.inputs[socket])
            if image is None:
                continue
            name = key+"_"+channel
            file = destination / (name+".png")
            image.filepath_raw = str(file)
            image.file_format = "PNG"
            image.save()
            info["textures"][channel] = name
            if name not in seen:
                imports.append({"file": "Textures/Scans/"+file.name,
                    "destination": "/Game/Nature/ScanTextures", "name": name})
                seen.add(name)
        opacity = destination / (key+"_opacity.png")
        if opacity.exists():
            name = key+"_opacity"
            info["textures"]["opacity"] = name
            imports.append({"file": "Textures/Scans/"+opacity.name,
                "destination": "/Game/Nature/ScanTextures", "name": name})
        if "color" not in info["textures"]:
            raise RuntimeError("Missing diffuse texture for "+material.name)
        slots.append(info)
    report[mesh_name] = slots

(root / "scan-materials.json").write_text(json.dumps(report, indent=2))
manifest_path = root / "manifest.json"
manifest = [e for e in json.loads(manifest_path.read_text()) if not e["file"].startswith("Textures/Scans/")]
manifest_path.write_text(json.dumps(manifest+imports, indent=2))
print("SCAN_TEXTURES_EXTRACTED", len(imports), json.dumps(report), flush=True)
