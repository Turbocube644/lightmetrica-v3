/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/parallelcontext.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE::parallel)

using Instance = comp::detail::ContextInstance<ParallelContext>;

LM_PUBLIC_API void init(const std::string& type, const Json& prop) {
    Instance::init(type, prop);
}

LM_PUBLIC_API void shutdown() {
    Instance::shutdown();
}

LM_PUBLIC_API int num_threads() {
    return Instance::get().num_threads();
}

LM_PUBLIC_API bool main_thread() {
    return Instance::get().main_thread();
}

LM_PUBLIC_API void foreach(long long numSamples, const ParallelProcessFunc& processFunc, const ProgressUpdateFunc& progressFunc) {
	Instance::get().foreach(numSamples, processFunc, progressFunc);
}

LM_NAMESPACE_END(LM_NAMESPACE::parallel)
