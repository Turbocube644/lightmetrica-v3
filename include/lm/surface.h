/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "math.h"

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

/*!
    \addtogroup scene
    @{
*/

/*!
    \brief Geometry information of a point inside the scene.

    \rst
    This structure represents a point inside the scene,
    which includes a surface point, point in the air, or point at infinity, etc.
    This structure is a basic quantity to be used in various renderer-related functions,
    like sampling or evaluation of terms.
    The structure represents three types of points.

    (1) *A point on a scene surface*.
        The structure describes a point on the scene surface if ``degenerated=false``.
        You can access the following associated information.

        - Position ``p``
        - Shading normal ``n``
        - Texture coordinates ``t``
        - Tangent vectors ``u`` and ``v``

    (2) *A point in a media*.
        The sturcture describes a point in a media if ``degenerated=true``,
        for instance used to represent a position of point light or pinhole camera.
        The associated information is the position ``p``.

    (3) *A point at infinity*.
        The structure describes a point at infinity if ``infinite=true``,
        used to represent a point generated by directional light or environment light.
        This point does not represent an actual point associated with certain position
        in the 3d space, but it is instead used to simplify the renderer interfaces.
        The associated information is the direction from the point at infinity ``wo``.
    \endrst
*/
struct PointGeometry {
    bool degenerated;       //!< True if surface is degenerated (e.g., point light).
    bool infinite;          //!< True if the point is a point at infinity.
    Vec3 p;                 //!< Position.
    union {
        Vec3 n;             //!< Shading normal.
        Vec3 wo;            //!< Direction from a point at infinity.
    };
    Vec2 t;                 //!< Texture coordinates.
    Vec3 u, v;              //!< Orthogonal tangent vectors.

    /*!
        \brief Make degenerated point.
        \param p Position.

        \rst
        A static function to generate a point in the air
        from the specified position ``p``.
        \endrst
    */
    static PointGeometry makeDegenerated(Vec3 p) {
        PointGeometry geom;
        geom.degenerated = true;
        geom.infinite = false;
        geom.p = p;
        return geom;
    }

    /*!
        \brief Make a point at infinity.
        \param wo Direction from a point at infinity.

        \rst
        A static function to generate a point at infinity
        from the specified direction from the point.
        \endrst
    */
    static PointGeometry makeInfinite(Vec3 wo) {
        PointGeometry geom;
        geom.degenerated = false;
        geom.infinite = true;
        geom.wo = wo;
        return geom;
    }

    /*!
        \brief Make a point on surface.
        \param p Position.
        \param n Normal.
        \param t Texture coordinates.

        \rst
        A static function to generate a point on the scene surface
        from the specified surface geometry information.
        \endrst
    */
    static PointGeometry makeOnSurface(Vec3 p, Vec3 n, Vec2 t) {
        PointGeometry geom;
        geom.degenerated = false;
        geom.infinite = false;
        geom.p = p;
        geom.n = n;
        geom.t = t;
        std::tie(geom.u, geom.v) = math::orthonormalBasis(n);
        return geom;
    }

    /*!
        \brief Make a point on surface.
        \param p Position.
        \param n Normal.

        \rst
        A static function to generate a point on the scene surface
        from the specified surface geometry information.
        \endrst
    */
    static PointGeometry makeOnSurface(Vec3 p, Vec3 n) {
        return makeOnSurface(p, n, {});
    }

    /*!
        \brief Checks if two directions lies in the same half-space.
        \param w1 First direction.
        \param w2 Second direction.

        \rst
        This function checks if two directions ``w1`` and ``w2``
        lies in the same half-space divided by the tangent plane.
        ``w1`` and ``w2`` are interchangeable.
        \endrst
    */
    bool opposite(Vec3 w1, Vec3 w2) const {
        return glm::dot(w1, n) * glm::dot(w2, n) <= 0;
    }

    /*!
        \brief Return orthonormal basis according to the incident direction.
        \param wi Incident direction.

        \rst
        The function returns an orthornormal basis according to the incident direction ``wi``.
        If ``wi`` is coming below the surface, the orthonormal basis are created
        based on the negated normal vector. This function is useful to support two-sided materials.
        \endrst
    */
    std::tuple<Vec3, Vec3, Vec3> orthonormalBasis(Vec3 wi) const {
        const int i = glm::dot(wi, n) > 0;
        return { i ? n : -n, u, i ? v : -v };
    }
};

namespace SurfaceComp {
    enum {
        All = -1,
        DontCare = 0,
    };
}

/*!
    \brief Terminator type.
*/
enum class TerminatorType {
    Camera,
    Light,
};

/*!
    \brief Scene interaction.

    \rst
    This structure represents a point of interaction between a light and the scene.
    The point represents a scattering point of an endpoint of a light transportation path,
    defined either on a surface or in a media.
    The point is associated with a geometry information around the point
    and an index of the primitive.
    The structure is used by the functions of :cpp:class:`lm::Scene` class.
    Also, the structure can represent special terminator
    representing the sentinels of the light path, which can be used to provide
    conditions to determine subspace of endpoint.
    \endrst
*/
struct SceneInteraction {
    int primitive;          //!< Primitive node index.
    int comp;               //!< Component index.
    PointGeometry geom;     //!< Surface point geometry information.
    bool endpoint;          //!< True if endpoint of light path.
    bool medium;            //!< True if it is medium interaction.
    std::optional<TerminatorType> terminator;   //!< Terminator type.

    // Information associated to terminator on camera
    struct {
        Vec4 window;
        Float aspectRatio;
    } cameraCond;

    /*!
        \brief Make surface interaction.
        \param primitive Primitive index.
        \param comp Component index.
        \param geom Point geometry.
        \return Created scene interaction.
    */
    static SceneInteraction makeSurfaceInteraction(int primitive, int comp, const PointGeometry& geom) {
        SceneInteraction si;
        si.primitive = primitive;
        si.comp = comp;
        si.geom = geom;
        si.endpoint = false;
        si.medium = false;
        si.terminator = {};
        return si;
    }

    /*!
        \brief Make medium interaction.
        \param primitive Primitive index.
        \param comp Component index.
        \param geom Point geometry.
        \return Created scene interaction.
    */
    static SceneInteraction makeMediumInteraction(int primitive, int comp, const PointGeometry& geom) {
        SceneInteraction si;
        si.primitive = primitive;
        si.comp = comp;
        si.geom = geom;
        si.endpoint = false;
        si.medium = true;
        si.terminator = {};
        return si;
    }

    /*!
        \brief Make camera endpoint.
        \param primitive Primitive index.
        \param comp Component index.
        \param geom Point geometry.
        \return Created scene interaction.
    */
    static SceneInteraction makeCameraEndpoint(int primitive, int comp, const PointGeometry& geom, Vec4 window, Float aspectRatio) {
        SceneInteraction si;
        si.primitive = primitive;
        si.comp = comp;
        si.geom = geom;
        si.endpoint = true;
        si.medium = false;
        si.terminator = {};
        si.cameraCond.window = window;
        si.cameraCond.aspectRatio = aspectRatio;
        return si;
    }

    /*!
        \brief Make light endpoint.
    */
    static SceneInteraction makeLightEndpoint(int primitive, int comp, const PointGeometry& geom) {
        SceneInteraction si;
        si.primitive = primitive;
        si.comp = comp;
        si.geom = geom;
        si.endpoint = true;
        si.medium = false;
        si.terminator = {};
        return si;
    }

    /*!
        \brief Make camera terminator.
    */
    static SceneInteraction makeCameraTerminator(Vec4 window, Float aspectRatio) {
        SceneInteraction si;
        si.endpoint = false;
        si.medium = false;
        si.terminator = TerminatorType::Camera;
        si.cameraCond.window = window;
        si.cameraCond.aspectRatio = aspectRatio;
        return si;
    }
};

/*!
    @}
*/

LM_NAMESPACE_BEGIN(surface)

/*!
    \addtogroup scene
    @{
*/

/*!
    \brief Compute geometry term.
    \param s1 First point.
    \param s2 Second point.

    \rst
    This function computes the geometry term :math:`G(\mathbf{x}\leftrightarrow\mathbf{y})`
    from the two surface points :math:`\mathbf{x}` and :math:`\mathbf{y}`.
    This function can accepts extended points represented by :cpp:class:`lm::PointGeometry`.
    \endrst
*/
static Float geometryTerm(const PointGeometry& s1, const PointGeometry& s2) {
    Vec3 d = s2.p - s1.p;
    const Float L2 = glm::dot(d, d);
    d = d / std::sqrt(L2);
    const auto cos1 = s1.degenerated ? 1_f : glm::abs(glm::dot(s1.n, d));
    const auto cos2 = s2.degenerated ? 1_f : glm::abs(glm::dot(s2.n, -d));
    return cos1 * cos2 / L2;
}

/*!
    \brief Copmute distance between two points.
    \param s1 First point.
    \param s2 Second point.

    \rst
    This function computes the distance between two points.
    If one of the points is a point at infinity, returns inf.
    \endrst
*/
static Float distance(const PointGeometry& s1, const PointGeometry& s2) {
    return s1.infinite || s2.infinite ? Inf : glm::distance(s1.p, s2.p);
}

/*!
    @}
*/

LM_NAMESPACE_END(surface)
LM_NAMESPACE_END(LM_NAMESPACE)
