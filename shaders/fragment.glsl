// ==========================================================================
// Vertex program for barebones GLFW boilerplate
//
// Author:  Sonny Chan, University of Calgary
// Date:    December 2015
// ==========================================================================
#version 410

// interpolated colour received from vertex stage
uniform sampler2D image;
uniform sampler2D nightmap;
uniform int shade_flg;
uniform int night_flg;
uniform vec3 camPosition;
uniform sampler2D pecularmap;

in vec2 Texcoord;   
in vec3 Vertexp;    // vertex position
in vec3 center;     // planet center

// first output is mapped to the framebuffer's colour index by default
out vec4 FragmentColour;

float max(float a, float b){
    if(a>b)return a;
    else return b;
}
float dot_normal(vec3 a, vec3 b){
    return a.x*b.x + a.y*b.y + a.z*b.z;
}
void main(void)
{
    if(shade_flg == 1){
        vec3 n = normalize(Vertexp - center);
        vec3 l = normalize(vec3(0,0,0) - Vertexp);
        float diffuse;
        diffuse = max(dot_normal(n, l), 0);
        float ratio = min(1, 0.2 + diffuse);
        
        if(night_flg == 1){
            vec4 spec = texture(pecularmap, Texcoord);
            if(spec.x == 0 && spec.y == 0 && spec.z == 0){
                FragmentColour = texture(image, Texcoord) * ratio + texture(nightmap, Texcoord) * (1 - ratio);
            }
            else{   // ocean
                vec3 viewDir = normalize(camPosition - Vertexp);   // View ray
                vec3 reflect_light = -l + 2 * n * (dot_normal(n,l));

                float spec_ratio = 0.7 * max(0,dot_normal(reflect_light, viewDir));
                FragmentColour = texture(image, Texcoord) * ratio + diffuse * pow(spec_ratio ,2) + texture(nightmap, Texcoord) * (1 - ratio);
            }
        }
        else FragmentColour = texture(image, Texcoord) * ratio;
    }else{
        FragmentColour = texture(image, Texcoord);
    }
}
