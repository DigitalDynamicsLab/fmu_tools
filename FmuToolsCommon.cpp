#include "FmuToolsCommon.h"


/////////////////////////////////////////////////////////////////////////////////////////////////
std::string fmi2Status_toString(fmi2Status status){
    switch (status)
    {
    case fmi2Status::fmi2Discard:
        return "Discard";
        break;
    case fmi2Status::fmi2Error:
        return "Error";
        break;
    case fmi2Status::fmi2Fatal:
        return "Fatal";
        break;
    case fmi2Status::fmi2OK:
        return "OK";
        break;
    case fmi2Status::fmi2Pending:
        return "Pending";
        break;
    case fmi2Status::fmi2Warning:
        return "Warning";
        break;
    default:
        throw std::runtime_error("Wrong fmi2Status");
        break;
    }
}

