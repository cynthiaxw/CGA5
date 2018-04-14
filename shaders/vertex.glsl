// ==========================================================================
// Vertex program for barebones GLFW boilerplate
//
// Author:  Sonny Chan, University of Calgary
// Date:    December 2015
// ==========================================================================
#version 410

// location indices for these attributes correspond to those specified in the
// InitializeGeometry() function of the main program
layout(location = 0) in vec3 VertexPosition;
layout(location = 1) in vec2 TextureCoord;

uniform mat4 modelViewProjection;
uniform mat4 modelMatrix;

out vec2 Texcoord;
out vec3 Vertexp;
out vec3 center;

void main()
{
    vec4 tmp = modelMatrix * vec4(0,0,0,1);
    center.x = tmp.x;
    center.y = tmp.y;
    center.z = tmp.z;
    tmp = modelMatrix * vec4(VertexPosition, 1.0);
    Vertexp.x = tmp.x;
    Vertexp.y = tmp.y;
    Vertexp.z = tmp.z;
    // assign vertex position without modification
    gl_Position = modelViewProjection * modelMatrix * vec4(VertexPosition, 1.0);
    Texcoord = TextureCoord;
}
