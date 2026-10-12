"""Copy a selected dependency closure from the user's downloaded Epic sample.
Never modifies the sample, cache, or launcher installation.
"""
from pathlib import Path
import json, re, shutil, sys
root=Path(__file__).resolve().parents[1]
source=Path(r'C:\ProgramData\Epic\EpicGamesLauncher\VaultCache\ElectricDreamsSample_5.8\data\Content')
seeds=[
 'Megascans/3D_Plants/AlexandraPalm/SM_AlexandraPalm_01',
 'Megascans/3D_Plants/AlexandraPalm/SM_AlexandraPalm_02',
 'Megascans/3D_Plants/ArecaPalm/SM_ArecaPalm_01',

 'Megascans/Surfaces/JungleGround/T_JungleGround_01_BC',
 'Megascans/Surfaces/JungleGround/T_JungleGround_01_N',
 'Megascans/Surfaces/MossyGrass/T_MossyGrass_01_BC',
 'Megascans/Surfaces/MossyGrass/T_MossyGrass_01_N',

 'Megascans/3D_Assets/HugeNordicCoastalCliff/SM_HugeNordicCoastalCliff_01',
 'Megascans/3D_Assets/HugeNordicCoastalCliff/SM_HugeNordicCoastalCliff_02',
 'Megascans/3D_Assets/HugeMossyForestCliff/SM_HugeMossyForestCliff_01',
 'Megascans/3D_Assets/HugeSandstoneCliff/SM_HugeSandstoneCliff_01',
 'Megascans/3D_Assets/TundraMossyBoulder/SM_TundraMossyBoulder_01',
 'Megascans/3D_Assets/LichenedForestBoulder/SM_LichenedForestBoulder_01',
 'Megascans/3D_Plants/EuropeanHornbeam/Geometry/SimpleWind/SM_EuropeanHornbeam_Forest_01',
 'Megascans/3D_Plants/EuropeanHornbeam/Geometry/SimpleWind/SM_EuropeanHornbeam_Forest_03',
 'Megascans/3D_Plants/EuropeanHornbeam/Geometry/SimpleWind/SM_EuropeanHornbeam_Forest_06',
 'Megascans/3D_Plants/CanePalm/SM_CanePalm_01',
 'Megascans/3D_Plants/BroadleafLadyPalm/SM_BroadleafLadyPalm_01',
 'Megascans/3D_Plants/ZamiaPalm/SM_ZamiaPalm_01',
 'Megascans/3D_Plants/BostonFern/SM_BostonFern_01',
 'Megascans/3D_Plants/BeechFern/SM_BeechFern_03',
]
index={'/Game/'+p.relative_to(source).with_suffix('').as_posix():p for p in source.rglob('*.uasset')}
pending=['/Game/'+s for s in seeds];selected=set();unresolved=set()
while pending:
    name=pending.pop()
    if name in selected:continue
    if name not in index:unresolved.add(name);continue
    selected.add(name)
    data=index[name].read_bytes()
    references=set(x.decode('ascii') for x in re.findall(rb'/Game/[A-Za-z0-9_/-]+',data))
    wide=data.decode('utf-16-le',errors='ignore')
    references.update(re.findall(r'/Game/[A-Za-z0-9_/-]+',wide))
    for ref in references:
        if ref in index and ref not in selected:pending.append(ref)
        elif ref not in index and ref != '/Game/':unresolved.add(ref)
total=sum(index[n].stat().st_size for n in selected)
report={'source':'Electric Dreams Environment 5.8, Epic Games',
 'license':'Epic UE-only sample content; retain within Unreal Engine projects',
 'seeds':['/Game/'+s for s in seeds],'packages':sorted(selected),
 'package_count':len(selected),'uasset_bytes':total,'unresolved_name_references':sorted(unresolved)}
(root/'Validation').mkdir(exist_ok=True)
(root/'Validation/ElectricDreamsImport.json').write_text(json.dumps(report,indent=2))
print(json.dumps({'packages':len(selected),'megabytes':round(total/1024**2,1),
 'unresolved':sorted(unresolved)},indent=2),flush=True)
assert all('/Game/'+s in selected for s in seeds),'Selected source model missing'
if '--copy' in sys.argv or '--copy-missing' in sys.argv:
    for name in sorted(selected):
        src=index[name];target=root/'Content'/src.relative_to(source)
        target.parent.mkdir(parents=True,exist_ok=True)
        if '--copy-missing' in sys.argv and target.exists():continue
        for suffix in ['.uasset','.ubulk','.uexp']:
            component=src.with_suffix(suffix)
            if component.exists():shutil.copy2(component,target.with_suffix(suffix))
    print('ELECTRIC_DREAMS_SELECTED_FILES_COPIED',flush=True)
