#version 450 core

in vec2 fragCoord;
uniform vec2 iResolution;
uniform int iFrame;
uniform float delta;
uniform float exposure;  // EV stops
uniform int srgbOutput;  // encode linear radiance for the display
uniform int perPixelCount; // 1: divide by the per-pixel sample count in alpha (adaptive sampling)
uniform int showAdaptive;  // tint the pixels adaptive sampling is still working on
uniform sampler2D adaptiveSampler; // a: -1 converged, else noise estimate / threshold (0: none yet)

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

    // Display transform only: average the accumulated samples, apply exposure, encode.
    vec3 radiance = texColor.rgb / (perPixelCount != 0 ? max(texColor.a, 1.0) : float(max(iFrame, 1)));
    vec3 c = clamp(radiance * exp2(exposure), 0.0, 1.0);
    if (srgbOutput != 0)
        c = mix(c * 12.92, 1.055 * pow(c, vec3(1.0 / 2.4)) - 0.055, step(0.0031308, c));
    if (showAdaptive != 0) {
        // Overlay, red -> yellow -> green: how far each pixel's noise is from the threshold
        // (red: 16x or more, or not measured yet; green: converged).
        float st = texture(adaptiveSampler, ouv).a;
        float t = st < 0.0 ? 0.0 : (st <= 0.0 ? 1.0 : clamp(log2(max(st, 1.0)) / 4.0, 0.0, 1.0));
        vec3 tint = t < 0.5 ? mix(vec3(0.1, 0.9, 0.1), vec3(1.0, 0.85, 0.05), t * 2.0)
                            : mix(vec3(1.0, 0.85, 0.05), vec3(1.0, 0.1, 0.05), t * 2.0 - 1.0);
        c = mix(c, tint, 0.4);
    }
    FragColor = vec4(c, 1.0);
}
