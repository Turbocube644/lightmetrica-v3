/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <lm/lm.h>
#include <iostream>

// ----------------------------------------------------------------------------

class Material_VisualizeNormal final : public lm::Material {
public:
    virtual bool construct(const lm::Json& prop) override {
        LM_UNUSED(prop);
        return true;
    }

    virtual std::optional<lm::RaySample> sampleRay(lm::Rng& rng, const lm::SurfacePoint& sp, lm::Vec3 wi) const override {
        LM_UNUSED(rng, sp, wi);
        LM_UNREACHABLE_RETURN();
    }

    virtual std::optional<lm::Vec3> reflectance(const lm::SurfacePoint& sp) const override {
        return glm::abs(sp.n);
    }
};

LM_COMP_REG_IMPL(Material_VisualizeNormal, "material::visualize_normal");

// ----------------------------------------------------------------------------

// This example illustrates how to extend features of Lightmetrica
// by creating simple material extension.
int main(int argc, char** argv) {
    try {
        // Initialize the framework
        lm::init({
            #if LM_DEBUG_MODE
            {"numThreads", -1}
            #else
            {"numThreads", -1}
            #endif
        });

        // Parse command line arguments
        const auto opt = lm::parsePositionalArgs<13>(argc, argv, R"({{
            "obj": "{}",
            "out": "{}",
            "w": {},
            "h": {},
            "eye": [{},{},{}],
            "lookat": [{},{},{}],
            "vfov": {}
        }})");

        // --------------------------------------------------------------------

        // Define assets

        // Film for the rendered image
        lm::asset("film1", "film::bitmap", {
            {"w", opt["w"]},
            {"h", opt["h"]}
        });

        // Pinhole camera
        lm::asset("camera1", "camera::pinhole", {
            {"film", "film1"},
            {"position", opt["eye"]},
            {"center", opt["lookat"]},
            {"up", {0,1,0}},
            {"vfov", opt["vfov"]}
        });

        // OBJ model
        // Replace all materials to diffuse and use our checker texture
        lm::asset("obj1", "model::wavefrontobj", {
            {"path", opt["obj"]},
            {"base_material", "material::visualize_normal"}
        });

        // --------------------------------------------------------------------

        // Define scene primitives

        // Camera
        lm::primitive(lm::Mat4(1), { {"camera", "camera1"} });

        // Create primitives from model asset
        lm::primitives(lm::Mat4(1), "obj1");

        // --------------------------------------------------------------------

        // Render an image
        lm::render("renderer::raycast", "accel::sahbvh", {
            {"output", "film1"},
            {"use_constant_color", true}
        });

        // Save rendered image
        lm::save("film1", opt["out"]);
    }
    catch (const std::exception& e) {
        LM_ERROR("Runtime error: {}", e.what());
    }

    return 0;
}