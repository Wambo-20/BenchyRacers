"""Cook the instanced buoy shader permutation without changing the filament look."""
import unreal as ue
m = ue.load_asset('/Game/Materials/M_Filament')
assert m
m.set_editor_property('used_with_instanced_static_meshes', True)
ue.MaterialEditingLibrary.recompile_material(m)
ue.get_editor_subsystem(ue.EditorAssetSubsystem).save_loaded_asset(m)
ue.log('BENCHY_BUOY_MATERIAL_READY')
