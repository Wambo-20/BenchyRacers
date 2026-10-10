import bpy,pathlib,math
root=pathlib.Path(__file__).resolve().parents[1]
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
bpy.ops.wm.stl_import(filepath=str(pathlib.Path(__file__).parent/'3DBenchy.stl'))
obj=bpy.context.object
obj.scale=(.06,.06,.06)
mat=bpy.data.materials.new('Mandarinorange');mat.diffuse_color=(0.95,.24,.045,1);mat.use_nodes=True
bsdf=mat.node_tree.nodes.get('Principled BSDF');bsdf.inputs['Base Color'].default_value=(.95,.24,.045,1);bsdf.inputs['Roughness'].default_value=.7
obj.data.materials.append(mat)
camera_data=bpy.data.cameras.new('IconCamera');camera=bpy.data.objects.new('IconCamera',camera_data);bpy.context.collection.objects.link(camera)
camera.location=(4.8,-6.0,4.3);target=__import__('mathutils').Vector((0,0,1.3));camera.rotation_euler=(target-camera.location).to_track_quat('-Z','Y').to_euler()
camera_data.type='ORTHO';camera_data.ortho_scale=5.4
scene=bpy.context.scene;scene.camera=camera
for name,pos,energy,size in [('Key',(2,-4,7),600,5),('Fill',(-5,-1,4),350,4)]:
    data=bpy.data.lights.new(name,'AREA');data.energy=energy;data.shape='DISK';data.size=size
    light=bpy.data.objects.new(name,data);bpy.context.collection.objects.link(light);light.location=pos
    light.rotation_euler=(target-light.location).to_track_quat('-Z','Y').to_euler()
scene.render.engine='BLENDER_EEVEE_NEXT';scene.render.resolution_x=256;scene.render.resolution_y=256;scene.render.resolution_percentage=100
scene.render.film_transparent=True;scene.render.image_settings.file_format='PNG';scene.render.filepath=str(root/'game/assets/icon.png')
bpy.ops.render.render(write_still=True)
