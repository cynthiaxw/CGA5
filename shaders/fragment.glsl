// ==========================================================================
// Vertex program for barebones GLFW boilerplate
//
// Author:  Sonny Chan, University of Calgary
// Date:    December 2015
// ==========================================================================
#version 410

// interpolated colour received from vertex stage
uniform vec3 Colour;
uniform sampler2D image;

in vec2 Texcoord;

// first output is mapped to the framebuffer's colour index by default
out vec4 FragmentColour;

void main(void)
{
    // write colour output without modification
    FragmentColour = texture(image, Texcoord);
    //FragmentColour = vec4(Colour, 1.f);
}
