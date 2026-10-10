shader_type spatial;
uniform sampler2D sand : source_color,filter_linear_mipmap_anisotropic,repeat_enable;
uniform sampler2D normal_map : hint_normal,filter_linear_mipmap_anisotropic,repeat_enable;
varying vec3 world_pos;
void vertex(){world_pos=(MODEL_MATRIX*vec4(VERTEX,1)).xyz;}
void fragment(){
    vec2 p=world_pos.xz;
    float ripple=sin(p.x*2.8+sin(p.y*0.32)*1.3)*0.04;
    ALBEDO=texture(sand,p*0.20).rgb*vec3(0.84,0.91,0.79)*(0.90+ripple);
    float caustic=pow(0.5+0.5*sin(p.x*1.8+p.y*0.7+TIME*1.4)*sin(p.y*1.4-p.x*0.9-TIME*1.1),8.0);
    EMISSION=vec3(0.13,0.24,0.20)*caustic;
    NORMAL_MAP=texture(normal_map,p*0.20).rgb;NORMAL_MAP_DEPTH=0.55;
    ROUGHNESS=0.9;
}
