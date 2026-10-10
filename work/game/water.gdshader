shader_type spatial;
render_mode cull_disabled;
uniform vec4 deep_color : source_color = vec4(0.005,0.08,0.13,1.0);
uniform vec4 light_color : source_color = vec4(0.02,0.25,0.26,1.0);
uniform float wave_scale = 1.0;
uniform float sea_time=0.0;
varying vec3 world_pos;
float wave(vec2 p,float t) {
    return sin(dot(p,vec2(0.095,0.042))+t*0.85)*0.32
        +sin(dot(p,vec2(-0.051,0.13))-t*1.12)*0.20
        +sin(dot(p,vec2(0.21,-0.17))+t*1.7)*0.075;
}
void vertex() {
    world_pos=(MODEL_MATRIX * vec4(VERTEX,1.0)).xyz;
    float w=wave(world_pos.xz,sea_time);
    VERTEX.y+=w*wave_scale;
    world_pos.y+=w*wave_scale;
}
void fragment() {
    vec2 p=world_pos.xz;
    float t=sea_time;
    float dx=(wave(p+vec2(0.2,0.0),t)-wave(p-vec2(0.2,0.0),t))*2.5*wave_scale;
    float dz=(wave(p+vec2(0.0,0.2),t)-wave(p-vec2(0.0,0.2),t))*2.5*wave_scale;
    dx+=sin(p.x*1.15+p.y*0.76+t*2.3)*0.032+sin(p.x*2.8-p.y*1.1-t*3.2)*0.018;
    dz+=cos(p.x*0.92-p.y*1.3-t*1.9)*0.03+cos(p.y*2.3+p.x*1.7+t*2.5)*0.015;
    vec3 n=normalize(vec3(-dx,1.0,-dz));
    NORMAL=normalize((VIEW_MATRIX*vec4(n,0.0)).xyz);
    float broad=sin(p.x*0.015+p.y*0.024)*0.5+0.5;
    float fresnel=pow(1.0-clamp(dot(NORMAL,VIEW),0.0,1.0),4.0);
    vec3 base=mix(deep_color.rgb,light_color.rgb,0.3+broad*0.55);
    float crest=smoothstep(0.44,0.58,world_pos.y)*0.15;
    ALBEDO=mix(base,vec3(0.08,0.17,0.22),fresnel*0.28)+vec3(crest);
    ROUGHNESS=0.16+fresnel*0.06;
    METALLIC=0.12;
    SPECULAR=0.9;
}
