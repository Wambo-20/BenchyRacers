shader_type spatial;
uniform sampler2D sand_albedo : source_color,filter_linear_mipmap_anisotropic,repeat_enable;
uniform sampler2D grass_albedo : source_color,filter_linear_mipmap_anisotropic,repeat_enable;
uniform sampler2D rock_albedo : source_color,filter_linear_mipmap_anisotropic,repeat_enable;
uniform sampler2D sand_normal : hint_normal,filter_linear_mipmap_anisotropic,repeat_enable;
uniform sampler2D grass_normal : hint_normal,filter_linear_mipmap_anisotropic,repeat_enable;
uniform float snow=0.0;
varying vec3 world_pos;
varying vec3 world_normal;
void vertex() {
    world_pos=(MODEL_MATRIX*vec4(VERTEX,1.0)).xyz;
    world_normal=normalize(MODEL_NORMAL_MATRIX*NORMAL);
}
void fragment() {
    vec2 sand_uv=world_pos.xz*0.22;
    vec2 grass_uv=world_pos.xz*0.16;
    vec3 sand=texture(sand_albedo,sand_uv).rgb;
    vec3 grass=texture(grass_albedo,grass_uv).rgb*vec3(0.57,0.90,0.46);
    vec3 weights=pow(abs(world_normal),vec3(4.0));weights/=max(0.001,weights.x+weights.y+weights.z);
    vec3 rock=texture(rock_albedo,world_pos.zy*0.09).rgb*weights.x+texture(rock_albedo,world_pos.xz*0.09).rgb*weights.y+texture(rock_albedo,world_pos.xy*0.09).rgb*weights.z;
    float greenery=smoothstep(2.0,4.1,world_pos.y)*smoothstep(0.35,0.80,world_normal.y);
    float cliff=(1.0-smoothstep(0.35,0.82,world_normal.y))*smoothstep(2.0,6.0,world_pos.y);
    float wet=1.0-smoothstep(0.0,1.15,world_pos.y);
    ALBEDO=mix(mix(sand,grass,greenery),rock*0.80,cliff)*(1.0-wet*0.32);
    ALBEDO=mix(ALBEDO,vec3(0.68,0.80,0.87)*texture(sand_albedo,sand_uv).rgb*1.8,snow*smoothstep(0.45,0.95,world_normal.y));
    NORMAL_MAP=mix(texture(sand_normal,sand_uv).rgb,texture(grass_normal,grass_uv).rgb,greenery);
    NORMAL_MAP_DEPTH=0.8;
    ROUGHNESS=0.94-wet*0.52;
    SPECULAR=0.25;
}
