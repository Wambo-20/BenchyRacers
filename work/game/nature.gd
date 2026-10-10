extends RefCounted
class_name NatureKit

var world:RaceWorld
var rng:RandomNumberGenerator
var terrain_material:ShaderMaterial
var leaf_material:ShaderMaterial
var wood_material:StandardMaterial3D
var rock_material:StandardMaterial3D
var palm_meshes:Array=[]
var bush_mesh:ArrayMesh
var islands:Array=[]
var tree_batches:Dictionary={}
var shrub_transforms:Array[Transform3D]=[]
var canopy_transforms:Array[Transform3D]=[]
var shore_segments:Array=[]
var scan_parts:Dictionary={}
var rock_transforms:Array[Transform3D]=[]

func setup(parent:RaceWorld)->void:
    world=parent;rng=parent.rng
    terrain_material=ShaderMaterial.new();terrain_material.shader=preload("res://terrain.gdshader")
    for pair in [["sand_albedo","sand_color"],["grass_albedo","grass_color"],["rock_albedo","rock_color"],["sand_normal","sand_normal"],["grass_normal","grass_normal"]]:
        terrain_material.set_shader_parameter(pair[0],load("res://assets/nature/"+pair[1]+".jpg"))
    leaf_material=ShaderMaterial.new();leaf_material.shader=preload("res://foliage.gdshader")
    leaf_material.set_shader_parameter("leaf_color",Color("488229"))
    wood_material=StandardMaterial3D.new();wood_material.albedo_texture=preload("res://assets/nature/bark_color.jpg")
    wood_material.normal_enabled=true;wood_material.normal_texture=preload("res://assets/nature/bark_normal.jpg")
    wood_material.roughness_texture=preload("res://assets/nature/bark_rough.jpg");wood_material.roughness=0.95
    rock_material=StandardMaterial3D.new();rock_material.albedo_texture=preload("res://assets/nature/rock_color.jpg")
    rock_material.normal_enabled=true;rock_material.normal_texture=preload("res://assets/nature/rock_normal.jpg")
    rock_material.uv1_triplanar=true;rock_material.uv1_world_triplanar=true;rock_material.uv1_scale=Vector3(0.17,0.17,0.17)
    rock_material.roughness=0.9
    for i in range(4):palm_meshes.append(_palm_mesh(i));tree_batches[i]=[]
    bush_mesh=_bush_mesh()
    for name in ["boulder_01","coast_rocks_02","island_tree_02","fern_02"]:
        var scene=load("res://assets/scans/"+name+".glb").instantiate()
        scan_parts[name]=[];_scan_parts(scene,Transform3D.IDENTITY,scan_parts[name]);scene.free()
        if name in ["boulder_01","coast_rocks_02"]:
            for part in scan_parts[name]:
                var mesh:Mesh=part.mesh
                for surface in range(mesh.get_surface_count()):
                    var old=mesh.surface_get_material(surface) as StandardMaterial3D
                    if old:
                        var stone=ShaderMaterial.new();stone.shader=preload("res://scanned_rock.gdshader")
                        stone.set_shader_parameter("albedo_tex",old.albedo_texture);stone.set_shader_parameter("normal_tex",old.normal_texture)
                        mesh.surface_set_material(surface,stone)

func _scan_parts(node:Node,parent:Transform3D,parts:Array)->void:
    var local=parent*node.transform if node is Node3D else parent
    if node is MeshInstance3D:parts.append({"mesh":node.mesh,"transform":local})
    for child in node.get_children():_scan_parts(child,local,parts)

func _scan_batch(name:String,transforms:Array)->void:
    for part in scan_parts[name]:
        var combined:Array=[]
        for transform in transforms:combined.append(transform*part.transform)
        _batch(part.mesh,combined)

func build_tropical()->void:
    var centroid=Vector2.ZERO
    for p in world.samples:centroid+=Vector2(p.x,p.z)
    centroid/=world.samples.size()
    var coast:PackedVector2Array=PackedVector2Array()
    for i in range(0,world.samples.size(),4):
        var p=Vector2(world.samples[i].x,world.samples[i].z)
        var inward=(centroid-p).normalized()
        coast.append(p+inward*(world.track_width*0.6+13+sin(float(i)*0.19)*4))
    _island(centroid,coast,48.0,340,1300)
    # Outer islands form channels and headlands around the entire racing route.
    for index in [12,74,122,188,245,292]:
        var cp=world.samples[index%world.samples.size()]
        var outward=(Vector2(cp.x,cp.z)-centroid).normalized()
        var radius=rng.randf_range(30,58)
        var center=Vector2(cp.x,cp.z)+outward*(radius+world.track_width*0.6+18)
        var outline=PackedVector2Array()
        for k in range(48):
            var a=float(k)/48*TAU
            var r=radius*(1+sin(a*3+index)*0.13+sin(a*7)*0.035)
            outline.append(center+Vector2(cos(a),sin(a))*r)
        _island(center,outline,rng.randf_range(28,48),int(radius*1.1),int(radius*3.5))
    # A distant archipelago keeps the horizon populated without blocking the route.
    for k in range(7):
        var a=float(k)/7*TAU+0.4;var center=Vector2(cos(a),sin(a))*rng.randf_range(560,780)
        var outline=PackedVector2Array();var radius=rng.randf_range(65,110)
        for j in range(48):
            var angle=float(j)/48*TAU
            outline.append(center+Vector2(cos(angle),sin(angle))*radius*(1+sin(angle*4+k)*0.12))
        _island(center,outline,75+rng.randf()*65,30,35)
    _cliff_groups(centroid)
    _seabed()
    _flush_instances()
    _shore_foam()
    _coastal_details()

func _height(ratio:float,p:Vector2,peak:float)->float:
    var beach=1.0-smoothstep(0.78,1.0,ratio)
    var hills=(sin(p.x*0.035+p.y*0.022)*0.5+0.5)*0.55+(sin(p.y*0.051-p.x*0.019)*0.5+0.5)*0.45
    var rugged=sin(p.x*0.077+sin(p.y*0.044)*1.8)*sin(p.y*0.067-p.x*0.013)*0.16
    var inland=pow(maxf(0,1-ratio),0.67)*peak*(0.38+hills*0.7+rugged)
    return -0.62+beach*2.8+inland

func _island(center:Vector2,coast:PackedVector2Array,peak:float,trees:int,shrubs:int)->void:
    var island={"center":center,"coast":coast,"peak":peak,"angles":[],"radii":[]}
    for p in coast:
        island.angles.append(atan2(p.y-center.y,p.x-center.x))
        island.radii.append(p.distance_to(center))
    islands.append(island)
    var st=SurfaceTool.new();st.begin(Mesh.PRIMITIVE_TRIANGLES);st.set_smooth_group(0)
    var rings=[0.0,0.06,0.13,0.21,0.30,0.40,0.50,0.60,0.68,0.76,0.83,0.89,0.94,0.98,1.02,1.08]
    var verts:Array=[]
    for ratio in rings:
        var ring:Array[Vector3]=[]
        for cp in coast:
            var p=center+(cp-center)*ratio
            ring.append(Vector3(p.x,_height(ratio,p,peak),p.y))
        verts.append(ring)
    for r in range(rings.size()-1):
        for i in range(coast.size()):
            var j=(i+1)%coast.size()
            var a:Vector3=verts[r][i];var b:Vector3=verts[r][j];var c:Vector3=verts[r+1][i];var d:Vector3=verts[r+1][j]
            for v in [a,c,b,b,c,d]:
                st.set_uv(Vector2(v.x,v.z)*0.16);st.add_vertex(v)
    st.index();st.generate_normals();st.generate_tangents()
    var terrain=MeshInstance3D.new();terrain.mesh=st.commit();terrain.material_override=terrain_material
    world.add_child(terrain)
    var bounds=Rect2(coast[0],Vector2.ZERO)
    for p in coast:bounds=bounds.expand(p)
    for k in range(trees):
        var p=center
        for attempt in range(20):
            p=Vector2(rng.randf_range(bounds.position.x,bounds.end.x),rng.randf_range(bounds.position.y,bounds.end.y))
            if Geometry2D.is_point_in_polygon(p,coast) and _ratio(p,island)<0.91:break
        var ratio=_ratio(p,island)
        if ratio>0.92:continue
        var h=_height(ratio,p,peak)
        var variant=k%4;var size=rng.randf_range(0.66,1.22)
        var basis=Basis(Vector3.UP,rng.randf()*TAU).scaled(Vector3(size,size*rng.randf_range(0.9,1.1),size))
        tree_batches[variant].append(Transform3D(basis,Vector3(p.x,h-0.12,p.y)))
    for k in range(shrubs):
        var p=Vector2(rng.randf_range(bounds.position.x,bounds.end.x),rng.randf_range(bounds.position.y,bounds.end.y))
        if not Geometry2D.is_point_in_polygon(p,coast):continue
        var ratio=_ratio(p,island)
        if ratio>0.91:continue
        var size=rng.randf_range(0.8,2.2)
        shrub_transforms.append(Transform3D(Basis(Vector3.UP,rng.randf()*TAU).scaled(Vector3.ONE*size),Vector3(p.x,_height(ratio,p,peak),p.y)))
        if k%3==0 and ratio<0.78:
            canopy_transforms.append(Transform3D(Basis(Vector3.UP,rng.randf()*TAU).scaled(Vector3.ONE*rng.randf_range(0.9,1.7)),Vector3(p.x,_height(ratio,p,peak),p.y)))
    for k in range(mini(trees/2,48)):
        var idx=rng.randi_range(0,coast.size()-1)
        var p=center+(coast[idx]-center)*rng.randf_range(0.84,0.98)
        var ratio=_ratio(p,island)
        var size=rng.randf_range(1.6,5.5)
        _boulder(Vector3(p.x,_height(ratio,p,peak)+size*0.15,p.y),size)
    for i in range(coast.size()):shore_segments.append([coast[i],coast[(i+1)%coast.size()]])

func _ratio(p:Vector2,island:Dictionary)->float:
    var center:Vector2=island.center;var delta=p-center
    var angle=atan2(delta.y,delta.x);var best=10.0;var radius=100.0
    for i in range(island.angles.size()):
        var diff=absf(wrapf(angle-island.angles[i],-PI,PI))
        if diff<best:best=diff;radius=island.radii[i]
    return delta.length()/radius

func ground_height(p:Vector2)->float:
    for island in islands:
        if Geometry2D.is_point_in_polygon(p,island.coast):return _height(_ratio(p,island),p,island.peak)
    return -10

func _vertex(st:SurfaceTool,p:Vector3,uv:Vector2,color:Color)->void:
    st.set_uv(uv);st.set_color(color);st.add_vertex(p)

func _blade(st:SurfaceTool,base:Vector3,tip:Vector3,width:float,color:Color)->void:
    var dir=(tip-base).normalized();var lateral=dir.cross(Vector3.UP).normalized()*width
    var mid=base.lerp(tip,0.46)+Vector3(0,0.09,0)
    var points=[base,mid-lateral,mid+lateral,tip]
    for tri in [[0,1,2],[1,3,2]]:
        for index in tri:
            var uv=[Vector2(0.5,0),Vector2(0,0.46),Vector2(1,0.46),Vector2(0.5,1)][index]
            _vertex(st,points[index],uv,color)

func _palm_mesh(variant:int)->Dictionary:
    var height=[16.0,18.5,14.0,20.0][variant]
    var bend=[1.1,-1.3,1.9,-0.7][variant]
    var trunk=SurfaceTool.new();trunk.begin(Mesh.PRIMITIVE_TRIANGLES)
    var rings=23;var radial=14
    for k in range(rings):
        for j in range(radial):
            var quad:Array=[]
            for pair in [[k,j],[k+1,j],[k,j+1],[k+1,j+1]]:
                var t=float(pair[0])/rings;var a=float(pair[1])/radial*TAU
                var radius=(0.39-0.15*t)*(1+sin(t*height*17)*0.04)
                var p=Vector3(cos(a)*radius+bend*t*t,height*t,sin(a)*radius)
                quad.append([p,Vector2(float(pair[1])/radial*2,t*height*0.7),Vector3(cos(a),0,sin(a))])
            for idx in [0,1,2,2,1,3]:
                trunk.set_uv(quad[idx][1]);trunk.set_normal(quad[idx][2]);trunk.add_vertex(quad[idx][0])
    trunk.generate_tangents();var trunk_mesh=trunk.commit();trunk_mesh.surface_set_material(0,wood_material)
    var leaves=SurfaceTool.new();leaves.begin(Mesh.PRIMITIVE_TRIANGLES)
    var top=Vector3(bend,height,0)
    for frond in range(17):
        var angle=float(frond)/17*TAU+variant*0.27
        var axis=Vector3(sin(angle),0,cos(angle));var side=Vector3(cos(angle),0,-sin(angle))
        var length=5.3+(frond%4)*0.48
        var droop=2.3 if frond<11 else 0.3
        for k in range(27):
            var t=float(k)/27
            var along=top+axis*t*length+Vector3(0,sin(t*PI)*1.5-t*droop,0)
            var leaflet_len=(0.3+sin(t*PI)*1.52)*(0.92+sin(k*2.7)*0.08)
            var shade=0.8+float((frond+k)%5)*0.05
            for s in [-1,1]:
                var tip=along+side*s*leaflet_len+axis*(0.25+t*0.25)+Vector3(0,-0.10-t*0.28,0)
                _blade(leaves,along,tip,0.14+sin(t*PI)*0.10,Color(shade,shade,shade,1))
    leaves.generate_normals();var leaf_mesh=leaves.commit();leaf_mesh.surface_set_material(0,leaf_material)
    return {"trunk":trunk_mesh,"leaves":leaf_mesh}

func _bush_mesh()->ArrayMesh:
    var st=SurfaceTool.new();st.begin(Mesh.PRIMITIVE_TRIANGLES)
    for i in range(28):
        var angle=float(i)*2.4;var height=0.18+float(i%5)*0.16
        var base=Vector3(sin(angle)*0.16,height*0.2,cos(angle)*0.16)
        var tip=Vector3(sin(angle)*(0.9+height*.3),height,cos(angle)*(0.9+height*.3))
        var shade=0.72+float(i%4)*0.09
        _blade(st,base,tip,0.16,Color(shade,shade,shade,1))
    st.generate_normals();var mesh=st.commit();mesh.surface_set_material(0,leaf_material);return mesh

func _batch(mesh:Mesh,transforms:Array)->void:
    if transforms.is_empty():return
    var cells:Dictionary={}
    for transform in transforms:
        var cell=Vector2i(int(floor(transform.origin.x/110.0)),int(floor(transform.origin.z/110.0)))
        if not cells.has(cell):cells[cell]=[]
        cells[cell].append(transform)
    for cell_transforms in cells.values():
        var multi=MultiMesh.new();multi.transform_format=MultiMesh.TRANSFORM_3D;multi.mesh=mesh;multi.instance_count=cell_transforms.size()
        for i in range(cell_transforms.size()):multi.set_instance_transform(i,cell_transforms[i])
        var instance=MultiMeshInstance3D.new();instance.multimesh=multi;instance.visibility_range_end=800;world.add_child(instance)

func _jungle_mesh()->ArrayMesh:
    var st=SurfaceTool.new();st.begin(Mesh.PRIMITIVE_TRIANGLES)
    for i in range(220):
        var angle=float(i)*2.39996
        var level=float(i%17)/17
        var radius=sin(level*PI)*3.5
        var p=Vector3(sin(angle)*radius,2.0+level*5.5,cos(angle)*radius)
        var direction=Vector3(sin(angle),0.15+level*0.4,cos(angle))
        var shade=0.62+float(i%7)*0.045
        _blade(st,p-direction*0.45,p+direction*1.0,0.44,Color(shade,shade,shade,1))
    st.generate_normals();var mesh=st.commit();mesh.surface_set_material(0,leaf_material);return mesh

func _flush_instances()->void:
    for variant in tree_batches:
        _batch(palm_meshes[variant].trunk,tree_batches[variant]);_batch(palm_meshes[variant].leaves,tree_batches[variant])
    _scan_batch("fern_02",shrub_transforms)
    var trees:Array=[]
    for transform in canopy_transforms:
        var tree:Transform3D=transform;tree.basis=tree.basis.scaled(Vector3.ONE*2.2);trees.append(tree)
    _scan_batch("island_tree_02",trees)
    _scan_batch("boulder_01",rock_transforms)

func _boulder(pos:Vector3,size:float)->void:
    var basis=Basis.from_euler(Vector3(rng.randf()*0.3,rng.randf()*TAU,rng.randf()*0.2))
    basis=basis.scaled(Vector3(size*1.4,size*rng.randf_range(1.1,1.8),size*1.3))
    rock_transforms.append(Transform3D(basis,pos-Vector3(0,size*0.3,0)))

func _cliff_groups(center:Vector2)->void:
    var coast_patches:Array=[]
    for index in range(18,world.samples.size(),38):
        var cp=world.samples[index];var inward=(Vector3(center.x,0,center.y)-cp).normalized()
        var p=cp+inward*58;var h=ground_height(Vector2(p.x,p.z))
        for k in range(3):
            var offset=Vector3(rng.randf_range(-7,7),0,rng.randf_range(-7,7))
            var scale=Vector3(rng.randf_range(12,19),rng.randf_range(20,38),rng.randf_range(12,19))
            rock_transforms.append(Transform3D(Basis(Vector3.UP,rng.randf()*TAU).scaled(scale),Vector3(p.x,h-3,p.z)+offset))
        var shore=cp+inward*29
        var rotation=atan2(-inward.x,-inward.z)
        coast_patches.append(Transform3D(Basis(Vector3.UP,rotation).scaled(Vector3(0.6,1.0,0.6)),shore-Vector3(0,1.7,0)))
    _scan_batch("coast_rocks_02",coast_patches)

func _seabed()->void:
    var mesh=PlaneMesh.new();mesh.size=Vector2(1800,1800);mesh.subdivide_width=140;mesh.subdivide_depth=140
    var arrays=mesh.get_mesh_arrays();var points:PackedVector3Array=arrays[Mesh.ARRAY_VERTEX]
    for i in range(points.size()):
        var p=points[i]
        var distance=world.nearest(Vector3(p.x,0,p.z)).distance
        p.y=-3.8-smoothstep(28.0,240.0,distance)*28.0+sin(p.x*0.053)*sin(p.z*0.046)*0.65
        points[i]=p
    arrays[Mesh.ARRAY_VERTEX]=points
    var surface=ArrayMesh.new();surface.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES,arrays)
    var material=ShaderMaterial.new();material.shader=preload("res://seabed.gdshader")
    material.set_shader_parameter("sand",preload("res://assets/nature/sand_color.jpg"));material.set_shader_parameter("normal_map",preload("res://assets/nature/sand_normal.jpg"))
    var bottom=MeshInstance3D.new();bottom.mesh=surface;bottom.material_override=material;bottom.layers=4;world.add_child(bottom)

func _shore_foam()->void:
    var st=SurfaceTool.new();st.begin(Mesh.PRIMITIVE_TRIANGLES)
    for segment in shore_segments:
        var a:Vector2=segment[0];var b:Vector2=segment[1]
        var d=(b-a).normalized();var side=Vector2(-d.y,d.x)*1.8
        var points=[a-side,b-side,a+side,b+side]
        for i in [0,1,2,2,1,3]:
            st.set_uv([Vector2(0,0),Vector2(0,1),Vector2(1,0),Vector2(1,1)][i])
            st.add_vertex(Vector3(points[i].x,0.035,points[i].y))
    var material=ShaderMaterial.new();material.shader=preload("res://shore.gdshader")
    var n=MeshInstance3D.new();n.mesh=st.commit();n.material_override=material;n.cast_shadow=GeometryInstance3D.SHADOW_CASTING_SETTING_OFF;world.add_child(n)

func _coastal_details()->void:
    # A populated starting cove with a wooden pier and weathered beach huts.
    var p=world.samples[0]
    var center:Vector2=islands[0].center
    var toward=(Vector3(center.x,0,center.y)-p).normalized()
    var side=Vector3(-toward.z,0,toward.x)
    var pier=Node3D.new();pier.position=p+toward*21;world.add_child(pier)
    pier.rotation.y=atan2(-toward.x,-toward.z)
    for i in range(25):world.box(Vector3(0,1.15,i*0.8-9),Vector3(5,0.16,0.7),Color("836e4e"),pier)
    for z in [-8,0,8]:
        for x in [-2.2,2.2]:world.cylinder(Vector3(x,-0.4,z),0.21,4,Color("574430"),pier)
    for i in range(5):
        var hp=p+toward*45+side*(i-2)*12
        var h=ground_height(Vector2(hp.x,hp.z))
        if h<0:continue
        var hut=Node3D.new();hut.position=Vector3(hp.x,h,hp.z);world.add_child(hut)
        world.box(Vector3(0,2,0),Vector3(6,4,5),Color("987e52"),hut)
        var roof=PrismMesh.new();roof.size=Vector3(8,2.6,7)
        world.shape(roof,Vector3(0,5,0),world.mat(Color("635c36")),hut)
        world.box(Vector3(0,2,2.51),Vector3(1.7,2.8,0.05),Color("28332a"),hut)
        for x in [-2,2]:world.box(Vector3(x,2.4,2.53),Vector3(1,1.1,0.05),Color("1c383b"),hut)
