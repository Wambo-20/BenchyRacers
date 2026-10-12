import unreal as ue
for path in ue.EditorAssetLibrary.list_assets('/Game/Materials',recursive=False):
    if '/M_Island_' not in path:continue
    material=ue.load_asset(path)
    material.set_editor_property('used_with_nanite',True)
    material.set_editor_property('used_with_instanced_static_meshes',True)
    ue.MaterialEditingLibrary.recompile_material(material)
    ue.EditorAssetLibrary.save_loaded_asset(material)
    assert material.get_editor_property('used_with_nanite')
ue.log('BENCHY_ISLAND_NANITE_READY')
