#ifndef TYPESVARIANTS_H
#define TYPESVARIANTS_H

#include <string>

#include "variant/variant_guard.hpp"
#include "fmi2/fmi2_headers/fmi2TypesPlatform.h"


#define FMITYPESPLATFORM_DEFAULT

// Note: variants do not allow repeated types.
// Since in the FMI standard both fmi2Boolean and fmi2Integer point to a int type, by default neither
// If the user defines their own custom types, it is required to define new valid variants in order to
// cover all possible available types.
using FmuVariableBindType = varns::variant<                                     //
    fmi2Real*,                                                                  //
    fmi2Integer*,                                                               //
    std::string*,                                                               //
    std::pair<std::function<fmi2Real()>, std::function<void(fmi2Real)>>,        //
    std::pair<std::function<fmi2Integer()>, std::function<void(fmi2Integer)>>,  //
    std::pair<std::function<std::string()>, std::function<void(std::string)>>   //
    >;

using FmuVariableStartType = varns::variant<  //
    fmi2Real,                                 //
    fmi2Integer,                              //
    std::string                               //
    >;

#endif
