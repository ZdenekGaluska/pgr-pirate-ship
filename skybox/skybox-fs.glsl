#version 330 core
// Skybox fragment shader.
// samplerCube samples the cubemap in the direction given by v_texCoord.
// No lighting applied -- skybox is always drawn at full brightness.

uniform samplerCube u_skybox;

in  vec3 v_texCoord;
out vec4 fragColor;

void main() {
    vec3 dir = v_texCoord;
    dir.y -= 10000.0;   // tweak this value to taste
    fragColor = texture(u_skybox, v_texCoord);
}