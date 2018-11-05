/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/camera.h>
#include <lm/film.h>
#include <lm/json.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

class Camera_Pinhole final : public Camera {
private:
    Film* film_;      // Underlying film
    Vec3 position_;   // Sensor position
    Vec3 u_, v_, w_;  // Basis for camera coordinates
    Float tf_;        // Half of the screen height at 1 unit forward from the position
    Float aspect_;    // Aspect ratio

public:
    virtual Component* underlying(const std::string& name) const {
        return film_;
    }

    virtual bool construct(const Json& prop) override {
        film_ = parent()->underlying<Film>(prop["aspect"]);      // Film
        position_ = castFromJson<Vec3>(prop["position"]);        // Camera position
        const auto center = castFromJson<Vec3>(prop["center"]);  // Look-at position
        const auto up = castFromJson<Vec3>(prop["up"]);          // Up vector
        const Float fv = prop["vfov"];                           // Vertical FoV
        tf_ = tan(fv * Pi / 180_f * .5_f);  // Precompute half of screen height
        // Compute basis
        w_ = glm::normalize(position_ - center);
        u_ = glm::normalize(glm::cross(up, w_));
        v_ = cross(w_, u_);
        return true;
    }

    virtual Ray primaryRay(Vec2 rp) const override {
        rp = 2_f*rp-1_f;
        const auto d = -glm::normalize(Vec3(film_->aspectRatio()*tf_*rp.x, tf_*rp.y, 1_f));
        return { position_, u_*d.x+v_*d.y+w_*d.z };
    }
};

LM_COMP_REG_IMPL(Camera_Pinhole, "camera::pinhole");

LM_NAMESPACE_END(LM_NAMESPACE)