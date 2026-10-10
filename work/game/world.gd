extends Node3D
class_name RaceWorld

const MAP_NAMES=["Sonnendeck", "Frostfjord", "Neonhafen"]
const MAP_DESCRIPTIONS=["Türkises Wasser · Palmen · weite Kurven", "Eisberge · kaltes Wasser · enge Passagen", "Nachtlichter · Container · schnelle Geraden"]
var map_id:int=0
var samples:PackedVector3Array=PackedVector3Array()
var gates:PackedVector3Array=PackedVector3Array()
var gate_nodes:Array[Node3D]=[]
var track_width:float=22.0
var track_length:float=0.0
var rng=RandomNumberGenerator.new()
var water:MeshInstance3D
var distant_water_material:ShaderMaterial
var environment:WorldEnvironment
var clock:float=0
var wave_scale:float=1.0
var external_clock:bool=false
var materials:Dictionary={}
var nature_kit:NatureKit
var weather:CPUParticles3D

func build(index:int)->void:
    map_id=index;rng.seed=8811+index*992
    wave_scale=1.5 if map_id==1 else 1.0
    track_width=[24.0,21.0,22.0][index]
    _route()
    _lighting()
    _sea()
    _buoys()
    _scenery()

func mat(col:Color,rough:float=0.85,glow:float=0.0)->StandardMaterial3D:
    var key=str(col)+str(rough)+str(glow)
    if materials.has(key):return materials[key]
    var m=StandardMaterial3D.new()
    m.albedo_color=col;m.roughness=rough
    if glow>0:m.emission_enabled=true;m.emission=col;m.emission_energy_multiplier=glow
    materials[key]=m
    return m

func shape(mesh:Mesh,pos:Vector3,material:Material,parent:Node=null)->MeshInstance3D:
    var n=MeshInstance3D.new();n.mesh=mesh;n.material_override=material;n.position=pos
    (parent if parent else self).add_child(n)
    return n

func box(pos:Vector3,size:Vector3,color:Color,parent:Node=null,glow:float=0.0)->MeshInstance3D:
    var m=BoxMesh.new();m.size=size
    return shape(m,pos,mat(color,0.75,glow),parent)

func cylinder(pos:Vector3,radius:float,height:float,color:Color,parent:Node=null,top:float=-1)->MeshInstance3D:
    var m=CylinderMesh.new();m.bottom_radius=radius;m.top_radius=top if top>=0 else radius;m.height=height;m.radial_segments=10
    return shape(m,pos,mat(color),parent)

func _route()->void:
    var sets=[
        [Vector3(-85,0,90),Vector3(-175,0,30),Vector3(-170,0,-80),Vector3(-70,0,-140),Vector3(25,0,-110),Vector3(85,0,-175),Vector3(175,0,-100),Vector3(180,0,5),Vector3(110,0,85),Vector3(20,0,135)],
        [Vector3(-75,0,130),Vector3(-140,0,35),Vector3(-110,0,-60),Vector3(-175,0,-140),Vector3(-65,0,-205),Vector3(20,0,-155),Vector3(40,0,-40),Vector3(140,0,-85),Vector3(180,0,35),Vector3(100,0,145),Vector3(15,0,90)],
        [Vector3(-120,0,100),Vector3(-180,0,0),Vector3(-160,0,-145),Vector3(-30,0,-150),Vector3(0,0,-60),Vector3(110,0,-125),Vector3(180,0,-35),Vector3(145,0,100),Vector3(35,0,150),Vector3(-25,0,65)]
    ]
    var points:Array=sets[map_id]
    for i in range(points.size()):
        var p0:Vector3=points[(i-1+points.size())%points.size()]
        var p1:Vector3=points[i]
        var p2:Vector3=points[(i+1)%points.size()]
        var p3:Vector3=points[(i+2)%points.size()]
        for j in range(32):
            var t=float(j)/32
            var p=0.5*((2*p1)+(-p0+p2)*t+(2*p0-5*p1+4*p2-p3)*t*t+(-p0+3*p1-3*p2+p3)*t*t*t)
            samples.append(p)
    for i in range(samples.size()):track_length+=samples[i].distance_to(samples[(i+1)%samples.size()])
    for i in range(16):gates.append(samples[int(float(i)/16*samples.size())])

func nearest(pos:Vector3,hint:int=-1)->Dictionary:
    var best=INF;var idx=0
    if hint>=0:
        for o in range(-22,23):
            var k=posmod(hint+o,samples.size())
            var dist=pos.distance_squared_to(samples[k])
            if dist<best:best=dist;idx=k
        if best<2500:return {"distance":sqrt(best),"index":idx}
    for k in range(samples.size()):
        var dist=pos.distance_squared_to(samples[k])
        if dist<best:best=dist;idx=k
    return {"distance":sqrt(best),"index":idx}

func tangent(idx:int)->Vector3:
    return (samples[(idx+1)%samples.size()]-samples[posmod(idx-1,samples.size())]).normalized()

func _lighting()->void:
    environment=WorldEnvironment.new()
    var env=Environment.new()
    env.background_mode=Environment.BG_SKY
    var sky=Sky.new();var sm=ProceduralSkyMaterial.new()
    if map_id==0:
        sm.sky_top_color=Color("328ac8");sm.sky_horizon_color=Color("bfe7ed")
        sm.ground_bottom_color=Color("046f7d");sm.ground_horizon_color=Color("bfe7ed")
        env.ambient_light_color=Color("bfe8ee");env.ambient_light_energy=0.40
        env.fog_light_color=Color("b2d7e4");env.fog_density=0.00032
    elif map_id==1:
        sm.sky_top_color=Color("547da8");sm.sky_horizon_color=Color("d9e7f0")
        sm.ground_bottom_color=Color("173e57");sm.ground_horizon_color=Color("d9e7f0")
        env.ambient_light_color=Color("bedeee");env.ambient_light_energy=0.50
        env.fog_light_color=Color("bccfdd");env.fog_density=0.0007
    else:
        sm.sky_top_color=Color("030614");sm.sky_horizon_color=Color("18284e")
        sm.ground_bottom_color=Color("020c21");sm.ground_horizon_color=Color("152845")
        env.ambient_light_color=Color("96b9e2");env.ambient_light_energy=0.38
        env.fog_light_color=Color("111c3c");env.fog_density=0.0009
    var sky_shader=ShaderMaterial.new();sky_shader.shader=preload("res://sky.gdshader")
    sky_shader.set_shader_parameter("top_color",sm.sky_top_color)
    sky_shader.set_shader_parameter("horizon_color",sm.sky_horizon_color)
    sky_shader.set_shader_parameter("night",1.0 if map_id==2 else 0.0)
    sky.sky_material=sky_shader;env.sky=sky
    env.fog_sky_affect=0.35
    env.ambient_light_source=Environment.AMBIENT_SOURCE_COLOR
    env.tonemap_mode=Environment.TONE_MAPPER_ACES
    env.tonemap_exposure=0.80
    env.ssr_enabled=map_id!=0
    env.ssr_max_steps=48
    env.glow_enabled=true
    env.glow_intensity=0.6
    env.glow_bloom=0.08
    env.ssao_enabled=true
    env.ssao_radius=2.0
    env.ssao_intensity=1.4
    env.adjustment_enabled=true
    env.adjustment_contrast=1.08
    env.adjustment_saturation=1.12
    env.ssil_enabled=true
    env.ssil_intensity=0.45
    env.fog_enabled=true
    environment.environment=env;add_child(environment)
    var sun=DirectionalLight3D.new()
    sun.rotation_degrees=Vector3(-45,-30,0)
    sun.light_energy=1.45 if map_id==0 else 1.18 if map_id==1 else 0.72
    sun.light_color=Color("fff0d4") if map_id==0 else Color("c6ddff")
    sun.shadow_enabled=true
    sun.directional_shadow_max_distance=160
    sun.light_angular_distance=0.6
    add_child(sun)

func _sea()->void:
    var m=PlaneMesh.new();m.size=Vector2(800,800);m.subdivide_width=800;m.subdivide_depth=800
    var shader=ShaderMaterial.new();shader.shader=preload("res://lagoon.gdshader") if map_id==0 else preload("res://water.gdshader")
    var deep=[Color("003044"),Color("06243a"),Color("020919")][map_id]
    var light=[Color("169b86"),Color("3b87a0"),Color("083a55")][map_id]
    shader.set_shader_parameter("deep_color",deep);shader.set_shader_parameter("light_color",light)
    shader.set_shader_parameter("wave_scale",1.0 if map_id!=1 else 1.5)
    water=shape(m,Vector3(0,-0.05,0),shader)
    water.layers=2
    water.cast_shadow=GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
    var far_material=ShaderMaterial.new();far_material.shader=preload("res://water.gdshader")
    far_material.set_shader_parameter("deep_color",deep);far_material.set_shader_parameter("light_color",light)
    distant_water_material=far_material
    for section in [[Vector2(2400,800),Vector3(0,-.05,-800)],[Vector2(2400,800),Vector3(0,-.05,800)],[Vector2(800,800),Vector3(-800,-.05,0)],[Vector2(800,800),Vector3(800,-.05,0)]]:
        var distant=PlaneMesh.new();distant.size=section[0];distant.subdivide_width=100;distant.subdivide_depth=80
        var far_water=shape(distant,section[1],far_material);far_water.layers=2;far_water.cast_shadow=GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
    water.cast_shadow=GeometryInstance3D.SHADOW_CASTING_SETTING_OFF

func _buoys()->void:
    var white=Color("edf9fa");var dark=Color("132b39")
    for i in range(0,samples.size(),7):
        var p=samples[i];var dir=tangent(i);var side=Vector3(-dir.z,0,dir.x)
        for s in [-1,1]:
            var bp=p+side*track_width*0.54*s
            cylinder(bp+Vector3(0,0.3,0),0.8,0.5,dark)
            cylinder(bp+Vector3(0,1.0,0),0.32,1.2,white, null,0.20)
            cylinder(bp+Vector3(0,1.5,0),0.34,0.28,Color("ff9f45") if s<0 else Color("35d4ba"))
    for i in range(gates.size()):
        var g=Node3D.new();g.position=gates[i]
        var idx=int(float(i)/gates.size()*samples.size())
        var dir=tangent(idx)
        g.rotation.y=atan2(-dir.x,-dir.z);add_child(g)
        gate_nodes.append(g)
        var c=Color("f5b75e") if i==0 else Color("9aeeee")
        for s in [-1,1]:
            box(Vector3(track_width*0.51*s,4.7,0),Vector3(0.8,9.4,0.8),Color("193c46"),g)
            box(Vector3(track_width*0.51*s,5,0),Vector3(0.85,2,0.85),c,g,0.7 if map_id==2 else 0)
        if i==0:
            box(Vector3(0,9.2,0),Vector3(track_width+2,1.8,1),dark,g)
            var label=Label3D.new();label.text="BENCHY RACERS";label.position=Vector3(0,9.25,0.56)
            label.font_size=96;label.pixel_size=0.014;label.modulate=white;label.outline_size=0
            g.add_child(label)
            for k in range(16):
                box(Vector3((k-7.5)*1.45,8,0),Vector3(1.4,0.28,1.2),white if k%2==0 else dark,g)
        else:
            var label=Label3D.new();label.text="%02d"%i;label.position=Vector3(track_width*0.51+1.2,5.2,0)
            label.font_size=80;label.pixel_size=0.035;label.billboard=BaseMaterial3D.BILLBOARD_ENABLED
            label.modulate=c;g.add_child(label)

func highlight_gate(index:int)->void:
    for i in range(gate_nodes.size()):
        var g=gate_nodes[i]
        # A floating navigation marker makes the next gate readable from the chase camera.
        var marker=g.get_node_or_null("NextMarker")
        if i==index and marker==null:
            var label=Label3D.new();label.name="NextMarker";label.text="▼";label.font_size=160
            label.pixel_size=0.03;label.position=Vector3(0,10.8,0);label.billboard=BaseMaterial3D.BILLBOARD_ENABLED
            label.modulate=Color("9bffd3");g.add_child(label)
        elif i!=index and marker!=null:marker.queue_free()

func _island(center:Vector3,radius:float,height:float,col:Color)->void:
    var st=SurfaceTool.new();st.begin(Mesh.PRIMITIVE_TRIANGLES)
    var count=18;var outer:Array[Vector3]=[];var inner:Array[Vector3]=[]
    for i in range(count):
        var a=TAU*i/count
        var r=radius*rng.randf_range(0.82,1.15)
        outer.append(Vector3(cos(a)*r,-0.65,sin(a)*r))
        inner.append(Vector3(cos(a)*r*0.66,height*0.6,sin(a)*r*0.66))
    for i in range(count):
        var j=(i+1)%count
        for v in [outer[i],inner[i],outer[j],outer[j],inner[i],inner[j]]:
            st.set_color(col);st.add_vertex(v)
        var top_col=Color("408b61") if map_id==0 else col.lightened(0.1)
        for v in [inner[i],Vector3(0,height,0),inner[j]]:
            st.set_color(top_col);st.add_vertex(v)
    st.generate_normals()
    var m=StandardMaterial3D.new();m.vertex_color_use_as_albedo=true;m.roughness=0.95
    shape(st.commit(),center,m)

func _palm(pos:Vector3)->void:
    var parent=Node3D.new();parent.position=pos;parent.rotation.y=rng.randf()*TAU;add_child(parent)
    var h=rng.randf_range(8,13)
    var trunk=cylinder(Vector3(0,h*0.5,0),0.48,h,Color("916d41"),parent,0.24)
    trunk.rotation.z=0.12
    for i in range(7):
        var leaf=Node3D.new();leaf.position=Vector3(-h*0.12,h,0);leaf.rotation.y=i*TAU/7;parent.add_child(leaf)
        var st=SurfaceTool.new();st.begin(Mesh.PRIMITIVE_TRIANGLES)
        for k in range(5):
            var t0=float(k)/5;var t1=float(k+1)/5
            var w0=sin(t0*PI)*1.0+0.05;var w1=sin(t1*PI)*1.0+0.05
            var y0=sin(t0*PI)*1.4-t0*1.8;var y1=sin(t1*PI)*1.4-t1*1.8
            var a=Vector3(-w0,y0,t0*7);var b=Vector3(w0,y0,t0*7)
            var c=Vector3(-w1,y1,t1*7);var d=Vector3(w1,y1,t1*7)
            for v in [a,c,b,b,c,d]:st.add_vertex(v)
        st.generate_normals()
        var lm=StandardMaterial3D.new();lm.albedo_color=Color("19894c") if i%2 else Color("3fa251");lm.cull_mode=BaseMaterial3D.CULL_DISABLED
        shape(st.commit(),Vector3.ZERO,lm,leaf)

func _rock(pos:Vector3,size:float,col:Color)->void:
    var m=SphereMesh.new();m.radius=size;m.height=size*1.4;m.radial_segments=7;m.rings=3
    var r=shape(m,pos,mat(col));r.rotation=Vector3(rng.randf(),rng.randf()*3,rng.randf()*0.5)
    r.scale=Vector3(1,rng.randf_range(0.8,1.9),rng.randf_range(0.7,1.2))

func _scenery()->void:
    if map_id==0:
        nature_kit=NatureKit.new();nature_kit.setup(self);nature_kit.build_tropical()
    elif map_id==1:
        for i in range(68):
            var p=Vector3(rng.randf_range(-470,470),0,rng.randf_range(-470,470))
            var size=rng.randf_range(8,39)
            if nearest(p).distance<size+22:continue
            _island(p,size,rng.randf_range(10,45),Color("c4e5ec") if i%2 else Color("eff7fb"))
            if i%3==0:_rock(p+Vector3(0,7,0),size*0.65,Color("7596b0"))
        for i in range(12):
            var a=i*TAU/12;var p=Vector3(cos(a)*490,0,sin(a)*490)
            _island(p,110,100+rng.randf()*80,Color("d1e3ed"))
            _island(p+Vector3(0,75,0),45,70,Color("f2f8fa"))
        var m=SphereMesh.new();m.radius=20;m.height=40;m.radial_segments=16;m.rings=8
        shape(m,Vector3(420,180,-500),mat(Color("e7f3ff"),1,0.3))
    else:
        for i in range(42):
            var p=Vector3(rng.randf_range(-410,410),0,rng.randf_range(-410,410))
            if nearest(p).distance<48:continue
            var h=rng.randf_range(12,80);var sx=rng.randf_range(15,32);var sz=rng.randf_range(15,32)
            box(p+Vector3(0,0.5,0),Vector3(sx+12,2,sz+12),Color("263742"))
            box(p+Vector3(0,h*0.5,0),Vector3(sx,h,sz),Color("1c2942") if i%2 else Color("27334b"))
            var c=Color("36e4d1") if i%2 else Color("f88abc")
            for y in range(4,int(h),5):
                box(p+Vector3(0,y,sz*0.5+0.1),Vector3(sx*0.84,0.8,0.15),c,null,1.5)
                box(p+Vector3(sx*0.5+0.1,y,0),Vector3(0.15,0.8,sz*0.84),c,null,1.5)
        for i in range(0,samples.size(),30):
            var t=tangent(i);var side=Vector3(-t.z,0,t.x)
            for s in [-1,1]:
                var p=samples[i]+side*(track_width+5)*s
                cylinder(p+Vector3(0,6,0),0.25,12,Color("41566b"))
                var m=SphereMesh.new();m.radius=0.9;m.height=1.8
                shape(m,p+Vector3(0,12,0),mat(Color("ffd69d"),0.5,3))
        # Container stacks on the inner harbor quay.
        for i in range(7):
            var p=Vector3(-65+i*13,0,-10)
            if nearest(p).distance<25:continue
            box(p+Vector3(0,0.5,0),Vector3(14,1,30),Color("39495c"))
            for k in range(2):
                var c=[Color("327f89"),Color("bc5d63"),Color("c99252")][i%3]
                box(p+Vector3(0,3+k*5,0),Vector3(10,4.8,22),c)
                for z in range(-9,10,3):box(p+Vector3(5.05,3+k*5,z),Vector3(0.1,4.3,0.18),c.lightened(0.15))

func _process(dt:float)->void:
    if not external_clock:clock+=dt
    if is_instance_valid(water):water.material_override.set_shader_parameter("sea_time",clock)
    if distant_water_material:distant_water_material.set_shader_parameter("sea_time",clock)
    for i in range(gate_nodes.size()):
        var marker=gate_nodes[i].get_node_or_null("NextMarker")
        if marker:marker.position.y=10.8+sin(clock*3)*0.5
