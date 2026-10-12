from pathlib import Path
import unreal as ue
root=Path(ue.Paths.project_dir()).resolve()
for path in sorted((root/'AssetSource'/'RaceSounds').glob('*.wav')):
    task=ue.AssetImportTask()
    task.filename=str(path);task.destination_path='/Game/Audio/Cues';task.destination_name=path.stem
    task.automated=True;task.replace_existing=True;task.save=True
    ue.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    asset=ue.load_asset('/Game/Audio/Cues/'+path.stem)
    assert asset, path
    asset.set_editor_property('looping',False)
    ue.EditorAssetLibrary.save_loaded_asset(asset)
ue.log('BENCHY_RACE_AUDIO_READY')
