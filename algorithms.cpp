//----------------------------------------------------------------------------------------
/**
 * \file    algorithms.cpp
 * \author  vaclaon3
 * \brief   Implementace CPU Gerstner wave algoritmu.
 */
 //----------------------------------------------------------------------------------------

#include "algorithms.h"
#include <cmath>

namespace vaclaon3 {
    static void gerstnerWaveContrib(
        glm::vec2  xzPos,       // world XZ souradnice bodu (stejny vstup jako v shaderu)
        glm::vec2  dir,         // smer sireni vlny (normalizovany 2D vektor)
        float      amplitude,   // vyska vlny
        float      wavelength,  // delka vlny (vzdalenost mezi hrbem a hrbem)
        float      speed,       // rychlost sireni
        float      steepness,   // ostrost hrbu (0 = hladky sinus, vyssi = ostrejsi hrb)
        float      phase,       // fazovy posun � posuva vlnu v prostoru (ruzne vlny nezac�naj� na stejnem miste)
        float      time,        // aktualni cas v sekundach
        glm::vec3& displacement,// inout � funkce do toho PRICITA svuj prispevek, neresetuje
        glm::vec3& nrm          // inout � stejne, pricita prispevek k normale
    ) {
        const float PI = 3.14159265f;

        // Vlnove cislo � kolikrat se vlna "otoci" na jednotku delky
        // Kratsi vlnova delka = vyssi k = vice oscilaci na stejne vzdalenosti
        float k = 2.0f * PI / wavelength;

        // Uhlova rychlost � jak rychle se meni faze v case
        float omega = speed * k;

        // Fazovy uhel tohoto bodu v teto vlne
        // dot(dir, xzPos) = vzdalenost bodu ve smeru sireni vlny
        // - omega * time  = posun vlny v case (vlna "uj�d�" dopredu)
        // + phase         = individualni posun teto vlnove komponenty
        float phi = k * glm::dot(dir, xzPos) - omega * time + phase;

        // Normalizovany koeficient ostrosti
        // Delime k*amplitude aby steepness=1 byl fyzikalni maximum (bez prevratu geometrie)
        float Q = steepness / (k * amplitude);

        // XZ displacement � vrcholy se nekrouzi jen nahoru/dolu ale i do strany
        // To dava Gerstnerove vlne charakteristicky ostry hrb (narozdil od sine wave)
        displacement.x += Q * amplitude * dir.x * std::cos(phi);
        displacement.z += Q * amplitude * dir.y * std::cos(phi);  // dir.y zde = Z slozka smeru

        // Y displacement � vertikalni pohyb
        displacement.y += amplitude * std::sin(phi);

        // Normala � analytick� derivace displacement funkce
        // Misto numerick�ho odhadu (sousedni body) pouzivame matematicky presny vzorec
        nrm.x -= dir.x * k * amplitude * std::cos(phi);
        nrm.z -= dir.y * k * amplitude * std::cos(phi);
        nrm.y -= Q * k * amplitude * std::sin(phi);
    }
    GerstnerResult evaluateGerstner(glm::vec2 worldXZ, float time) {

        // Zac�n�me od nuloveho posunu a normaly smerujici nahoru (klidna hladina)
        glm::vec3 disp = glm::vec3(0.0f);
        glm::vec3 normal = glm::vec3(0.0f, 1.0f, 0.0f);

        // Stejne vlny jako v simple-vs.glsl � stejne poradi, stejne hodnoty
        gerstnerWaveContrib(worldXZ, glm::normalize(glm::vec2(1.0f, 0.2f)), 0.800f, 25.0f, 1.0f, 0.040f, 0.0f, time, disp, normal);
        gerstnerWaveContrib(worldXZ, glm::normalize(glm::vec2(0.8f, 0.5f)), 0.600f, 20.0f, 0.9f, 0.035f, 2.3f, time, disp, normal);
        gerstnerWaveContrib(worldXZ, glm::normalize(glm::vec2(-0.4f, 1.0f)), 0.140f, 15.0f, 1.3f, 0.015f, 1.1f, time, disp, normal);
        gerstnerWaveContrib(worldXZ, glm::normalize(glm::vec2(0.3f, -0.8f)), 0.120f, 13.0f, 1.4f, 0.012f, 3.7f, time, disp, normal);
        gerstnerWaveContrib(worldXZ, glm::normalize(glm::vec2(-0.7f, 0.4f)), 0.120f, 10.0f, 1.7f, 0.010f, 0.8f, time, disp, normal);
        gerstnerWaveContrib(worldXZ, glm::normalize(glm::vec2(0.6f, -0.3f)), 0.068f, 9.0f, 1.6f, 0.010f, 2.9f, time, disp, normal);
        gerstnerWaveContrib(worldXZ, glm::normalize(glm::vec2(-0.2f, -0.9f)), 0.060f, 7.0f, 2.0f, 0.008f, 4.2f, time, disp, normal);
        gerstnerWaveContrib(worldXZ, glm::normalize(glm::vec2(0.9f, 0.1f)), 0.040f, 6.0f, 2.1f, 0.007f, 1.6f, time, disp, normal);
        gerstnerWaveContrib(worldXZ, glm::normalize(glm::vec2(-0.5f, 0.7f)), 0.028f, 4.5f, 2.4f, 0.005f, 3.3f, time, disp, normal);
        gerstnerWaveContrib(worldXZ, glm::normalize(glm::vec2(0.4f, 0.6f)), 0.020f, 3.0f, 2.8f, 0.004f, 5.1f, time, disp, normal);

        // normalize() � normala po souctu vsech prispevku uz neni jednotkova,
        // normalize() ji prevede na delku 1 (shader a rotacni matice to vyzaduji)
        return { disp, glm::normalize(normal) };
    }

} // namespace vaclaon3