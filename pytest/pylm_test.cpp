/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch_pylm.h>
#include <lm/pylm.h>

LM_NAMESPACE_BEGIN(lmtest)

PYBIND11_MODULE(pylm_test, m) {
    m.doc() = "Lightmetrica python test module";

    // Initialize submodules
    std::regex reg(R"(pytestbinder::(\w+))");
    lm::comp::detail::foreach_registered([&](const std::string& name) {
        // Find the name matched with 'pytestbinder::*'
        std::smatch match;
        if (std::regex_match(name, match, reg)) {
            auto binder = lm::comp::create<lm::PyBinder>(name, "");
            auto submodule = m.def_submodule(match[1].str().c_str());
            binder->bind(submodule);
        }
    });
}

LM_NAMESPACE_END(lmtest)
