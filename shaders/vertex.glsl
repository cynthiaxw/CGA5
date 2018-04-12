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

void main()
{
    // assign vertex position without modification
    //gl_Position = modelViewProjection * vec4(VertexPosition.x, VertexPosition.y+1.0, VertexPosition.z, 1.0);
    
    //vec3 tmpvertex = (modelMatrix * vec4(VertexPosition, 1.0)).xyz;
    gl_Position = modelViewProjection * modelMatrix * vec4(VertexPosition, 1.0);

    Texcoord = TextureCoord;
}
