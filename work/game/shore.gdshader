shader_type spatial;
render_mode cull_disabled,depth_draw_never;
float hash(vec2 p){return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453);}
float noise(vec2 p){vec2 i=floor(p),f=fract(p);f=f*f*(3.0-2.0*f);return mix(mix(hash(i),hash(i+vec2(1,0)),f.x),mix(hash(i+vec2(0,1)),hash(i+vec2(1,1)),f.x),f.y);}
void fragment() {
    float bands=sin(UV.x*22.0-TIME*1.6+sin(UV.y*16.0)*0.7)*0.5+0.5;
    float envelope=sin(UV.x*3.14159);
    float fine=noise(UV*vec2(130,170)-TIME*1.5);
    ALBEDO=vec3(0.86,0.93,0.90);ROUGHNESS=0.95;
    ALPHA=envelope*smoothstep(0.30,0.7,bands)*smoothstep(.20,.68,fine)*.60;
}
