"""Run in the Benchy Unreal editor after copying the selected sample packages."""
import unreal as ue
import json
from pathlib import Path
root=Path(ue.Paths.project_dir()).resolve()
plan=json.loads((root/'Validation/ElectricDreamsImport.json').read_text())
report=[]
for path in plan['packages']:
    if path.split('/')[-1].startswith('T_'):
        texture=ue.load_asset(path)
        if isinstance(texture,ue.Texture2D):
            texture.set_editor_property('max_texture_size',2048)
            ue.EditorAssetLibrary.save_loaded_asset(texture)
for path in plan['seeds']:
    if '/Surfaces/' in path:continue
    if 'HugeMossyForestCliff' in path:continue # Source mesh has no assigned material; not used.
    mesh=ue.load_asset(path)
    assert isinstance(mesh,ue.StaticMesh),path
    bounds=mesh.get_bounds()
    slots=mesh.get_editor_property('static_materials')
    materials=[]
    for slot in slots:
        material=slot.material_interface
        assert material,path
        base=material.get_base_material()
        base.set_editor_property('used_with_instanced_static_meshes',True)
        ue.MaterialEditingLibrary.recompile_material(base)
        ue.EditorAssetLibrary.save_loaded_asset(base)
        materials.append(material.get_path_name())
    entry={'path':path,'origin':[bounds.origin.x,bounds.origin.y,bounds.origin.z],
        'extent':[bounds.box_extent.x,bounds.box_extent.y,bounds.box_extent.z],
        'materials':materials,'nanite':mesh.get_editor_property('nanite_settings').enabled}
    report.append(entry)
    ue.log('BENCHY_SAMPLE_MESH '+json.dumps(entry))
(root/'Validation/ElectricDreamsMeshes.json').write_text(json.dumps(report,indent=2))
ue.log('BENCHY_SAMPLE_INSPECTED')
