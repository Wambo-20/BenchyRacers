import unreal as ue
import json
from pathlib import Path
root=Path(ue.Paths.project_dir()).resolve()
plan=json.loads((root/'Validation/ElectricDreamsMeshes.json').read_text())
report={}
for mesh in plan:
    for path in mesh['materials']:
        if path in report:continue
        mi=ue.load_asset(path)
        textures={}
        for name in ue.MaterialEditingLibrary.get_texture_parameter_names(mi.get_base_material()):
            t=ue.MaterialEditingLibrary.get_material_instance_texture_parameter_value(mi,name)
            if t:textures[str(name)]=t.get_path_name()
        report[path]={'textures':textures,'blend':str(mi.get_blend_mode())}
(root/'Validation/IslandBindings.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
ue.log('BENCHY_ISLAND_BINDINGS_INSPECTED')
