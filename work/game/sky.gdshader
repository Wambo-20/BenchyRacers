shader_type sky;
uniform vec4 top_color : source_color = vec4(0.08,0.36,0.68,1.0);
uniform vec4 horizon_color : source_color = vec4(0.68,0.84,0.90,1.0);
uniform float night = 0.0;
float hash(vec2 p) { return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453); }
float noise(vec2 p) {
    vec2 i=floor(p),f=fract(p);f=f*f*(3.0-2.0*f);
    return mix(mix(hash(i),hash(i+vec2(1,0)),f.x),mix(hash(i+vec2(0,1)),hash(i+vec2(1,1)),f.x),f.y);
}
float clouds(vec2 p) {
    return noise(p)*0.5+noise(p*2.03)*0.25+noise(p*4.01)*0.125+noise(p*8.07)*0.0625;
}
void sky() {
    float h=clamp(EYEDIR.y,0.0,1.0);
    vec3 base=mix(horizon_color.rgb,top_color.rgb,pow(h,0.45));
    vec2 uv=EYEDIR.xz/(max(0.08,EYEDIR.y)+0.15)*1.9+vec2(TIME*0.001,0.0);
    float cl=smoothstep(0.47,0.67,clouds(uv))*smoothstep(0.02,0.25,h);
    vec3 cloud_col=mix(vec3(0.91,0.94,0.98),vec3(0.08,0.10,0.19),night);
    base=mix(base,cloud_col,cl*(0.73-night*0.45));
    float sun=dot(normalize(vec3(-0.35,0.707,0.612)),EYEDIR);
    base+=vec3(1.0,0.81,0.55)*(pow(max(sun,0.0),800.0)*2.0+pow(max(sun,0.0),24.0)*0.12)*(1.0-night);
    float star=step(0.9965,hash(floor(EYEDIR.xz/(h+0.15)*420.0)));
    base+=star*night*smoothstep(0.15,0.6,h)*0.65;
    COLOR=base;
}
