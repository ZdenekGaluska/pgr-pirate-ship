#pragma once
//----------------------------------------------------------------------------------------
/**
 * \file    algorithms.h
 * \author  vaclaon3
 * \brief   CPU mirror GPU Gerstner wave algoritmu.
 */
 //----------------------------------------------------------------------------------------

#include "globals.h"

namespace vaclaon3 {

    /// Vysledek evaluace Gerstner vln pro jeden bod ve world space.
    struct GerstnerResult {
        glm::vec3 displacement;   // o kolik se bod posunul v XYZ oproti klidove hladine
        glm::vec3 normal;         // normala povrchu vody v tomto bode (smer "nahoru" z vlny)
    };


    GerstnerResult evaluateGerstner(glm::vec2 worldXZ, float time);

} 