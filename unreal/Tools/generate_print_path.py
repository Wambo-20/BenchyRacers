from pathlib import Path
import struct,zlib,numpy as np
ROOT=Path(__file__).resolve().parents[1]
b=(ROOT/'AssetSource/Meshes/SM_Benchy.fbx').read_bytes();version=struct.unpack_from('<I',b,23)[0];wide=version>=7500;hsize=25 if wide else 13
found={}
def parse(pos,depth=0):
    while pos+hsize<len(b):
        end,n,plen,namelen=struct.unpack_from('<QQQB' if wide else '<IIIB',b,pos)
        if end==0:return pos+hsize
        name=b[pos+hsize:pos+hsize+namelen].decode();p=pos+hsize+namelen;props=[]
        for _ in range(n):
            typ=chr(b[p]);p+=1
            if typ in 'YCFDIL':
                fmt={'Y':'h','C':'?','F':'f','D':'d','I':'i','L':'q'}[typ];v=struct.unpack_from('<'+fmt,b,p)[0];p+=struct.calcsize(fmt)
            elif typ in 'SR':
                length=struct.unpack_from('<I',b,p)[0];p+=4;v=b[p:p+length];p+=length
            else:
                length,enc,size=struct.unpack_from('<III',b,p);p+=12;raw=b[p:p+size];p+=size
                if enc:raw=zlib.decompress(raw)
                dtype={'f':'<f4','d':'<f8','l':'<i8','i':'<i4','b':'u1','c':'u1'}[typ];v=np.frombuffer(raw,dtype=dtype)
            props.append(v)
        if name in ['Vertices','PolygonVertexIndex'] and name not in found:found[name]=props[0]
        child=pos+hsize+namelen+plen
        if child<end-hsize:parse(child,depth+1)
        pos=end
    return pos
parse(27)
v=found['Vertices'].reshape(-1,3);idx=found['PolygonVertexIndex'];tri=[];poly=[]
for i in idx:
    poly.append(int(-i-1 if i<0 else i))
    if i<0:
        for k in range(1,len(poly)-1):tri.append([poly[0],poly[k],poly[k+1]])
        poly=[]
t=v[np.array(tri)];lo=v.min(0);hi=v.max(0);print('FBX',version,'bounds',lo,hi,'triangles',len(t))
# Outer perimeter of the actual horizontal mesh section, resampled for the moving nozzle.
rows=[];N=128;S=48
for k in range(N):
    z=lo[2]+(hi[2]-lo[2])*(k+.5)/N
    segments=[]
    active=t[(t[:,:,2].min(1)<=z)&(t[:,:,2].max(1)>z)]
    for a in active:
        points=[]
        for j in range(3):
            p,q=a[j],a[(j+1)%3]
            if (p[2]<=z<q[2]) or (q[2]<=z<p[2]):points.append(p+(q-p)*(z-p[2])/(q[2]-p[2]))
        if len(points)==2:segments.append((points[0][:2],points[1][:2]))
    coords={};edges={}
    def key(p):return tuple(np.round(p,5))
    for a,c in segments:
        ka,kc=key(a),key(c);coords[ka]=a;coords[kc]=c;edges.setdefault(ka,[]).append(kc);edges.setdefault(kc,[]).append(ka)
    visited=set();loops=[]
    for start in edges:
        if start in visited:continue
        path=[];cur=start;prev=None
        while cur not in visited:
            visited.add(cur);path.append(coords[cur]);nxt=[q for q in edges[cur] if q!=prev]
            if not nxt:break
            prev,cur=cur,nxt[0]
        if len(path)>2:loops.append(np.array(path))
    if not loops:rows.append(rows[-1] if rows else np.zeros((S,2)));continue
    loop=max(loops,key=lambda a:np.linalg.norm(np.roll(a,-1,axis=0)-a,axis=1).sum())
    length=np.linalg.norm(np.roll(loop,-1,axis=0)-loop,axis=1);cum=np.r_[0,np.cumsum(length)];total=cum[-1]
    samples=[]
    for distance in np.arange(S)*total/S:
        j=min(np.searchsorted(cum,distance,side='right')-1,len(loop)-1);p=loop[j]+(loop[(j+1)%len(loop)]-loop[j])*(distance-cum[j])/max(length[j],1e-9)
        samples.append((p-(lo[:2]+hi[:2])/2)/(hi[0]-lo[0]))
    rows.append(np.array(samples))
header='#pragma once\n#include "CoreMinimal.h"\n// Generated from the horizontal contours of SM_Benchy.fbx. XY is normalized by hull length.\nnamespace BenchyPrint {\ninline FVector2D Nozzle(float Progress,float Time,float Length){\nstatic const FVector2D Path[128][48]={\n'
for a in rows:header+='{'+','.join('FVector2D(%.5ff,%.5ff)'%(p[0],p[1]) for p in a)+'},\n'
header+='};\nint Layer=FMath::Clamp(FMath::FloorToInt(Progress*128),0,127);float Phase=FMath::Fmod(Time*4.5f,1.f)*48;int Point=FMath::FloorToInt(Phase);return FMath::Lerp(Path[Layer][Point],Path[Layer][(Point+1)%48],Phase-Point)*Length;\n}\n}\n'
(ROOT/'Source/BenchyRacers/BenchyPrintPath.h').write_text(header,encoding='utf-8')
