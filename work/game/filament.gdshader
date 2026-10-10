shader_type spatial;
uniform vec4 filament_color : source_color = vec4(1.0,0.4,0.05,1.0);
uniform float matte = 0.78;
uniform float galaxy = 0.0;
varying vec3 local_pos;
float hash3(vec3 p) {return fract(sin(dot(p,vec3(127.1,311.7,74.7)))*43758.5453);}
void vertex() { local_pos=VERTEX; }
void fragment() {
    float line=smoothstep(0.6,0.95,fract(local_pos.y*90.0));
    vec3 cell=floor(local_pos*94.0);
    float seed=hash3(cell);
    float flake=step(0.96,seed)*galaxy;
    vec3 facet=normalize(NORMAL+vec3(hash3(cell+13.7)-0.5,hash3(cell+5.3)-0.5,hash3(cell+28.1)-0.5)*0.8);
    float glint=pow(max(dot(facet,normalize(VIEW+vec3(-0.25,0.55,0.85))),0.0),48.0);
    float micro=pow(max(dot(facet,VIEW),0.0),95.0);
    float sparkle=flake*(glint*2.3+micro*0.35);
    ALBEDO=mix(filament_color.rgb*(0.94+0.06*line),vec3(0.42,0.46,0.58),flake*0.35);
    NORMAL=mix(NORMAL,facet,flake*0.18);
    ROUGHNESS=mix(matte,0.19,flake);
    METALLIC=mix(0.02,0.82,flake);
    EMISSION=vec3(0.68,0.78,1.0)*sparkle;
    SPECULAR=0.4;
}
