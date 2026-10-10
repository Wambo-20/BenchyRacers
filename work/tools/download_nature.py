import urllib.request,json,pathlib,hashlib,concurrent.futures
root=pathlib.Path(__file__).resolve().parents[1]/'game/assets/nature'
root.mkdir(parents=True,exist_ok=True)
choices={'sand':'coast_sand_01','grass':'grass_ground','rock':'rock_boulder_dry','bark':'bark_brown_02'}
def download(pair):
    prefix,asset=pair
    request=urllib.request.Request('https://api.polyhaven.com/files/'+asset,headers={'User-Agent':'BenchyRacers/0.2'})
    meta=json.load(urllib.request.urlopen(request,timeout=45))
    result=[]
    for key,suffix in [('Diffuse','color'),('nor_gl','normal'),('Rough','rough')]:
        entry=meta[key]['1k']['jpg']
        data=urllib.request.urlopen(entry['url'],timeout=90).read()
        assert hashlib.md5(data).hexdigest()==entry['md5']
        path=root/(prefix+'_'+suffix+'.jpg');path.write_bytes(data)
        result.append(path.name)
    print(asset,', '.join(result),flush=True)
    return {'name':asset,'url':'https://polyhaven.com/a/'+asset,'license':'CC0'}
with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:credits=list(pool.map(download,choices.items()))
(root/'credits.json').write_text(json.dumps(credits,indent=2))
