/**
 * @copyright (C) 2012-2020 Euclid Science Ground Segment
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 3.0 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <boost/python.hpp>
#include "Pyston/ExceptionRaiser.h"
#include "Pyston/Helpers.h"
#include "Pyston/NodeConverter.h"
#include "Pyston/Graph/Cast.h"
#include "Pyston/Graph/Constant.h"
#include "Pyston/Graph/Functors.h"
#include "Pyston/Graph/Placeholder.h"

#if BOOST_VERSION < 105600
#include <boost/units/detail/utility.hpp>
using boost::units::detail::demangle;
#else
using boost::core::demangle;
#endif

namespace py = boost::python;

namespace Pyston {


template<typename T>
struct RegisterNode {

  /**
   * Define operations where the other value has a different type (To)
   * i.e. if __add__ is called on a boolean, and the other value (To) is a float,
   * self has to be upcasted
   */
  template<typename To>
  static void defCastOperations(py::class_<Node<T>, boost::noncopyable>& node) {
    node
      .def("__add__", makeBinary<T, To, std::plus>("+"))
      .def("__sub__", makeBinary<T, To, std::minus>("-"))
      .def("__mul__", makeBinary<T, To, std::multiplies>("*"))
      .def("__truediv__", makeBinary<T, To, std::divides>("/"))
      .def("__radd__", makeBinary<T, To, std::plus>("+", true))
      .def("__rsub__", makeBinary<T, To, std::minus>("-", true))
      .def("__rmul__", makeBinary<T, To, std::multiplies>("*", true))
      .def("__rtruediv__", makeBinary<T, To, std::divides>("/", true));
  }

  /**
   * Methods for specific types
   */
  template<typename Y>
  static void specialized(py::class_<Node<Y>, boost::noncopyable>& node, void *);

  /**
   * Methods for floating point types
   */
  template<typename Y>
  static void specialized(py::class_<Node<Y>, boost::noncopyable>& node,
                          typename std::enable_if<std::is_floating_point<Y>::value>::type * = nullptr) {
    node
      .def("__pow__", makeBinary<T, T, Pow>("^"))
      .def("__rpow__", makeBinary<T, T, Pow>("^", true))
      .def("__round__", makeUnary<T, Round>("round"))
      .def("__abs__", makeUnary<T, Abs>("abs"));

    // Functions
    // When using numpy methods, numpy will delegate to these
    // Taken from here, although there are a bunch not implemented:
    // https://numpy.org/devdocs/reference/ufuncs.html
    node
      .def("exp", makeUnary<T, Exp>("exp"))
      .def("exp2", makeUnary<T, Exp2>("exp2"))
      .def("log", makeUnary<T, Log>("log"))
      .def("log2", makeUnary<T, Log2>("log2"))
      .def("log10", makeUnary<T, Log10>("log10"))
      .def("sqrt", makeUnary<T, Sqrt>("sqrt"))
      .def("sin", makeUnary<T, Sin>("sin"))
      .def("cos", makeUnary<T, Cos>("cos"))
      .def("tan", makeUnary<T, Tan>("tan"))
      .def("arcsin", makeUnary<T, ArcSin>("arcsin"))
      .def("arccos", makeUnary<T, ArcCos>("arccos"))
      .def("arctan", makeUnary<T, ArcTan>("arctan"))
      .def("sinh", makeUnary<T, Sinh>("sinh"))
      .def("cosh", makeUnary<T, Cosh>("cosh"))
      .def("tanh", makeUnary<T, Tanh>("tanh"))
      .def("arcsinh", makeUnary<T, ArcSinh>("arcsinh"))
      .def("arccosh", makeUnary<T, ArcCosh>("arccosh"))
      .def("arctanh", makeUnary<T, ArcTanh>("arctanh"));
  }

  /**
   * Methods for the boolean type
   */
  template<typename Y>
  static void specialized(py::class_<Node<Y>, boost::noncopyable>& node,
                          typename std::enable_if<std::is_same<Y, bool>::value>::type * = nullptr) {
    // Upcast to double and int
    defCastOperations<double>(node);
    defCastOperations<int64_t>(node);
  }

  /**
   * Methods for integral types, except bool
   */
  template<typename Y>
  static void specialized(py::class_<Node<Y>, boost::noncopyable>& node,
                          typename std::enable_if<std::is_integral<Y>::value &&
                                                  !std::is_same<Y, bool>::value>::type * = nullptr) {
    node
      .def("__abs__", makeUnary<T, Abs>("abs"));
    // Upcast to double
    defCastOperations<double>(node);
    node
      .def("__pow__", makeBinary<T, double, Pow>("^"))
      .def("__rpow__", makeBinary<T, double, Pow>("^", true));
  }

  static void general(py::class_<Node<T>, boost::noncopyable>& node) {
    // https://docs.python.org/3/reference/datamodel.html#basic-customization
    // Same types
    node
      .def("__lt__", makeBinary<T, T, std::less>("<"))
      .def("__le__", makeBinary<T, T, std::less_equal>("<="))
      .def("__eq__", makeBinary<T, T, std::equal_to>("=="))
      .def("__ne__", makeBinary<T, T, std::not_equal_to>("!="))
      .def("__gt__", makeBinary<T, T, std::greater>(">"))
      .def("__ge__", makeBinary<T, T, std::greater_equal>(">="));

    // https://docs.python.org/3/reference/datamodel.html#emulating-numeric-types
    // Same types
    node
      .def("__add__", makeBinary<T, T, std::plus>("+"))
      .def("__sub__", makeBinary<T, T, std::minus>("-"))
      .def("__mul__", makeBinary<T, T, std::multiplies>("*"))
      .def("__truediv__", makeBinary<T, T, std::divides>("/"))
      .def("__radd__", makeBinary<T, T, std::plus>("+", true))
      .def("__rsub__", makeBinary<T, T, std::minus>("-", true))
      .def("__rmul__", makeBinary<T, T, std::multiplies>("*", true))
      .def("__rtruediv__", makeBinary<T, T, std::divides>("/", true))
      .def("__neg__", makeUnary<T, std::negate>("-"))
      .def("__pos__", makeUnary<T, Identity>("+"));

    // Can not be used in conditionals!
    node.def("__bool__", py::make_function(
      ExceptionRaiser<T>("Can not use variable placeholders in conditionals"),
      py::default_call_policies(),
      boost::mpl::vector<void, const std::shared_ptr<Node<T>>>()
    ));
  }

  static void Do() {
    auto node_name = std::string("Node<") + demangle(typeid(T).name()) + ">";
    py::class_<Node<T>, boost::noncopyable> node(node_name.c_str(), "AST Node", py::no_init);

    // Operators and method applicable to all types
    general(node);

    // Operators and method applicable only to a given type
    // i.e. __floordiv__ is not applicable to float
    specialized<T>(node);

    // Register convertion between shared pointer and Node
    py::register_ptr_to_python<std::shared_ptr<Node<T>>>();

    // Custom conversion so primitive values can be converted to a node
    py::converter::registry::push_back(&NodeConverter<T>::isConvertible,
                                       &NodeConverter<T>::construct,
                                       py::type_id<std::shared_ptr<Node<T>>>());

    // Triggers the building of a tree
    auto placeholder_name = std::string("Placeholder<") + demangle(typeid(T).name()) + ">";
    py::class_<Placeholder<T>, py::bases<Node<T>>>(
      placeholder_name.c_str(), "Variable placeholder", py::no_init);
    py::register_ptr_to_python<std::shared_ptr<Placeholder<T>>>();
  }
};

BOOST_PYTHON_MODULE (libPyston) {
  RegisterNode<double>::Do();
  RegisterNode<int64_t>::Do();
  RegisterNode<bool>::Do();
}

} // end of namespace Pyston
