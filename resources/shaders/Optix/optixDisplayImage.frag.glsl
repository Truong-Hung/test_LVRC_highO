#version 460

layout(binding = 0) uniform sampler2D u_renderedImage;

out vec4 out_fragmentColor;

void main()
{
    const ivec2 currentPixel = ivec2(gl_FragCoord.xy);
    const vec4 currentPixelColor = texelFetch(u_renderedImage, currentPixel, 0);

    out_fragmentColor = vec4(currentPixelColor.xyz, 1.);
}