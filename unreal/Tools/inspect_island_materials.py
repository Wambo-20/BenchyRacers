import unreal as ue
import json
from pathlib import Path
root=Path(ue.Paths.project_dir()).resolve()
out=root/'Validation/IslandMaterials.json'
rows=[]
paths=[p for p in ue.EditorAssetLibrary.list_assets('/Game/Megascans',recursive=True) if '/T_' in p]
for path in paths:
    t=ue.load_asset(path)
    if isinstance(t,ue.Texture2D):
        rows.append({'path':path,'srgb':t.get_editor_property('srgb'),
            'virtual':t.get_editor_property('virtual_texture_streaming'),
            'compression':str(t.get_editor_property('compression_settings'))})
out.write_text(json.dumps(rows,indent=2),encoding='utf-8')
ue.log('BENCHY_ISLAND_MATERIALS_INSPECTED '+str(len(rows)))
