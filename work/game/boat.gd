extends Node3D
class_name RacingBoat

var peer_id:int=0
var driver_name:String="Skipper"
var color_index:int=0
var is_bot:bool=false
var heading:float=0.0
var speed:float=0.0
var velocity:Vector3=Vector3.ZERO
var boost:float=100.0
var lap:int=0
var checkpoint:int=1
var finished:bool=false
var finish_time:float=0.0
var lap_start:float=0.0
var best_lap:float=99999.0
var route_index:int=0
var off_track:bool=false
var controls:Vector3=Vector3.ZERO
var visual:Node3D
var material:ShaderMaterial
var name_tag:Label3D
var target_position:Vector3
var target_heading:float=0.0
var has_target:bool=false
var wake:MeshInstance3D
var trail:Array[MeshInstance3D]=[]
var wake_timer:float=0.0
var wake_points:Array=[]
var wake_mesh:ArrayMesh
var wake_sample_time:float=0.0
var bob_time:float=0.0
var vertical_velocity:float=0.0
var hull_pitch:float=0.0
var hull_roll:float=0.0
var pitch_velocity:float=0.0
var roll_velocity:float=0.0
var yaw_velocity:float=0.0
const DRAFT=0.34
var coins:int=0
var held_item:int=0
var stun:float=0.0
var shield:float=0.0
var item_boost:float=0.0
var shield_visual:MeshInstance3D
var spray:CPUParticles3D
static var foam_texture:ImageTexture

func setup(id:int,nick:String,idx:int,col:Color,bot:bool=false)->void:
    peer_id=id;driver_name=nick;color_index=idx;is_bot=bot
    visual=preload("res://assets/benchy.glb").instantiate()
    add_child(visual)
    material=ShaderMaterial.new()
    material.shader=preload("res://filament.gdshader")
    material.set_shader_parameter("filament_color",col)
    material.set_shader_parameter("galaxy",1.0 if idx==8 else 0.0)
    _paint(visual)
    name_tag=Label3D.new()
    name_tag.text=nick
    name_tag.position=Vector3(0,3.3,0)
    name_tag.font_size=32
    name_tag.pixel_size=0.0035
    name_tag.billboard=BaseMaterial3D.BILLBOARD_ENABLED
    name_tag.modulate=Color(0.91,0.98,1)
    name_tag.outline_size=10
    name_tag.no_depth_test=false
    add_child(name_tag)
    var bubble=SphereMesh.new();bubble.radius=2.25;bubble.height=4.5
    shield_visual=MeshInstance3D.new();shield_visual.mesh=bubble;shield_visual.position.y=1.2
    var bubble_mat=StandardMaterial3D.new();bubble_mat.albedo_color=Color(0.15,0.85,1,0.16)
    bubble_mat.transparency=BaseMaterial3D.TRANSPARENCY_ALPHA
    bubble_mat.shading_mode=BaseMaterial3D.SHADING_MODE_UNSHADED
    shield_visual.material_override=bubble_mat;shield_visual.visible=false;add_child(shield_visual)
    spray=CPUParticles3D.new();spray.amount=120;spray.lifetime=0.85;spray.local_coords=false
    spray.position=Vector3(0,0.3,1.5);spray.direction=Vector3(0,0.8,1)
    spray.spread=48;spray.initial_velocity_min=3;spray.initial_velocity_max=9
    spray.gravity=Vector3(0,-9,0);spray.scale_amount_min=0.07;spray.scale_amount_max=0.19
    var drop=SphereMesh.new();drop.radius=0.15;drop.height=0.3;drop.radial_segments=4;drop.rings=2
    var droplet=StandardMaterial3D.new();droplet.albedo_color=Color("c4f4ff")
    drop.material=droplet;spray.mesh=drop;spray.emitting=false;add_child(spray)
    if foam_texture==null:
        var img=Image.create(64,64,false,Image.FORMAT_RGBA8)
        var foam_rng=RandomNumberGenerator.new();foam_rng.seed=779
        for y in range(64):
            for x in range(64):
                var uv=Vector2((x-31.5)/31.5,(y-31.5)/31.5)
                var falloff=maxf(0.0,1.0-uv.length_squared())
                var alpha=pow(falloff,1.5)*foam_rng.randf_range(0.20,1.0)
                img.set_pixel(x,y,Color(0.8,0.97,1,alpha))
        foam_texture=ImageTexture.create_from_image(img)
    for i in range(22):
        var w=MeshInstance3D.new()
        var m=PlaneMesh.new();m.size=Vector2(1.2,0.5)
        w.mesh=m
        var mat=StandardMaterial3D.new()
        mat.albedo_color=Color(0.93,0.98,1,0.48)
        mat.albedo_texture=foam_texture
        mat.transparency=BaseMaterial3D.TRANSPARENCY_ALPHA
        mat.shading_mode=BaseMaterial3D.SHADING_MODE_UNSHADED
        mat.cull_mode=BaseMaterial3D.CULL_DISABLED
        w.material_override=mat
        w.visible=false
        trail.append(w)
    wake_mesh=ArrayMesh.new();wake=MeshInstance3D.new();wake.mesh=wake_mesh
    var wake_material=ShaderMaterial.new();wake_material.shader=preload("res://wake.gdshader");wake_material.render_priority=2
    wake.material_override=wake_material;wake.cast_shadow=GeometryInstance3D.SHADOW_CASTING_SETTING_OFF;add_child(wake)
    wake.top_level=true;wake.global_transform=Transform3D.IDENTITY

func _paint(n:Node)->void:
    if n is MeshInstance3D: n.material_override=material
    for c in n.get_children():_paint(c)

func put_on_grid(pos:Vector3,angle:float)->void:
    position=pos;heading=angle;rotation.y=heading
    speed=0;velocity=Vector3.ZERO;boost=100
    lap=0;checkpoint=1;finished=false;finish_time=0;best_lap=99999;lap_start=0
    route_index=0;has_target=false
    coins=0;held_item=0;stun=0;shield=0;item_boost=0
    vertical_velocity=0;hull_pitch=0;hull_roll=0;pitch_velocity=0;roll_velocity=0;yaw_velocity=0

func simulate(dt:float,world:Node,elapsed:float)->void:
    if finished:controls=Vector3(0,0,0)
    var gas=controls.x
    var steering=controls.y
    stun=maxf(0,stun-dt);shield=maxf(0,shield-dt);item_boost=maxf(0,item_boost-dt)
    if stun>0:gas=0;steering=sin(stun*15);speed=move_toward(speed,0,35*dt)
    var use_boost=(controls.z>0.5 and boost>1 or item_boost>0) and gas>0 and not finished and stun<=0
    var vmax=(46.0 if use_boost else 31.0)*(1.0+minf(coins,10)*0.006)
    var accel=25.0 if use_boost else 13.5
    if gas>0:speed=move_toward(speed,vmax,accel*dt)
    elif gas<0:speed=move_toward(speed,-8.0,22.0*dt)
    else:speed=move_toward(speed,0.0,5.0*dt)
    if use_boost and item_boost<=0:boost=maxf(0.0,boost-dt*32)
    else:boost=minf(100.0,boost+dt*11)
    var turn_factor=clampf(absf(speed)/12,0.16,1.0)
    var rudder_rate=-steering*1.35*turn_factor*(0.85 if use_boost else 1.0)
    yaw_velocity=lerpf(yaw_velocity,rudder_rate,1-exp(-dt*7.0))
    heading+=yaw_velocity*dt
    var forward=Vector3(-sin(heading),0,-cos(heading))
    var side=Vector3(cos(heading),0,-sin(heading))
    var motor_force=forward*(speed-velocity.dot(forward))*4.2
    var lateral_drag=-side*velocity.dot(side)*3.2
    velocity+=(motor_force+lateral_drag)*dt
    position+=velocity*dt
    simulate_buoyancy(dt,world.clock,world.wave_scale)
    var nearest:Dictionary=world.nearest(position,route_index)
    route_index=nearest.index
    off_track=nearest.distance>world.track_width*0.65
    if off_track and speed>13.0:speed=move_toward(speed,13.0,18.0*dt)
    if position.length()>1100:
        position=world.samples[route_index]+Vector3(0,0,0)
        speed=0;velocity=Vector3.ZERO
    rotation.y=heading
    if not finished and position.distance_to(world.gates[checkpoint])<world.track_width*0.7:
        checkpoint+=1
        if checkpoint>=world.gates.size():checkpoint=0
        elif checkpoint==1:
            lap+=1
            best_lap=minf(best_lap,elapsed-lap_start)
            lap_start=elapsed
            if lap>=3:finished=true;finish_time=elapsed

func animate(dt:float,clock:float,wave_strength:float=1.0)->void:
    if has_target:
        position=position.lerp(target_position,1-exp(-dt*12))
        heading=lerp_angle(heading,target_heading,1-exp(-dt*12))
        rotation.y=heading
    visual.position.y=-DRAFT
    visual.rotation.z=hull_roll
    visual.rotation.x=hull_pitch
    if stun>0:visual.rotation.y=clock*12
    else:visual.rotation.y=lerp_angle(visual.rotation.y,0,dt*8)
    shield_visual.visible=shield>0
    shield_visual.rotation.y=clock
    spray.emitting=absf(speed)>16
    _update_wake(dt,clock)
    wake.material_override.set_shader_parameter("sea_time",clock)
    wake.material_override.set_shader_parameter("wave_scale",wave_strength)
    wake.material_override.set_shader_parameter("strength",clampf(absf(speed)/35,0,1))
    wake_timer+=dt
    if wake_timer>0.065 and absf(speed)>3:
        wake_timer=0
        var w:MeshInstance3D=trail.pop_front()
        trail.append(w)
        if not is_instance_valid(w.get_parent()):get_parent().add_child(w)
        w.global_position=global_position+Vector3(sin(heading),0,cos(heading))*1.6
        w.global_position.y=_wave_height(w.global_position,clock)+0.09
        w.rotation.y=heading
        w.scale=Vector3.ONE
        w.visible=true
    for w in trail:
        if w.visible:
            w.scale.x+=dt*1.6
            w.scale.z+=dt*0.7
            w.global_position.y=_wave_height(w.global_position,clock)+0.09
            var c:Color=w.material_override.albedo_color
            c.a=maxf(0,c.a-dt*0.32)
            w.material_override.albedo_color=c
            if c.a<=0:w.visible=false
        else:
            w.material_override.albedo_color=Color(0.93,0.98,1,0.48)

func _wave_height(p:Vector3,t:float)->float:
    return sin(p.x*0.095+p.z*0.042+t*0.85)*0.32+sin(-p.x*0.051+p.z*0.13-t*1.12)*0.20+sin(p.x*0.21-p.z*0.17+t*1.7)*0.075

func _update_wake(dt:float,clock:float)->void:
    # Recorded vertices are world coordinates; never inherit the boat spawn transform.
    wake.global_transform=Transform3D.IDENTITY
    for point in wake_points:point.age+=dt
    while not wake_points.is_empty() and wake_points[-1].age>2.8:wake_points.pop_back()
    wake_sample_time+=dt
    if absf(speed)>5 and wake_sample_time>0.055:
        wake_sample_time=0
        var p=global_position+Vector3(sin(heading),0,cos(heading))*1.6
        if not wake_points.is_empty() and p.distance_to(wake_points[0].pos)>8:wake_points.clear()
        wake_points.push_front({"pos":p,"side":Vector3(cos(heading),0,-sin(heading)),"age":0.0,"power":clampf(absf(speed)/35,0,1)})
    wake.visible=wake_points.size()>2
    if not wake.visible:return
    var vertices=PackedVector3Array();var uv=PackedVector2Array();var colors=PackedColorArray();var normals=PackedVector3Array()
    for i in range(wake_points.size()-1):
        var a=wake_points[i];var b=wake_points[i+1]
        for side in [-1.0,0.0,1.0]:
            var corners:Array=[]
            for pair in [[a,-1.0],[a,1.0],[b,-1.0],[b,1.0]]:
                var point=pair[0];var edge=float(pair[1]);var age=float(point.age)
                var spread=0.28+age*0.85
                var width=(0.62+age*0.22) if side==0.0 else (0.25+age*0.18)
                var p:Vector3=point.pos+point.side*(side*spread+edge*width)
                p+=Vector3(sin(clock*.7+p.z*.12),0,cos(clock*.6+p.x*.13))*age*.13
                corners.append([p,Vector2((edge+1)*.5,age/2.8),Color(1,1,1,pow(maxf(0,1-age/2.8),1.5)*point.power*(0.85 if side==0.0 else 0.55))])
            for index in [0,2,1,1,2,3]:
                vertices.append(corners[index][0]);uv.append(corners[index][1]);colors.append(corners[index][2]);normals.append(Vector3.UP)
    var arrays:Array=[];arrays.resize(Mesh.ARRAY_MAX);arrays[Mesh.ARRAY_VERTEX]=vertices;arrays[Mesh.ARRAY_TEX_UV]=uv;arrays[Mesh.ARRAY_COLOR]=colors;arrays[Mesh.ARRAY_NORMAL]=normals
    wake_mesh.clear_surfaces();wake_mesh.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES,arrays)

func simulate_buoyancy(dt:float,t:float,wave_scale:float=1.0)->void:
    # Four displaced hull sections: buoyancy balances weight at 34 cm draft.
    # Forces and torques are integrated, rather than snapping the hull onto a plane.
    var forward=Vector3(-sin(heading),0,-cos(heading))
    var side=Vector3(cos(heading),0,-sin(heading))
    var heave_accel=-9.81-vertical_velocity*3.4
    var pitch_accel=-pitch_velocity*5.8+absf(speed)*0.012
    var roll_accel=-roll_velocity*6.4+controls.y*absf(speed)*0.045
    for longitudinal in [-1.30,1.30]:
        for lateral in [-0.62,0.62]:
            var point=global_position+forward*longitudinal+side*lateral
            var hull_y=global_position.y-DRAFT+longitudinal*sin(hull_pitch)+lateral*sin(hull_roll)
            var water_y=_wave_height(point,t)*wave_scale
            var submerged=clampf(water_y-hull_y,0.0,DRAFT*3.0)
            var force_accel=9.81*submerged/DRAFT*0.25
            heave_accel+=force_accel
            pitch_accel+=longitudinal*force_accel/1.8
            roll_accel+=lateral*force_accel/0.70
    vertical_velocity+=heave_accel*dt
    position.y+=vertical_velocity*dt
    pitch_velocity+=pitch_accel*dt;roll_velocity+=roll_accel*dt
    hull_pitch=clampf(hull_pitch+pitch_velocity*dt,-0.28,0.28)
    hull_roll=clampf(hull_roll+roll_velocity*dt,-0.42,0.42)

func _exit_tree()->void:
    for w in trail:
        if is_instance_valid(w) and w.get_parent()==null:w.free()

func ai_input(world:Node,clock:float)->Vector3:
    if finished:return Vector3.ZERO
    var lookahead=7+int(absf(speed)*0.12)
    var ahead:int=(route_index+lookahead)%world.samples.size()
    var goal:Vector3=world.samples[ahead]
    var gate_index=int(float(checkpoint)/world.gates.size()*world.samples.size())
    var to_gate=posmod(gate_index-route_index,world.samples.size())
    if to_gate<lookahead+4 or to_gate>world.samples.size()/2:
        goal=world.gates[checkpoint]
    var side=Vector3(cos(heading),0,-sin(heading))
    goal+=side*sin(peer_id*1.3)*world.track_width*0.08
    var delta=goal-position
    var desired=atan2(-delta.x,-delta.z)
    var angle=wrapf(heading-desired,-PI,PI)
    var steering=clampf(angle*2.6,-1,1)
    var boosting=absf(angle)<0.12 and boost>35 and fmod(clock+peer_id,9)<2.6
    var gas=-0.5 if absf(angle)>0.65 and speed>19 else 1.0
    return Vector3(gas,steering,1 if boosting else 0)

func reset_to_course(world:Node)->void:
    var idx:int=maxi(0,int(float(checkpoint-1 if checkpoint>0 else world.gates.size()-1)/world.gates.size()*world.samples.size()))
    var a:Vector3=world.samples[idx%world.samples.size()]
    var b:Vector3=world.samples[(idx+1)%world.samples.size()]
    position=a
    heading=atan2(-(b-a).x,-(b-a).z)
    speed=0;velocity=Vector3.ZERO;has_target=false
