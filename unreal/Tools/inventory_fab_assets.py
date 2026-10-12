"""Record downloaded Fab assets without changing the cache or importing into live maps."""
from pathlib import Path
import datetime,json

project=Path(__file__).resolve().parents[1]
vault=Path('C:/ProgramData/Epic/EpicGamesLauncher/VaultCache')
library=vault/'FabLibrary'
owned=[
    ('Environment - Rock Collection 04 (Free)','a51e61ac','all worlds, seven additional rock shapes'),
    ('Nordic Coastal Cliff','35d695d8','Frostfjord, rocky fjord shore'),
    ('Ice Cliff','198d6561','Frostfjord, ice walls'),
    ('Fresh Windswept Snow','6ac5c478','Frostfjord, snow-covered ground'),
    ('Wooden Planks','1f86b760','Kupferhafen, docks and buildings'),
    ('Rusty Metal Sheet','6e61dcd3','Kupferhafen, selected industrial details'),
    ('Huge Nordic Coastal Cliff (sloping variant)',None,'additional coastal cliffs'),
    ('Huge Nordic Coastal Cliff (column variant)',None,'additional coastal cliffs'),
    ('Snow Pile',None,'Frostfjord, snow banks'),
    ('Nordic Beach Rock Formation',None,'low coastal formations'),
    ('Quarry Cliff',None,'selected weathered cliff faces'),
    ('Beach Sand With Pebbles',None,'Sonnendeck, shoreline material'),
    ('Real Stones Pack 01',None,'small stones and shore variety'),
]
previous=project/'Validation/FabAssetInventory.json'
previous_report=json.loads(previous.read_text(encoding='utf-8-sig')) if previous.exists() else {}
placed={a['name']:a.get('placed_in_race_worlds',False) for a in previous_report.get('assets',[])}
assets=[]
for name,short_id,usage in owned:
    folders=list(library.glob('*-'+short_id)) if short_id else []
    files=[p for folder in folders for p in folder.rglob('*') if p.is_file() and p.name not in {'manifest','thumbnail.jpeg'}]
    if short_id=='a51e61ac':
        rock=vault/'Environm12b7b0efc617V1/data/Content/Rock_Collection_04'
        if rock.exists():
            files+=list(rock.rglob('*.uasset'));folders.append(rock)
    models=[p for p in files if p.suffix.lower()=='.fbx' or (p.suffix=='.uasset' and p.name.startswith('SM_Rock_'))]
    textures=[p for p in files if p.suffix.lower() in {'.jpg','.png','.exr','.tga'} or (p.suffix=='.uasset' and p.name.startswith('T_Rock_'))]
    assets.append({'name':name,'owned_in_fab_library':True,'download_verified':bool(models or textures),'placed_in_race_worlds':False,'planned_use':usage,'cache_locations':[str(p) for p in folders],'model_files':[p.name for p in models],'texture_count':len(textures),'downloaded_file_bytes':sum(p.stat().st_size for p in files)})
    if name=='Beach Sand With Pebbles':
        assets[-1]['download_blocker']='Launcher download dialog: selected UEFN Reference license excludes FBX/GLB/GLTF source downloads; obtain the Standard license variant before using in this native game.'
    assets[-1]['placed_in_race_worlds']=placed.get(name,False)
report={'checked_at_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'ownership_evidence':'Epic Games Launcher Fab library visually inspected; 14 entries including the existing Electric Dreams sample','integration_status':'Preparation only. Additional acquired assets are not imported or placed in the 1.4.1 race worlds. Original source assets are not redistributed in the project archive.','assets':assets}
if any(a['placed_in_race_worlds'] for a in assets):report['integration_status']=previous_report.get('integration_status','See native placement reports for actual integration.')
(project/'Validation/FabAssetInventory.json').write_text(json.dumps(report,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
print(json.dumps({'verified_downloads':[a['name'] for a in assets if a['download_verified']]},ensure_ascii=False))
