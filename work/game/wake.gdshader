shader_type spatial;
render_mode cull_disabled,depth_draw_never;
uniform float sea_time=0.0;
uniform float strength=0.0;
uniform float wave_scale=1.0;
varying vec3 world_pos;
float hash(vec2 p){return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453);}
float noise(vec2 p){vec2 i=floor(p),f=fract(p);f=f*f*(3.0-2.0*f);return mix(mix(hash(i),hash(i+vec2(1,0)),f.x),mix(hash(i+vec2(0,1)),hash(i+vec2(1,1)),f.x),f.y);}
void vertex(){
    world_pos=(MODEL_MATRIX*vec4(VERTEX,1)).xyz;
    vec2 p=world_pos.xz;float t=sea_time;
    world_pos.y=(sin(dot(p,vec2(.095,.042))+t*.85)*.32+sin(dot(p,vec2(-.051,.13))-t*1.12)*.20+sin(dot(p,vec2(.21,-.17))+t*1.7)*.075)*wave_scale+0.12;
    VERTEX=(inverse(MODEL_MATRIX)*vec4(world_pos,1)).xyz;
}
void fragment(){
    float edge=pow(max(0.0,sin(clamp(UV.x,0.0,1.0)*3.14159)),.7);
    float bubbles=noise(world_pos.xz*5.0+vec2(sea_time*.45,-sea_time*.7));
    float fine=noise(world_pos.xz*14.0-sea_time*.9);
    float foam=smoothstep(.30,.68,bubbles*.65+fine*.35);
    ALBEDO=vec3(.82,.91,.88);ROUGHNESS=0.96;
    ALPHA=edge*foam*COLOR.a*.78;
}
