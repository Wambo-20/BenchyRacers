import pathlib,zipfile,hashlib,json,re
from PIL import Image
root=pathlib.Path(__file__).resolve().parents[2]
icon=Image.open(root/'work/game/assets/icon.png').convert('RGBA')
icon.save(root/'work/tools/icon.ico',sizes=[(16,16),(32,32),(48,48),(64,64),(128,128),(256,256)])
print('Created Windows icon')
game_dir=root/'outputs/BenchyRacers'
if (game_dir/'BenchyRacers.exe').exists() and (game_dir/'Uninstall.exe').exists():
    for dest in [root/'work/tools/payload.zip',root/'outputs/BenchyRacers-Portable.zip']:
        with zipfile.ZipFile(dest,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as archive:
            for p in sorted(game_dir.iterdir()):
                if p.is_file():archive.write(p,p.name)
        with zipfile.ZipFile(dest) as archive:
            assert archive.testzip() is None
        print(dest.name, dest.stat().st_size)
    with zipfile.ZipFile(root/'outputs/BenchyRacers-Quellcode.zip','w',zipfile.ZIP_DEFLATED) as archive:
        for p in sorted((root/'work/game').rglob('*')):
            if not p.is_file() or '.godot' in p.parts:continue
            rel=p.relative_to(root/'work/game')
            if rel.name=='export_presets.cfg':
                text=re.sub(r'custom_template/release="[^"]*"', 'custom_template/release=""', p.read_text(encoding='utf-8'))
                text=text.replace('../../outputs/BenchyRacers/BenchyRacers.exe','build/BenchyRacers.exe')
                archive.writestr('BenchyRacers/'+str(rel).replace('\\','/'),text)
            else:archive.write(p,'BenchyRacers/'+str(rel).replace('\\','/'))
        for p in ['Installer.cs','convert_benchy.py','render_icon.py','compose_soundtrack.py','download_nature.py','download_scans.py','optimize_scans.py']:
            archive.write(root/'work/tools'/p,'Build-Werkzeuge/'+p)
        archive.write(game_dir/'LIESMICH.txt','LIESMICH.txt')
    manifest={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in game_dir.iterdir() if p.is_file()}
    (root/'work/payload-sha256.json').write_text(json.dumps(manifest,indent=2))
