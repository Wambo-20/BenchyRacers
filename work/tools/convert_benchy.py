import bpy, pathlib
root=pathlib.Path(__file__).resolve().parents[1]/'game'
(root/'assets').mkdir(parents=True,exist_ok=True)
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)
bpy.ops.wm.stl_import(filepath=str(pathlib.Path(__file__).parent/'3DBenchy.stl'))
obj=bpy.context.object
obj.name='Original_3DBenchy_Daniel_Noree'
mod=obj.modifiers.new('Game ready geometry','DECIMATE');mod.ratio=0.095
bpy.ops.object.modifier_apply(modifier=mod.name)
for v in obj.data.vertices:
    x,y,z=v.co
    v.co=(-y*.06, (x-.8245)*.06, z*.06)
bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
for poly in obj.data.polygons: poly.use_smooth=True
bpy.ops.export_scene.gltf(filepath=str(root/'assets/benchy.glb'),export_format='GLB',use_selection=True)
print('Exported original Benchy',len(obj.data.polygons),'triangles')
