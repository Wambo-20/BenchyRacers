import bpy,pathlib,json
from mathutils import Vector
root=pathlib.Path(__file__).resolve().parents[2]
output=root/'work/game/assets/scans';output.mkdir(parents=True,exist_ok=True)
report=[]
for name,target in [('boulder_01',8500),('coast_rocks_02',18000),('island_tree_02',42000),('fern_02',4000)]:
    bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
    bpy.ops.import_scene.gltf(filepath=str(root/'work/tools/raw-nature'/name/(name+'.gltf')))
    objects=[o for o in bpy.context.scene.objects if o.type=='MESH']
    if name=='fern_02':
        keep=max(objects,key=lambda o:len(o.data.polygons))
        for obj in objects:
            if obj!=keep:bpy.data.objects.remove(obj,do_unlink=True)
        objects=[keep]
    for obj in objects:
        bpy.context.view_layer.objects.active=obj
        faces=len(obj.data.polygons)
        if faces>target:
            modifier=obj.modifiers.new('Game geometry','DECIMATE');modifier.ratio=target/faces
            bpy.ops.object.modifier_apply(modifier=modifier.name)
    points=[obj.matrix_world@Vector(v) for obj in objects for v in obj.bound_box]
    bottom=min(v.z for v in points)
    center=Vector(((min(v.x for v in points)+max(v.x for v in points))/2,(min(v.y for v in points)+max(v.y for v in points))/2,bottom))
    for obj in objects:obj.location-=center
    for obj in bpy.context.scene.objects:
        if obj.type!='MESH':obj.hide_render=True
    bpy.ops.object.select_all(action='DESELECT')
    for obj in objects:obj.select_set(True)
    bpy.ops.export_scene.gltf(filepath=str(output/(name+'.glb')),export_format='GLB',use_selection=True,export_yup=True)
    info={'asset':name,'polygons':sum(len(o.data.polygons) for o in objects),'width':max(v.x for v in points)-min(v.x for v in points),'height':max(v.z for v in points)-bottom}
    report.append(info);print('BENCHY_SCAN',info,flush=True)
(root/'work/scan-report.json').write_text(json.dumps(report,indent=2))
(output/'credits.json').write_bytes((root/'work/tools/raw-nature/credits.json').read_bytes())
