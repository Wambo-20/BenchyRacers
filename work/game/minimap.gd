extends Control

var race_world:RaceWorld
var boats:Dictionary={}
var local_id:int=1
var colours:Array=[]

func _draw()->void:
    if not is_instance_valid(race_world):return
    var center=size*0.5
    var scale_factor=minf(size.x,size.y)/540.0
    var pts=PackedVector2Array()
    for p in race_world.samples:pts.append(center+Vector2(p.x,p.z)*scale_factor)
    if pts.size()>0:pts.append(pts[0])
    draw_polyline(pts,Color(0.13,0.34,0.41,0.9),9,true)
    draw_polyline(pts,Color(0.46,0.76,0.79,0.85),2,true)
    var start=race_world.samples[0]
    draw_circle(center+Vector2(start.x,start.z)*scale_factor,4,Color("ffcf7b"))
    for b in boats.values():
        if not is_instance_valid(b):continue
        var p=center+Vector2(b.position.x,b.position.z)*scale_factor
        var c:Color=colours[b.color_index].color if colours.size()>b.color_index else Color.WHITE
        if b.peer_id==local_id:draw_circle(p,7,Color.WHITE)
        draw_circle(p,4.5,c)

func _process(_dt:float)->void:queue_redraw()
