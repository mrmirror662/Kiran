#version 430
out vec2 fragCoord;

void main() {
    vec2 positions[4] = vec2[](
        vec2(-1.0,  1.0),
        vec2(-1.0, -1.0),
        vec2( 1.0, -1.0),
        vec2( 1.0,  1.0)
    );

    vec2 texCoords[4] = vec2[](
        vec2(0.0, 1.0),
        vec2(0.0, 0.0),
        vec2(1.0, 0.0),
        vec2(1.0, 1.0)
    );

    gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);

    fragCoord = texCoords[gl_VertexID];
}