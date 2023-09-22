#ifndef IS_INVOCABLE_H
#define IS_INVOCABLE_H

#include "../variant/variant_guard.hpp"

//#if _HAS_CXX17
//    namespace is_invocable_ns = std;
//#else


    #include <iostream>
    #include <functional>
    #include <type_traits>
    // Define a custom is_invocable type trait for C++14
    template <typename T, typename = void>
    struct is_callable : std::false_type {};

    template <typename R, typename... Args>
    struct is_callable<std::function<R(Args...)>> : std::true_type {};

    template <typename T>
    typename std::enable_if<std::is_class<T>::value, typename T::result_type>::type is_callable_test(const T&){}

    template <typename T>
    typename std::enable_if<!std::is_class<T>::value, T>::type is_callable_test(const T&){}

    template <typename T>
    struct is_callable<T, decltype(is_callable_test(std::declval<T>()))> : std::true_type {};

    template <typename T>
    bool is_invocable(T myVariant) {
        return (varns::visit([](const auto& value) { return is_callable<decltype(value)>::value; }, myVariant));
    }

//#endif



#endif  // IS_INVOCABLE_H