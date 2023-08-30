
#pragma once
#include "variant/variant_guard.hpp"
#include "fmi2_headers/fmi2TypesPlatform.h"
#include <string>


// variants does not allow repeated types;
// unfortunately, in the default implementation, both fmi2Boolean and fmi2Integer point to a int type
// that's why, by default, FmuVariable::PtrType and FmuVariable::StartType does not have both of them
// however, in case the user has its own custom types definition, it is required to define new valid variants;
using FmuVariablePtrType = varns::variant<fmi2Real*, fmi2Integer*, fmi2String*>;
using FmuVariableStartType = varns::variant<fmi2Real, fmi2Integer, std::string>;
