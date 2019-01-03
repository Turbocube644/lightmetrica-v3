/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "component.h"
#include "math.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

/*!
    \addtogroup accel
    @{
*/

/*!
    \brief Ray-triangles acceleration structure.
*/
class Accel : public Component {
public:
    /*!
        \brief Build acceleration structure.
    */
    virtual void build(const Scene& scene) = 0;

    /*!
        \brief Compute closest intersection point.
    */
    struct Hit {
        Float t;        // Distance to the hit point
        Vec2 uv;        // Barycentric coordinates
        int primitive;  // Primitive index
        int face;       // Face index
    };
    virtual std::optional<Hit> intersect(Ray ray, Float tmin, Float tmax) const = 0;
};

/*!
    @}
*/

LM_NAMESPACE_END(LM_NAMESPACE)
