"""Generate closed rocks and a layered broadleaf tree; run with Blender."""
import bpy, math, random, json, sys
from pathlib import Path
from mathutils import Vector

root=Path(sys.argv[sys.argv.index('--')+1]).resolve()
destination=root/'AssetSource/Meshes'
report=[]

def clear():
    bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
    bpy.context.scene.unit_settings.scale_length=1

def export(name):
    obj=bpy.context.object
    obj.name=name
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    for v in obj.data.vertices:v.co*=100
    for p in obj.data.polygons:p.use_smooth='Rock' not in name
    bpy.context.scene.unit_settings.system='METRIC'
    bpy.context.scene.unit_settings.scale_length=.01
    bpy.ops.export_scene.fbx(filepath=str(destination/(name+'.fbx')),use_selection=True,
        object_types={'MESH'},axis_forward='X',axis_up='Z',apply_unit_scale=True,
        apply_scale_options='FBX_SCALE_UNITS',bake_anim=False)
    edge_uses={}
    for poly in obj.data.polygons:
        for a,b in poly.edge_keys:edge_uses[(a,b)]=edge_uses.get((a,b),0)+1
    closed=all(n==2 for n in edge_uses.values())
    if 'Rock' in name:assert closed,(name,'rock surface is open')
    entry={'name':name,'polygons':len(obj.data.polygons),'closed_surface':closed}
    report.append(entry);print('JUNGLE_ASSET',entry,flush=True)

for variant in [1,2]:
    clear()
    rng=random.Random(830+variant)
    # Fractured polygonal strata, not a smoothly displaced sphere.
    count=9;levels=[-1,-.58,-.55,-.04,0,.48,.52,1]
    verts=[];polys=[]
    azimuths=[k*math.tau/count+rng.uniform(-.09,.09) for k in range(count)]
    outline=[rng.uniform(.78,1.15) for k in range(count)]
    for j,z in enumerate(levels):
        radius=[.77,1.03,.90,.98,.86,.91,.78,.67][j]
        for k,a in enumerate(azimuths):
            r=radius*outline[k]+rng.uniform(-.045,.045)
            verts.append((math.cos(a)*r+.09*z,math.sin(a)*r-.07*z,z+rng.uniform(-.025,.025)))
    for j in range(len(levels)-1):
        for k in range(count):
            a=j*count+k;b=j*count+(k+1)%count;c=b+count;d=a+count
            polys.extend([(a,b,c),(a,c,d)])
    polys.extend([tuple(reversed(range(count))),tuple((len(levels)-1)*count+k for k in range(count))])
    mesh=bpy.data.meshes.new('FracturedStrata');mesh.from_pydata(verts,[],polys);mesh.update()
    obj=bpy.data.objects.new('FracturedRock',mesh);bpy.context.collection.objects.link(obj)
    bpy.context.view_layer.objects.active=obj;obj.select_set(True)
    export('SM_SolidRock'+str(variant))

clear()
rng=random.Random(4127)
vertices=[];faces=[];slots=[]

def tube(points,radii):
    offset=len(vertices)
    for j,p in enumerate(points):
        for k in range(8):
            angle=k*math.tau/8
            vertices.append(tuple(p+Vector((math.cos(angle)*radii[j],math.sin(angle)*radii[j],0))))
    for j in range(len(points)-1):
        for k in range(8):
            faces.append((offset+j*8+k,offset+j*8+(k+1)%8,offset+(j+1)*8+(k+1)%8,offset+(j+1)*8+k));slots.append(0)
    faces.extend([tuple(offset+k for k in reversed(range(8))),tuple(offset+(len(points)-1)*8+k for k in range(8))]);slots.extend([0,0])

tube([Vector((.15*math.sin(j*.35),.2*math.sin(j*.22),j*1.5)) for j in range(12)],
    [.36*(1-j/16) for j in range(12)])
for branch in range(12):
    angle=branch*math.tau/7
    radius=3.0+branch%3*.7
    base=Vector((0,0,9.2+branch%4*1.6))
    center=Vector((math.cos(angle)*radius,math.sin(angle)*radius,13+branch%4*1.05))
    tube([base,base.lerp(center,.45),center],[.15,.11,.025])
    for leaf in range(110):
        a=rng.random()*math.tau
        rr=math.sqrt(rng.random())*2.6
        p=center+Vector((math.cos(a)*rr,math.sin(a)*rr,rng.uniform(-.6,.95)))
        direction=Vector((math.cos(a),math.sin(a),rng.uniform(-.4,.45))).normalized()
        side=direction.cross(Vector((0,0,1))).normalized()
        length=rng.uniform(.33,.68);width=length*rng.uniform(.30,.46)
        idx=len(vertices)
        vertices.extend([tuple(p-direction*length),tuple(p+side*width),tuple(p+direction*length),
                         tuple(p-side*width),tuple(p+Vector((0,0,.10)))])
        faces.extend([(idx,idx+1,idx+4),(idx+1,idx+2,idx+4),(idx+2,idx+3,idx+4),(idx+3,idx,idx+4)])
        slots.extend([1]*4)
mesh=bpy.data.meshes.new('JungleTree');mesh.from_pydata(vertices,[],faces);mesh.update()
obj=bpy.data.objects.new('JungleTree',mesh);bpy.context.collection.objects.link(obj)
bpy.context.view_layer.objects.active=obj;obj.select_set(True)
for name in ['JungleBark','JungleLeaves']:
    m=bpy.data.materials.new(name);mesh.materials.append(m)
for p,s in zip(mesh.polygons,slots):p.material_index=s
export('SM_JungleTree')
manifest_path=root/'AssetSource/manifest.json'
manifest=json.loads(manifest_path.read_text())
for entry in report:
    if not any(m['name']==entry['name'] for m in manifest):
        manifest.append({'file':'Meshes/'+entry['name']+'.fbx','destination':'/Game/Nature','name':entry['name']})
manifest_path.write_text(json.dumps(manifest,indent=2))
(destination/'jungle-geometry-report.json').write_text(json.dumps(report,indent=2))
