shader_type spatial;
render_mode cull_disabled;
uniform vec4 leaf_color : source_color=vec4(0.14,0.38,0.07,1.0);
uniform float wind_strength=0.16;
varying vec3 leaf_pos;
void vertex() {
    vec3 origin=MODEL_MATRIX[3].xyz;
    float phase=origin.x*0.12+origin.z*0.15;
    float height=clamp(VERTEX.y/18.0,0.0,1.0);
    VERTEX.x+=sin(TIME*0.9+phase+VERTEX.y*0.18)*wind_strength*height*height;
    VERTEX.z+=cos(TIME*0.75+phase+VERTEX.x*0.3)*wind_strength*height;
    VERTEX.y+=sin(TIME*2.3+phase+UV.y*6.0)*0.025*UV.y;
    leaf_pos=VERTEX;
}
void fragment() {
    float vein=1.0-smoothstep(0.01,0.04,abs(UV.x-0.5));
    float fibers=sin(UV.y*90.0+UV.x*12.0)*0.025;
    ALBEDO=leaf_color.rgb*COLOR.rgb*(0.90+fibers)+vec3(0.10,0.13,0.035)*vein;
    ROUGHNESS=0.61;
    SPECULAR=0.33;
    BACKLIGHT=vec3(0.12,0.20,0.035);
}
