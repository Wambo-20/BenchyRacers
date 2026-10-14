from pathlib import Path
import json,zipfile,hashlib
root=Path.cwd();out=root/'outputs/BenchyRacers-v1.6.2'
old=json.loads((root/'work/layers17-payload-hashes.json').read_text());new=json.loads((root/'work/network18-payload-hashes.json').read_text())
def sha(p):
 h=hashlib.sha256()
 with p.open('rb') as f:
  for b in iter(lambda:f.read(1048576),b''):h.update(b)
 return h.hexdigest()
changed=[k for k,v in new.items() if old.get(k)!=v];assert not set(old)-set(new)
assert 'BenchyRacers/Config/BenchyEOS.ini' not in changed
patch=out/'BenchyRacers-1.6.1-auf-1.6.2-Update.zip'
with zipfile.ZipFile(out/'BenchyRacers-1.6.2-Portable.zip') as payload,zipfile.ZipFile(patch,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
 for name in changed:
  with payload.open(name) as src,z.open(name,'w',force_zip64=True) as dst:
   while b:=src.read(1048576):dst.write(b)
files=[{'path':k,'bytes':new[k][0],'sha256':new[k][1],'oldSha256':old.get(k,[None,None])[1]} for k in changed]
manifest={'schema':1,'version':'1.6.2','patches':[{'fromVersion':'1.6.1','url':'https://github.com/Wambo-20/BenchyRacers/releases/download/v1.6.2/'+patch.name,'bytes':patch.stat().st_size,'sha256':sha(patch),'files':files}]}
(out/'UpdateManifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
with zipfile.ZipFile(patch) as z:
 for f in files:
  with z.open(f['path']) as p:assert hashlib.file_digest(p,'sha256').hexdigest()==f['sha256']
(out/'SHA256.txt').write_text(''.join(sha(p)+'  '+p.name+'\n' for p in sorted(out.iterdir()) if p.suffix in {'.exe','.zip','.json'}),encoding='utf-8')
print('Verified delta:',len(files),'files;',patch.stat().st_size,'bytes')
