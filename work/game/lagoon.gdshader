shader_type spatial;
render_mode unshaded,cull_disabled,depth_draw_never;
uniform sampler2D scene_color : hint_screen_texture,filter_linear_mipmap,repeat_disable;
uniform sampler2D scene_depth : hint_depth_texture,filter_nearest,repeat_disable;
uniform vec4 deep_color : source_color=vec4(0.008,0.10,0.16,1.0);
uniform vec4 sky_top : source_color=vec4(0.196,0.54,0.784,1.0);
uniform vec4 sky_horizon : source_color=vec4(0.75,0.906,0.93,1.0);
uniform float sea_time=0.0;
uniform float wave_scale=1.0;
uniform bool reflections=true;
uniform int reflection_steps=24;
varying vec3 world_pos;
float hash(vec2 p){return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453);}
float noise(vec2 p){vec2 i=floor(p),f=fract(p);f=f*f*(3.0-2.0*f);return mix(mix(hash(i),hash(i+vec2(1,0)),f.x),mix(hash(i+vec2(0,1)),hash(i+vec2(1,1)),f.x),f.y);}
float micro(vec2 p,float t){
    return sin(dot(p,vec2(.83,.42))+t*1.05)*.050
        +sin(dot(p,vec2(1.41,-.73))-t*1.7)*.025
        +sin(dot(p,vec2(-.62,1.74))+t*1.9)*.020
        +sin(dot(p,vec2(2.47,.93))-t*2.4)*.012
        +sin(dot(p,vec2(.42,3.16))+t*2.8)*.008
        +sin(dot(p,vec2(-3.54,-1.17))-t*3.1)*.006
        +sin(dot(p,vec2(5.21,2.34))+t*4.0)*.003
        +sin(dot(p,vec2(1.95,-4.36))-t*3.6)*.004
        +sin(dot(p,vec2(-1.26,-2.79))+t*2.9)*.011
        +sin(dot(p,vec2(3.57,-2.02))+t*3.5)*.005
        +sin(dot(p,vec2(6.18,-3.27))+t*4.8)*.002
        +sin(dot(p,vec2(-4.44,3.85))-t*4.2)*.0025;
}
float wave(vec2 p,float t) {
    return sin(dot(p,vec2(0.095,0.042))+t*0.85)*0.32+sin(dot(p,vec2(-0.051,0.13))-t*1.12)*0.20+sin(dot(p,vec2(0.21,-0.17))+t*1.7)*0.075;
}
vec3 reconstruct(vec2 uv,float depth,mat4 inverse_projection) {
#if CURRENT_RENDERER == RENDERER_COMPATIBILITY
    vec4 p=inverse_projection*vec4(uv*2.0-1.0,depth*2.0-1.0,1.0);
#else
    vec4 p=inverse_projection*vec4(uv*2.0-1.0,depth,1.0);
#endif
    return p.xyz/p.w;
}
void vertex() {
    world_pos=(MODEL_MATRIX*vec4(VERTEX,1.0)).xyz;
    float height=wave(world_pos.xz,sea_time)*wave_scale;
    VERTEX.y+=height;world_pos.y+=height;
}
void fragment() {
    vec2 p=world_pos.xz;float t=sea_time;
    float dx=(wave(p+vec2(0.12,0),t)-wave(p-vec2(0.12,0),t))/0.24;
    float dz=(wave(p+vec2(0,0.12),t)-wave(p-vec2(0,0.12),t))/0.24;
    dx+=(micro(p+vec2(.08,0),t)-micro(p-vec2(.08,0),t))/.16;
    dz+=(micro(p+vec2(0,.08),t)-micro(p-vec2(0,.08),t))/.16;
    vec3 n=normalize(vec3(-dx*wave_scale,1.0,-dz*wave_scale));
    NORMAL=normalize((VIEW_MATRIX*vec4(n,0)).xyz);
    float raw=texture(scene_depth,SCREEN_UV).r;
    vec3 behind=reconstruct(SCREEN_UV,raw,INV_PROJECTION_MATRIX);
    float thickness=max(0.0,-behind.z+VERTEX.z);
    vec2 refract_uv=clamp(SCREEN_UV+NORMAL.xy*0.012*smoothstep(0.1,2.0,thickness),vec2(0.001),vec2(0.999));
    vec3 displaced=reconstruct(refract_uv,texture(scene_depth,refract_uv).r,INV_PROJECTION_MATRIX);
    if(displaced.z>VERTEX.z-0.04)refract_uv=SCREEN_UV;
    vec3 background=textureLod(scene_color,refract_uv,0.0).rgb;
    vec3 transmission=exp(-vec3(0.18,0.060,0.045)*min(thickness,80.0));
    vec3 refracted=background*transmission+deep_color.rgb*(vec3(1.0)-transmission);
    float fresnel=0.02+0.98*pow(1.0-clamp(dot(NORMAL,VIEW),0.0,1.0),5.0);
    float foam=(1.0-smoothstep(0.1,0.85,thickness))*smoothstep(0.02,0.15,thickness);
    foam*=smoothstep(-0.25,0.65,sin(p.x*3.1+p.y*2.7-t*2.2)+sin(p.x*8.2-p.y*7.0)*0.28);
    vec3 reflected=vec3(0.0);float hit=0.0;
    if(reflections && fresnel>0.03) {
        vec3 direction=reflect(normalize(VERTEX),NORMAL);
        vec3 ray=VERTEX+NORMAL*0.12;
        float stride=0.7;
        for(int i=0;i<40;i++) {
            if(i>=reflection_steps)break;
            ray+=direction*stride;stride*=1.13;
            vec4 projected=PROJECTION_MATRIX*vec4(ray,1.0);
            vec2 uv=projected.xy/projected.w*0.5+0.5;
            if(uv.x<0.005 || uv.x>0.995 || uv.y<0.005 || uv.y>0.995 || ray.z>0.0)break;
            vec3 surface=reconstruct(uv,texture(scene_depth,uv).r,INV_PROJECTION_MATRIX);
            float delta=surface.z-ray.z;
            vec3 hit_world=(INV_VIEW_MATRIX*vec4(surface,1.0)).xyz;
            if(delta>0.0 && delta<stride*1.5 && hit_world.y>0.25) {
                reflected=textureLod(scene_color,uv,1.0).rgb;
                hit=(1.0-smoothstep(0.35,0.50,abs(uv.x-0.5)))*(1.0-smoothstep(0.35,0.50,abs(uv.y-0.5)));
                break;
            }
        }
    }
    vec3 reflected_direction=(INV_VIEW_MATRIX*vec4(reflect(normalize(VERTEX),NORMAL),0)).xyz;
    float elevation=clamp(reflected_direction.y,0.0,1.0);
    vec3 sky=mix(sky_horizon.rgb,sky_top.rgb,pow(elevation,.45));
    vec2 cloud_uv=reflected_direction.xz/(max(.08,elevation)+.15)*1.9;
    float clouds=noise(cloud_uv)*.5+noise(cloud_uv*2.03)*.25+noise(cloud_uv*4.01)*.125;
    sky=mix(sky,vec3(.78,.86,.92),smoothstep(.47,.65,clouds)*smoothstep(.02,.25,elevation)*.55);
    float sun=pow(max(dot(normalize(reflected_direction),normalize(vec3(-.35,.707,.612))),0.0),800.0);
    sky+=vec3(12.0,10.0,7.2)*sun;
    vec3 reflection=mix(sky,reflected,hit);
    ALBEDO=mix(mix(refracted,reflection,fresnel),vec3(.78,.88,.85),foam*.7);
    ALPHA=1.0;
}
