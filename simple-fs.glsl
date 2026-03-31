#version 330 core

uniform vec3 vDiffuse;    // barva materialu
uniform vec3 vLightDir;   // smer svetla (normalizovany)
uniform vec3 vCameraPos;  // pozice kamery
uniform sampler2D uTexture;

in vec3 vNormal;
in vec3 vWorldPos;
in vec2 vTexCoord;

out vec4 fragmentColor;

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(vLightDir);
    vec3 V = normalize(vCameraPos - vWorldPos);
    vec3 R = reflect(-L, N);

    float ambient  = 0.35;
    float diffuse  = max(dot(N, L), 0.0);
    float specular = pow(max(dot(R, V), 0.0), 64.0) * 0.4;

    vec3 texColor = texture(uTexture, vTexCoord).rgb;
    vec3 color = texColor * (ambient + diffuse) + vec3(specular);
    fragmentColor = vec4(color, 1.0);
return;
}
