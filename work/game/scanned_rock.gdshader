shader_type spatial;
uniform sampler2D albedo_tex : source_color,filter_linear_mipmap_anisotropic;
uniform sampler2D normal_tex : hint_normal,filter_linear_mipmap_anisotropic;
varying float height;
void vertex(){height=(MODEL_MATRIX*vec4(VERTEX,1)).y;}
void fragment(){
    vec3 color=texture(albedo_tex,UV).rgb;
    float gray=dot(color,vec3(.2126,.7152,.0722));
    float wet=1.0-smoothstep(-.3,2.0,height);
    ALBEDO=mix(vec3(gray)*vec3(.90,.97,1.0),color,.20)*(1.0-wet*.24);
    NORMAL_MAP=texture(normal_tex,UV).rgb;NORMAL_MAP_DEPTH=.9;
    ROUGHNESS=.85-wet*.24;SPECULAR=.3;
}
