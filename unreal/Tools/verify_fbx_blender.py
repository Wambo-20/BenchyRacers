"""Round-trip generated FBX files to verify geometry scale and completeness."""
import bpy
import sys
import json
from pathlib import Path
from mathutils import Vector
root = Path(sys.argv[sys.argv.index("--") + 1])
expected = json.loads((root / "conversion-report.json").read_text())
results = []
for entry in expected:
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)
    bpy.context.scene.unit_settings.scale_length = 1
    bpy.ops.import_scene.fbx(filepath=str(root / (entry["name"] + ".fbx")))
    meshes = [o for o in bpy.context.scene.objects if o.type == "MESH"]
    assert meshes, entry["name"]
    positions = [o.matrix_world @ v.co for o in meshes for v in o.data.vertices]
    size = [max(p[a] for p in positions)-min(p[a] for p in positions) for a in range(3)]
    cm = [v*100 for v in size]
    assert all(abs(a-b)<.1 for a,b in zip(cm,entry["dimensions_cm"])), (entry["name"], cm, entry["dimensions_cm"])
    assert sum(len(o.data.polygons) for o in meshes) == entry["polygons"]
    results.append({"name": entry["name"], "dimensions_cm": cm, "status": "roundtrip_passed"})
    print("ROUNDTRIP_PASS",entry["name"],cm,flush=True)
(root / "roundtrip-report.json").write_text(json.dumps(results,indent=2))
