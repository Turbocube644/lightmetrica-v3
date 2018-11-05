/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <lm/lm.h>

int main(int argc, char** argv) {
    // Initialize the framework
    lm::init();

    // ------------------------------------------------------------------------

    // Define assets

    // Film for the rendered image
    lm::asset("film1", "film::bitmap", {{"w", 1920}, {"h", 1080}});

    // Pinhole camera
    lm::asset("camera1", "camera::pinhole", {
        {"film", "film1"},
        {"position", {0,0,5}},
        {"center", {0,0,0}},
        {"up", {0,1,0}},
        {"vfov", 30}
    });

    // Load mesh with raw vertex data
    lm::asset("mesh1", "mesh::raw", {
        {"ps", {-1,-1,-1,1,-1,-1,1,1,-1,-1,1,-1}},
        {"ns", {0,0,1}},
        {"ts", {0,0,1,0,1,1,0,1}},
        {"fs", {
            {"p", {0,1,2,0,2,3}},
            {"n", {0,0,0,0,0,0}},
            {"t", {0,1,2,0,2,3}}
        }}
    });

    // ------------------------------------------------------------------------

    // Define scene primitives

    // Camera
    lm::primitive(lm::Mat4(1), {{"camera", "camera1"}});

    // Mesh
    lm::primitive(lm::Mat4(1), {{"mesh", "mesh1"}});

    // ------------------------------------------------------------------------

    // Render an image
    lm::render("renderer::raycast", "accel::sahbvh", {
        {"output", "film1"},
        {"color", {0,0,0}}
    });

    // Save rendered image
    lm::save("film1", "result.pfm");

    return 0;
}