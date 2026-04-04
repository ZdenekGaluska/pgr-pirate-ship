#version 330 core

uniform mat4  mPVM;
uniform mat4  mModel;
uniform float uWaterUVScale;
uniform float u_time;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;

out vec2 vTexCoord;
out vec3 vNormal;
out vec3 vWorldPos;

// -----------------------------------------------------------------------
// gerstnerWave() — posune jeden vrchol podle jedne vlnove komponenty.
//
// Kazda "vlna" ma parametry:
//   dir       = smer sireni vlny v rovine XZ (normalizovany 2D vektor)
//   amplitude = vyska vlny (jak vysoko se vrchol zvedne)
//   wavelength = delka vlny (vzdalenost mezi dvema hrby)
//   speed      = rychlost sírení
//   steepness  = ostrost hrbù (0 = sinus, 1 = max ostry hrb)
//
// Vystup:
//   displacement = o kolik posunout vrchol v XYZ
//   normal       = analyticky spocitana normala (pricte se k vec3(0,1,0))
// -----------------------------------------------------------------------
void gerstnerWave(
    vec2  xzPos,       // world XZ pozice vrcholu
    vec2  dir,         // smer sireni vlny
    float amplitude,
    float wavelength,
    float speed,
    float steepness,
    float phase,
    inout vec3 displacement,  // inout = funkce do toho pricita, neresetuje
    inout vec3 nrm
) {
    // k = vlnove cislo = kolikrat se vlna "otoci" na jednotku delky
    // vyssi k = kratsi vlna
    float k     = 2.0 * 3.14159265 / wavelength;

    // omega = uhlova rychlost = jak rychle se vlna pohybuje v case
    float omega = speed * k;

    // phi = fazovy posun tohoto vrcholu = kde na sinus krivce tento vrchol lezi
    // dot(dir, xzPos) = vzdalenost vrcholu ve smeru sírení vlny
    // odecteme omega * u_time = vlna se pohybuje v case
    float phi   = k * dot(dir, xzPos) - omega * u_time + phase;

    // Q = steepness / (k * amplitude) — normalizovany koeficient ostrosti
    // Delime k*amplitude aby steepness=1 byl skutecny maximum bez prevratu geometrie
    float Q     = steepness / (k * amplitude);

    // XZ displacement — vrcholy se nekrouzi jen nahoru/dolu ale i do strany
    // To dava Gerstnerove vlne charakteristicky ostry hrb
    displacement.x += Q * amplitude * dir.x * cos(phi);
    displacement.z += Q * amplitude * dir.y * cos(phi);

    // Y displacement — vertikalni pohyb vrcholu
    displacement.y += amplitude * sin(phi);

    // Analytická normála — derivace displacement funkce
    // Misto numerického odhadu (sousedni vrcholy) pouzijeme matematicky presny vzorec
    // Tím se vyhneme artefaktùm na okrajích møížky
    nrm.x -= dir.x * k * amplitude * cos(phi);
    nrm.z -= dir.y * k * amplitude * cos(phi);
    nrm.y -= Q     * k * amplitude * sin(phi);
}

void main() {
    vWorldPos = vec3(mModel * vec4(position, 1.0));

    if (uWaterUVScale > 0.0) {
        // -----------------------------------------------------------------------
        // Vodní vrchol — aplikuj Gerstner waves
        //
        // Pouzijeme dve vlnove komponenty s ruznymi smery a parametry.
        // Superpozice vice vln = realitictejsi voda nez jedna vlna.
        // -----------------------------------------------------------------------
        vec3 disp   = vec3(0.0);         // zacneme od nuloveho posunu
        vec3 nAccum = vec3(0.0, 1.0, 0.0); // zacneme od normaly smerujici nahoru (flat plane)

gerstnerWave(vWorldPos.xz, normalize(vec2(1.0, 0.2)),
    0.8, 25.0, 1.0, 0.04, 0.0, disp, nAccum);
    
gerstnerWave(vWorldPos.xz, normalize(vec2(0.8, 0.5)),
    0.6, 20.0, 0.9, 0.035, 2.3, disp, nAccum);

gerstnerWave(vWorldPos.xz, normalize(vec2(-0.4, 1.0)),
    0.14, 15.0, 1.3, 0.015, 1.1, disp, nAccum);

gerstnerWave(vWorldPos.xz, normalize(vec2(0.3, -0.8)),
    0.12, 13.0, 1.4, 0.012, 3.7, disp, nAccum);

gerstnerWave(vWorldPos.xz, normalize(vec2(-0.7, 0.4)),
    0.12, 10.0, 1.7, 0.01, 0.8, disp, nAccum);

gerstnerWave(vWorldPos.xz, normalize(vec2(0.6, -0.3)),
    0.068, 9.0, 1.6, 0.01, 2.9, disp, nAccum);

gerstnerWave(vWorldPos.xz, normalize(vec2(-0.2, -0.9)),
    0.060, 7.0, 2.0, 0.008, 4.2, disp, nAccum);

gerstnerWave(vWorldPos.xz, normalize(vec2(0.9, 0.1)),
    0.04, 6.0, 2.1, 0.007, 1.6, disp, nAccum);

gerstnerWave(vWorldPos.xz, normalize(vec2(-0.5, 0.7)),
    0.029, 4.5, 2.4, 0.005, 3.3, disp, nAccum);

gerstnerWave(vWorldPos.xz, normalize(vec2(0.4, 0.6)),
    0.02, 3.0, 2.8, 0.004, 5.1, disp, nAccum);

        // Aplikuj displacement na world pozici vrcholu
        vec3 displacedPos = position + disp;

        // Normala je uz ve world space (Gerstner ji pocita primo v world coords)
        vNormal   = normalize(nAccum);
        vTexCoord = (vWorldPos.xz + disp.xz) * uWaterUVScale;  // UV sleduje pohyb vrcholu
        vWorldPos = vec3(mModel * vec4(displacedPos, 1.0));
        gl_Position = mPVM * vec4(displacedPos, 1.0);

    } else {
        // Normalni objekt (lod, ostrovy...) — zadny displacement
        mat3 normalMatrix = transpose(inverse(mat3(mModel)));
        vNormal   = normalize(normalMatrix * normal);
        vTexCoord = texCoord;
        gl_Position = mPVM * vec4(position, 1.0);
    }
}