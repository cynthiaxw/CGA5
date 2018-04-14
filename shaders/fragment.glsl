// ==========================================================================
// Vertex program for barebones GLFW boilerplate
//
// Author:  Sonny Chan, University of Calgary
// Date:    December 2015
// ==========================================================================
#version 410

// interpolated colour received from vertex stage
uniform sampler2D image;
uniform int shade_flg;

in vec2 Texcoord;
in vec3 Vertexp;
in vec3 center;

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
        float ratio = min(1, 0.3 + diffuse);
        FragmentColour = texture(image, Texcoord) * ratio;
    }else{
        FragmentColour = texture(image, Texcoord);
    }
}
