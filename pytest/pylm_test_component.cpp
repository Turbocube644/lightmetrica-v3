/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch_pylm.h>
#define LM_TEST_INTERFACE_REG_IMPL
#include <test_interface.h>
#include <lm/pylm.h>

namespace py = pybind11;
using namespace py::literals;

LM_NAMESPACE_BEGIN(lmtest)

class PyTestBinder_Component : public lm::PyBinder {
public:
    virtual void bind(py::module& m) const {
        // Define a trampoline class (see ref) for the interface A
        // https://pybind11.readthedocs.io/en/stable/advanced/classes.html
        struct A_Py final : public A {
            PYLM_SERIALIZE_IMPL(A)
            virtual void construct(const lm::Json& prop) override {
                PYBIND11_OVERLOAD(void, A, construct, prop);
            }
            virtual int f1() override {
                PYBIND11_OVERLOAD_PURE(int, A, f1);
            }
            virtual int f2(int a, int b) override {
                PYBIND11_OVERLOAD_PURE(int, A, f2, a, b);
            }
        };
        // You must not add .def() for construct() function
        // which is already registered in parent class.
        py::class_<A, A_Py, Ptr<A>>(m, "A")
            .def(py::init<>())
            .def("f1", &A::f1)
            .def("f2", &A::f2)
            .PYLM_DEF_COMP_BIND(A);

        // ----------------------------------------------------------------------------------------

        struct TestPlugin_Py final : public TestPlugin {
            virtual void construct(const lm::Json& prop) override {
                PYBIND11_OVERLOAD(void, TestPlugin, construct, prop);
            }
            virtual int f() override {
                PYBIND11_OVERLOAD_PURE(int, TestPlugin, f);
            }
        };
        py::class_<TestPlugin, TestPlugin_Py, Ptr<TestPlugin>>(m, "TestPlugin")
            .def(py::init<>())
            .def("f", &TestPlugin::f)
            .PYLM_DEF_COMP_BIND(TestPlugin);

        // ----------------------------------------------------------------------------------------

        struct D_Py final : public D {
            virtual void construct(const lm::Json& prop) override {
                PYBIND11_OVERLOAD(void, D, construct, prop);
            }
            virtual int f() override {
                PYBIND11_OVERLOAD_PURE(int, D, f);
            }
        };
        py::class_<D, D_Py, lm::Component, Ptr<D>>(m, "D")
            .def(py::init<>())
            .def("f", &D::f)
            .PYLM_DEF_COMP_BIND(D);

        // ----------------------------------------------------------------------------------------

        m.def("create_A1", []() {
            return dynamic_cast<A*>(
                lm::comp::detail::create_comp("test::comp::a1"));
        });

        m.def("create_test_plugin", []() {
            // Use .release() as pybind11 does not support direct conversion of Ptr<T> types
            return lm::comp::create<TestPlugin>("testplugin::default", "").release();
        });

        m.def("use_A", [](A* a) -> int {
            return a->f1() * 2;
        });

        m.def("create_A4_and_call_funcs", []() -> std::tuple<int, int> {
            // test::comp::a4 should be defined inside Python script
            int v1, v2;
            {
                auto p = lm::comp::create<A>("test::comp::a4", "");
                v1 = p->f1();
                v2 = p->f2(2, 3);
            }
            return { v1, v2 };
        });

        m.def("create_A5_and_call_funcs", []() -> std::tuple<int, int> {
            int v1, v2;
            {
                auto p = lm::comp::create<A>("test::comp::a5", "", {{"v", 7}});
                v1 = p->f1();
                v2 = p->f2(1, 2);
            }
            return { v1, v2 };
        });

        // ----------------------------------------------------------------------------------------

        m.def("round_trip_serialized_A", []() -> int {
            // Create instance registered in Python side
            auto p = lm::comp::create<A>("test::comp::serializable", "", {{"v",23}});

            // Serialize it
            std::ostringstream os;
            {
                lm::OutputArchive ar(os);
                p->save(ar);
            }
            const auto serialized = os.str();
            
            // Create another instance and deserialize it
            auto p2 = lm::comp::create_without_construct<A>("test::comp::serializable", "");
            {
                std::istringstream is(serialized);
                lm::InputArchive ar(is);
                p2->load(ar);
            }

            const auto v = p2->f1();
            return v;
        });

        m.def("round_trip_serialized_A_use_serial", []() -> int {
            auto p = lm::comp::create<A>("test::comp::serializable", "", {{"v",23}});
            std::stringstream ss;
            lm::serial::save_comp(ss, p, "");
            lm::Component::Ptr<A> p2;
            lm::serial::load_comp(ss, p2, "");
            return p2->f1();
        });

        m.def("round_trip_serialized_A_with_pickle", []() -> int {
            auto p = lm::comp::create<A>("test::comp::serializable_with_pickle", "", {
                {"v1",5}, {"v2",43}
            });
            std::stringstream ss;
            lm::serial::save_comp(ss, p, "");
            lm::Component::Ptr<A> p2;
            lm::serial::load_comp(ss, p2, "");
            return p2->f1();
        });
    }
};

LM_COMP_REG_IMPL(PyTestBinder_Component, "pytestbinder::component");

LM_NAMESPACE_END(lmtest)
