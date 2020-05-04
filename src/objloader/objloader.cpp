/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/objloader.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE::objloader)

using Instance = comp::detail::ContextInstance<OBJLoaderContext>;

LM_PUBLIC_API void init(const std::string& type, const Json& prop) {
    Instance::init("objloader::" + type, prop);
}

LM_PUBLIC_API void shutdown() {
    Instance::shutdown();
}

LM_PUBLIC_API bool load(
    const std::string& path,
    OBJSurfaceGeometry& geo,
    const ProcessMeshFunc& process_mesh,
    const ProcessMaterialFunc& process_material) {
    return Instance::get().load(path, geo, process_mesh, process_material);
}

LM_NAMESPACE_END(LM_NAMESPACE::objloader)
