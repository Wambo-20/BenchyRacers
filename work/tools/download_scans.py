import pathlib,json,urllib.request,hashlib,concurrent.futures
root=pathlib.Path(__file__).resolve().parent/'raw-nature'
root.mkdir(exist_ok=True)
def fetch(url):
    return urllib.request.urlopen(urllib.request.Request(url,headers={'User-Agent':'BenchyRacers/0.4'}),timeout=120).read()
def asset(name):
    meta=json.loads(fetch('https://api.polyhaven.com/files/'+name))
    entry=meta['gltf']['2k']['gltf'];folder=root/name;folder.mkdir(exist_ok=True)
    files={name+'.gltf':entry,**entry['include']}
    for rel,data in files.items():
        dest=folder/rel;dest.parent.mkdir(parents=True,exist_ok=True)
        if dest.exists() and hashlib.md5(dest.read_bytes()).hexdigest()==data['md5']:continue
        blob=fetch(data['url']);assert hashlib.md5(blob).hexdigest()==data['md5'];dest.write_bytes(blob)
    doc=json.loads((folder/(name+'.gltf')).read_text())
    print(name,[(m.get('name'),len(m.get('primitives',[]))) for m in doc['meshes']],flush=True)
    return {'name':name,'license':'CC0','url':'https://polyhaven.com/a/'+name}
with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
    credits=list(pool.map(asset,['boulder_01','coast_rocks_02','island_tree_02','fern_02']))
(root/'credits.json').write_text(json.dumps(credits,indent=2))
