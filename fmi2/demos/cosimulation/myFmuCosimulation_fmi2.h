// =============================================================================
// Example FMU for co-simulation (FMI 2.0 standard)
// =============================================================================

#pragma once

#include <vector>
#include <array>
#include <fstream>

// #define FMI2_FUNCTION_PREFIX MyModel_
#include "fmi2/FmuToolsExport.h"

class myFmuComponent : public FmuComponentBase {
  public:
    myFmuComponent(fmi2String instanceName,
                   fmi2Type fmuType,
                   fmi2String fmuGUID,
                   fmi2String fmuResourceLocation,
                   const fmi2CallbackFunctions* functions,
                   fmi2Boolean visible,
                   fmi2Boolean loggingOn)
        : FmuComponentBase(
              instanceName,
              fmuType,
              fmuGUID,
              fmuResourceLocation,
              functions,
              visible,
              loggingOn,
              {{"logEvents", true},
               {"logSingularLinearSystems", true},
               {"logNonlinearSystems", true},
               {"logStatusWarning", true},
               {"logStatusError", true},
               {"logStatusPending", true},
               {"logDynamicStateSelection", true},
               {"logStatusDiscard", true},
               {"logStatusFatal", true},
               {"logAll", true}},
              {"logStatusWarning", "logStatusDiscard", "logStatusError", "logStatusFatal", "logStatusPending"}) {
        initializeType(fmuType);

        /// FMU ACTION: in constructor:
        /// - list the log categories that this FMU should handle, together with a flag that specifies if it
        /// they have to be enabled dy default;
        /// - list, among the log categories, which are those to be considered as debug

        /// FMU_ACTION: define new units if needed
        UnitDefinitionType UD_J("J");
        UD_J.kg = 1;
        UD_J.m = 2;
        UD_J.s = -2;
        addUnitDefinition(UD_J);

        /// FMU_ACTION: declare relevant variables
        AddFmuVariable(&q_t[0], "x_tt", FmuVariable::Type::Real, "m/s2", "cart acceleration");
        AddFmuVariable(&q[0], "x_t", FmuVariable::Type::Real, "m/s", "cart velocity");
        AddFmuVariable(&q[1], "x", FmuVariable::Type::Real, "m", "cart position");
        AddFmuVariable(&q_t[2], "theta_tt", FmuVariable::Type::Real, "rad/s2", "pendulum ang acceleration");
        AddFmuVariable(&q[2], "theta_t", FmuVariable::Type::Real, "rad/s", "pendulum ang velocity");
        AddFmuVariable(&q[3], "theta", FmuVariable::Type::Real, "rad", "pendulum angle");
        auto& fmu_len = AddFmuVariable(&len, "len", FmuVariable::Type::Real, "m", "pendulum length",
                                       FmuVariable::CausalityType::parameter, FmuVariable::VariabilityType::fixed);
        auto& fmu_m = AddFmuVariable(&m, "m", FmuVariable::Type::Real, "kg", "pendulum mass",
                                     FmuVariable::CausalityType::parameter, FmuVariable::VariabilityType::fixed);
        auto& fmu_M = AddFmuVariable(&M, "M", FmuVariable::Type::Real, "kg", "cart mass",
                                     FmuVariable::CausalityType::parameter, FmuVariable::VariabilityType::fixed);

        /// FMU_ACTION: you can also pass std::functions to get/set the value of the variable if queried
        // AddFmuVariable(std::make_pair(
        //     std::function<fmi2Real()>([this]() -> fmi2Real {
        //         return (0.5*(this->m*this->len*this->len/3)*(this->q_t[2]*this->q_t[2]));}),
        //     std::function<void(fmi2Real)>([this](fmi2Real) {})),
        //     "kineticEnergy",    FmuVariable::Type::Real, "J",    "kinetic energy");

        // same result is achieved through helper function 'MAKE_FUN_PAIR'
        AddFmuVariable(
            MAKE_GETSET_PAIR(fmi2Real,
                             { return (0.5 * (this->m * this->len * this->len / 3) * (this->q_t[2] * this->q_t[2])); },
                             {}),
            "kineticEnergy", FmuVariable::Type::Real, "J", "kinetic energy");

        // FMU_ACTION: start value will be automatically grabbed from 'len' during AddFmuVariable;

        filename = "myData.txt";
        auto& fmu_Filename =
            AddFmuVariable(&filename, "filename", FmuVariable::Type::String, "kg", "additional mass on cart",
                           FmuVariable::CausalityType::parameter, FmuVariable::VariabilityType::fixed);

        auto& fmu_approximateOn =
            AddFmuVariable(&approximateOn, "approximateOn", FmuVariable::Type::Boolean, "1", "use approximated model",
                           FmuVariable::CausalityType::parameter, FmuVariable::VariabilityType::fixed);

        // Additional commands
        q = {0, 0, 0, 3.14159265358979323846 / 4};
    }

    virtual void _enterInitializationMode() override {}

    virtual void _exitInitializationMode() override {
        std::string myfile_location = m_resources_location + "/" + filename;
        std::ifstream resfile(myfile_location);

        if (!resfile.good()) {
            sendToLog("Unable to open required file: " + myfile_location + "; check if 'resources' folder is set",
                      fmi2Status::fmi2Fatal, "logStatusFatal");
            throw std::runtime_error("Unable to open required file: " + myfile_location);
        }

        double additional_mass = 0;
        if (resfile >> additional_mass) {
            M += additional_mass;
        } else {
            throw std::runtime_error("Expected number in: " + myfile_location);
        }

        resfile.close();
        sendToLog("Loaded additional cart mass " + std::to_string(additional_mass) + " from " + filename,
                  fmi2Status::fmi2OK, "logAll");
    }

    virtual ~myFmuComponent() {}

    /// FMU_ACTION: override DoStep of the base class with the problem-specific implementation
    virtual fmi2Status _doStep(fmi2Real currentCommunicationPoint,
                               fmi2Real communicationStepSize,
                               fmi2Boolean noSetFMUStatePriorToCurrentPoint) override;

    // Problem-specific functions
    double get_x_tt(double th_t, double th);
    double get_th_tt(double th_t, double th);
    double get_x_tt_linear(double th_t, double th);
    double get_th_tt_linear(double th_t, double th);
    void get_q_t(const std::array<double, 4>& _q, std::array<double, 4>& q_t);

  protected:
    /// FMU_ACTION: override the base methods so to retrieve the proper answer
    virtual bool is_cosimulation_available() const override { return true; }
    virtual bool is_modelexchange_available() const override { return false; }

    // Problem-specific data members
    std::array<double, 4> q;
    std::array<double, 4> q_t;
    double len = 0.5;
    double m = 1.0;
    double M = 1.0;
    const double g = 9.81;
    fmi2Boolean approximateOn = fmi2False;
    std::string filename;
    std::array<double, 4> k1 = {0, 0, 0, 0};
    std::array<double, 4> k2 = {0, 0, 0, 0};
    std::array<double, 4> k3 = {0, 0, 0, 0};
    std::array<double, 4> k4 = {0, 0, 0, 0};

    const bool cosim_available = true;
    const bool modelexchange_available = false;
};
