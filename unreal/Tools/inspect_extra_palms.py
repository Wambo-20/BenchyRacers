import unreal as ue
import json
from pathlib import Path
root=Path(ue.Paths.project_dir()).resolve()
report=json.loads((root/'Validation/ElectricDreamsMeshes.json').read_text())
bindings=json.loads((root/'Validation/IslandBindings.json').read_text())
for kind,name in [('AlexandraPalm','01'),('AlexandraPalm','02'),('ArecaPalm','01')]:
    path='/Game/Megascans/3D_Plants/'+kind+'/SM_'+kind+'_'+name
    mesh=ue.load_asset(path);assert mesh,path
    bounds=mesh.get_bounds();materials=[]
    for slot in mesh.get_editor_property('static_materials'):
        mi=slot.material_interface;assert mi,path
        materials.append(mi.get_path_name())
        textures={}
        for key in ue.MaterialEditingLibrary.get_texture_parameter_names(mi.get_base_material()):
            tex=ue.MaterialEditingLibrary.get_material_instance_texture_parameter_value(mi,key)
            if tex:textures[str(key)]=tex.get_path_name()
        bindings[mi.get_path_name()]={'textures':textures,'blend':str(mi.get_blend_mode())}
    report=[r for r in report if r['path']!=path]
    row={'path':path,'origin':[bounds.origin.x,bounds.origin.y,bounds.origin.z],
        'extent':[bounds.box_extent.x,bounds.box_extent.y,bounds.box_extent.z],
        'materials':materials,'nanite':mesh.get_editor_property('nanite_settings').enabled}
    report.append(row);ue.log('BENCHY_PALM '+json.dumps(row))
(root/'Validation/ElectricDreamsMeshes.json').write_text(json.dumps(report,indent=2))
(root/'Validation/IslandBindings.json').write_text(json.dumps(bindings,indent=2))
ue.log('BENCHY_PALMS_INSPECTED')
