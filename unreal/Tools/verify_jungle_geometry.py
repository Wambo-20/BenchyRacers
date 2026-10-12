"""Verify the exported solid rocks are still closed and consistently oriented."""
import bpy, bmesh, json, sys
from pathlib import Path
from mathutils import Vector
root=Path(sys.argv[sys.argv.index('--')+1]).resolve()
result=[]
for name in ['SM_SolidRock1','SM_SolidRock2','SM_JungleTree']:
    bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
    bpy.context.scene.unit_settings.scale_length=1
    bpy.ops.import_scene.fbx(filepath=str(root/'AssetSource/Meshes'/f'{name}.fbx'))
    obj=next(o for o in bpy.context.scene.objects if o.type=='MESH')
    edges={}
    for p in obj.data.polygons:
        for e in p.edge_keys:edges[e]=edges.get(e,0)+1
    closed=all(n==2 for n in edges.values())
    entry={'name':name,'polygon_count':len(obj.data.polygons),'closed_surface':closed}
    if 'Rock' in name:
        assert closed,name
        # Stepped, concave rock ledges need a topology-based orientation check;
        # dot(normal, center) only works for convex shapes around the origin.
        bm=bmesh.new();bm.from_mesh(obj.data);bm.faces.ensure_lookup_table()
        before=[f.normal.copy() for f in bm.faces]
        bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces))
        wrong=sum(1 for f,n in zip(bm.faces,before) if f.normal.dot(n)<.99)
        assert bm.calc_volume(signed=True)>0,(name,'negative signed volume')
        bm.free()
        assert wrong==0,(name,'inward faces',wrong)
        entry['inward_faces']=wrong
    else:
        assert any('Leaves' in s.material.name for s in obj.material_slots)
        assert entry['polygon_count']>5000
    result.append(entry);print('JUNGLE_ROUNDTRIP',entry,flush=True)
(root/'AssetSource/Meshes/jungle-roundtrip-report.json').write_text(json.dumps(result,indent=2))
