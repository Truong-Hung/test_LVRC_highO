#version 460

void main()
{
    vec2 xy = vec2((gl_VertexID & 1) << 2, (gl_VertexID & 2) << 1);

    gl_Position = vec4(xy*.5 - 1., 0., 1.);
}