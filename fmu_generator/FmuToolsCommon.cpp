#include "FmuToolsCommon.h"

bool is_pointer_variant(const FmuVariableBindType& myVariant) {
    return varns::visit([](auto&& arg) -> bool {
        return std::is_pointer_v<std::decay_t<decltype(arg)>>;
    }, myVariant);
}