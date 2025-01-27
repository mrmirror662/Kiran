#version 460 core

in vec2 fragCoord;
uniform vec2 iResolution;
uniform int iFrame;
uniform float delta;

// Input from the vertex shader

// Texture sampler
uniform sampler2D textureSampler;

// Output color
out vec4 FragColor;
in vec4 gl_FragCoord;

void main()
{
    vec4 coords = gl_FragCoord;
    vec2 ouv = coords.xy / iResolution.xy;
    vec4 texColor = texture(textureSampler, ouv);

    vec4 fcolor = vec4(1.0);
    if (delta > 0.00001 || iFrame == 1)
        fcolor = texColor;
    else
        fcolor = texColor / iFrame;
    FragColor = fcolor;
}
