#pragma once

#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <cstdarg>
#include <iostream>
#include <system_error>

#include "FmuToolsCommon.h"
#include "FmuToolsRuntimeLinking.h"

// ATTENTION: order of included headers is IMPORTANT!
//            rapidxml.hpp must be included first here.

#include "rapidxml/rapidxml.hpp"
#include "miniz-cpp/zip_file.hpp"
#include "filesystem.hpp"

#define LOAD_FMI_FUNCTION(funcName)                                                                    \
    this->_##funcName = (funcName##TYPE*)get_function_ptr(this->dynlib_handle, #funcName);             \
    if (!this->_##funcName)                                                                            \
        throw std::runtime_error(std::string(std::string("Could not find ") + std::string(#funcName) + \
                                             std::string(" in the FMU library. Wrong or outdated FMU?")));

// =============================================================================

/// Class for a node in a tree of FMU variables.
/// The tree is constructed by analyzing the flat list of variables in the XML.
/// In the XML one has the flattened list such as for example
///     myobject.mysubobjetct.pos
///     myobject.mysubobjetct.dir
/// so the tree will contain:
///     myobject
///          mysubobject
///                pos
///                dir
class FmuVariableTreeNode {
  public:
    std::string object_name;

    std::map<std::string, FmuVariableTreeNode> children;
    FmuVariable* leaf = nullptr;

    FmuVariableTreeNode() {}
};

// =============================================================================

/// Class for managing a FMU.
/// It contains functions to load the shared library in run-time, to parse the XML, to set/get variables, etc.
class FmuUnit {
  public:
    FmuUnit();
    virtual ~FmuUnit() {}

    /// Load the FMU from the directory, assuming it has been already unzipped
    virtual void LoadUnzipped(const std::string& directory);

    /// Load the FMU, optionally defining where the FMU will be unzipped (default is the temporary folder)
    void Load(const std::string& fmupath,
              const std::string& unzipdir = fs::temp_directory_path().generic_string() + std::string("/_fmu_temp"));

    /// Return the folder in which the FMU has been unzipped
    std::string GetUnzippedFolder() const { return directory; }

    /// Return version number of header files.
    std::string GetVersion() const;

    /// Return types platform.
    std::string GetTypesPlatform() const;

    /// Instantiate the model.
    void Instantiate(std::string tool_name, std::string resource_dir, bool logging = false, bool visible = false);

    /// Instantiate the model, setting as resources folder the one from the unzipped FMU.
    void Instantiate(std::string tool_name, bool logging = false, bool visible = false);

    /// Set debug logging level.
    fmi2Status SetDebugLogging(fmi2Boolean loggingOn, const std::vector<std::string>& logCategories);

    /// Set up experiment.
    fmi2Status SetupExperiment(fmi2Boolean toleranceDefined,
                               fmi2Real tolerance,
                               fmi2Real startTime,
                               fmi2Boolean stopTimeDefined,
                               fmi2Real stopTime);

    fmi2Status EnterInitializationMode();
    fmi2Status ExitInitializationMode();

    /// Advance state of the FMU from currentCommunicationPoint to currentCommunicationPoint+communicationStepSize.
    /// Available only for an FMU that iplements the CoSimulation interface.
    fmi2Status DoStep(fmi2Real currentCommunicationPoint,
                      fmi2Real communicationStepSize,
                      fmi2Boolean noSetFMUStatePriorToCurrentPoint);

    /// Set a new time instant and re-initialize caching of variables that depend on time.
    /// Available only for an FMU that iplements the ModelExchange interface.
    fmi2Status SetTime(const fmi2Real time);

    /// Set a new (continuous) state vector and re-initialize caching of variables that depend on the states. Argument
    /// nx is the length of vector x and is provided for checking purposes.
    /// Available only for an FMU that iplements the ModelExchange interface.
    fmi2Status SetContinuousStates(const fmi2Real x[], size_t nx);

    /// Compute state derivatives at the current time instant and for the current states. The derivatives are returned
    /// as a vector with nx elements.
    /// Available only for an FMU that iplements the ModelExchange interface.
    fmi2Status GetDerivatives(fmi2Real derivatives[], size_t nx);

    template <class T>
    fmi2Status GetVariable(fmi2ValueReference vr, T& value, FmuVariable::Type vartype) noexcept(false);

    template <class T>
    fmi2Status SetVariable(fmi2ValueReference vr, const T& value, FmuVariable::Type vartype) noexcept(false);

    template <class T>
    fmi2Status GetVariable(const std::string& varname, T& value, FmuVariable::Type vartype) noexcept(false);

    template <class T>
    fmi2Status SetVariable(const std::string& varname, const T& value, FmuVariable::Type vartype) noexcept(false);

    fmi2Status GetVariable(const std::string& varname, std::string& value) noexcept(false);
    fmi2Status SetVariable(const std::string& varname, const std::string& value) noexcept(false);

    fmi2Status GetVariable(const std::string& varname, bool& value) noexcept(false);
    fmi2Status SetVariable(const std::string& varname, const bool& value) noexcept(false);

  protected:
    /// Parse XML and create the list of variables.
    void LoadXML();

    /// Load the shared library in run-time and do the dynamic linking to the needed FMU functions.
    void LoadSharedLibrary();

    /// Construct a tree of variables from the flat variable list.
    void BuildVariablesTree();

    /// Dump the tree of variables (recursive)
    void DumpTree(FmuVariableTreeNode* mynode, int tab);

  public:
    std::string directory;
    std::string binaries_dir;

    std::string modelName;
    std::string guid;
    std::string fmiVersion;
    std::string description;
    std::string generationTool;
    std::string generationDateAndTime;
    std::string variableNamingConvention;
    std::string numberOfEventIndicators;

    bool cosim;
    std::string info_cosim_modelIdentifier;
    std::string info_cosim_needsExecutionTool;
    std::string info_cosim_canHandleVariableCommunicationStepSize;
    std::string info_cosim_canInterpolateInputs;
    std::string info_cosim_maxOutputDerivativeOrder;
    std::string info_cosim_canRunAsynchronuously;
    std::string info_cosim_canBeInstantiatedOnlyOncePerProcess;
    std::string info_cosim_canNotUseMemoryManagementFunctions;
    std::string info_cosim_canGetAndSetFMUstate;
    std::string info_cosim_canSerializeFMUstate;

    bool modex;
    std::string info_modex_modelIdentifier;
    std::string info_modex_needsExecutionTool;
    std::string info_modex_completedIntegratorStepNotNeeded;
    std::string info_modex_canBeInstantiatedOnlyOncePerProcess;
    std::string info_modex_canNotUseMemoryManagementFunctions;
    std::string info_modex_canGetAndSetFMUState;
    std::string info_modex_canSerializeFMUstate;
    std::string info_modex_providesDirectionalDerivative;

    std::map<std::string, FmuVariable> scalarVariables;

    FmuVariableTreeNode tree_variables;

  public:
    // private:
    fmi2CallbackFunctions callbacks;

    fmi2Component component;

    fmi2SetDebugLoggingTYPE* _fmi2SetDebugLogging;

    fmi2InstantiateTYPE* _fmi2Instantiate;
    fmi2FreeInstanceTYPE* _fmi2FreeInstance;
    fmi2GetVersionTYPE* _fmi2GetVersion;
    fmi2GetTypesPlatformTYPE* _fmi2GetTypesPlatform;

    fmi2SetupExperimentTYPE* _fmi2SetupExperiment;
    fmi2EnterInitializationModeTYPE* _fmi2EnterInitializationMode;
    fmi2ExitInitializationModeTYPE* _fmi2ExitInitializationMode;
    fmi2TerminateTYPE* _fmi2Terminate;
    fmi2ResetTYPE* _fmi2Reset;

    fmi2GetRealTYPE* _fmi2GetReal;
    fmi2GetIntegerTYPE* _fmi2GetInteger;
    fmi2GetBooleanTYPE* _fmi2GetBoolean;
    fmi2GetStringTYPE* _fmi2GetString;

    fmi2SetRealTYPE* _fmi2SetReal;
    fmi2SetIntegerTYPE* _fmi2SetInteger;
    fmi2SetBooleanTYPE* _fmi2SetBoolean;
    fmi2SetStringTYPE* _fmi2SetString;

    fmi2DoStepTYPE* _fmi2DoStep;

    fmi2SetTimeTYPE* _fmi2SetTime;
    fmi2SetContinuousStatesTYPE* _fmi2SetContinuousStates;
    fmi2GetDerivativesTYPE* _fmi2GetDerivatives;

  private:
    DYNLIB_HANDLE dynlib_handle;
};

// -----------------------------------------------------------------------------

FmuUnit::FmuUnit() : cosim(false), modex(false) {
    // default binaries directory in FMU unzipped directory
    binaries_dir = "/binaries/" + std::string(FMU_OS_SUFFIX);
}

void FmuUnit::LoadUnzipped(const std::string& directory) {
    this->directory = directory;

    LoadXML();
    LoadSharedLibrary();

    BuildVariablesTree();
}

void FmuUnit::Load(const std::string& filepath, const std::string& unzipdir) {
    std::cout << "Unzipping FMU: " << filepath << " in: " << unzipdir << std::endl;
    std::error_code ec;
    fs::remove_all(unzipdir, ec);
    fs::create_directories(unzipdir);
    miniz_cpp::zip_file fmufile(filepath);
    fmufile.extractall(unzipdir);

    LoadUnzipped(unzipdir);
}

void FmuUnit::LoadXML() {
    std::string xml_filename = directory + "/modelDescription.xml";

    rapidxml::xml_document<>* doc_ptr = new rapidxml::xml_document<>();

    // Read the xml file into a vector
    std::ifstream theFile(xml_filename);

    if (!theFile.good())
        throw std::runtime_error("Cannot find file: " + xml_filename + "\n");

    std::vector<char> buffer((std::istreambuf_iterator<char>(theFile)), std::istreambuf_iterator<char>());
    buffer.push_back('\0');
    // Parse the buffer using the xml file parsing library into doc
    doc_ptr->parse<0>(&buffer[0]);

    // Find the root node
    auto root_node = doc_ptr->first_node("fmiModelDescription");
    if (!root_node)
        throw std::runtime_error("Not a valid FMU. Missing <fmiModelDescription> in XML. \n");

    if (auto attr = root_node->first_attribute("modelName")) {
        modelName = attr->value();
    }
    if (auto attr = root_node->first_attribute("guid")) {
        guid = attr->value();
    }
    if (auto attr = root_node->first_attribute("fmiVersion")) {
        fmiVersion = attr->value();
    }
    if (auto attr = root_node->first_attribute("description")) {
        description = attr->value();
    }
    if (auto attr = root_node->first_attribute("generationTool")) {
        generationTool = attr->value();
    }
    if (auto attr = root_node->first_attribute("generationDateAndTime")) {
        generationDateAndTime = attr->value();
    }
    if (auto attr = root_node->first_attribute("variableNamingConvention")) {
        variableNamingConvention = attr->value();
    }
    if (auto attr = root_node->first_attribute("numberOfEventIndicators")) {
        numberOfEventIndicators = attr->value();
    }

    // Find the cosimulation node
    auto cosimulation_node = root_node->first_node("CoSimulation");
    if (cosimulation_node) {
        if (auto attr = cosimulation_node->first_attribute("modelIdentifier")) {
            info_cosim_modelIdentifier = attr->value();
        }
        if (auto attr = cosimulation_node->first_attribute("needsExecutionTool")) {
            info_cosim_needsExecutionTool = attr->value();
        }
        if (auto attr = cosimulation_node->first_attribute("canHandleVariableCommunicationStepSize")) {
            info_cosim_canHandleVariableCommunicationStepSize = attr->value();
        }
        if (auto attr = cosimulation_node->first_attribute("canInterpolateInputs")) {
            info_cosim_canInterpolateInputs = attr->value();
        }
        if (auto attr = cosimulation_node->first_attribute("maxOutputDerivativeOrder")) {
            info_cosim_maxOutputDerivativeOrder = attr->value();
        }
        if (auto attr = cosimulation_node->first_attribute("canRunAsynchronuously")) {
            info_cosim_canRunAsynchronuously = attr->value();
        }
        if (auto attr = cosimulation_node->first_attribute("canBeInstantiatedOnlyOncePerProcess")) {
            info_cosim_canBeInstantiatedOnlyOncePerProcess = attr->value();
        }
        if (auto attr = cosimulation_node->first_attribute("canNotUseMemoryManagementFunctions")) {
            info_cosim_canNotUseMemoryManagementFunctions = attr->value();
        }
        if (auto attr = cosimulation_node->first_attribute("canGetAndSetFMUstate")) {
            info_cosim_canGetAndSetFMUstate = attr->value();
        }
        if (auto attr = cosimulation_node->first_attribute("canSerializeFMUstate")) {
            info_cosim_canSerializeFMUstate = attr->value();
        }
        cosim = true;
    }

    // Find the model exchange node
    auto modelexchange_node = root_node->first_node("ModelExchange");
    if (modelexchange_node) {
        if (auto attr = modelexchange_node->first_attribute("modelIdentifier")) {
            info_modex_modelIdentifier = attr->value();
        }
        if (auto attr = modelexchange_node->first_attribute("needsExecutionTool")) {
            info_modex_needsExecutionTool = attr->value();
        }
        if (auto attr = modelexchange_node->first_attribute("completedIntegratorStepNotNeeded")) {
            info_modex_completedIntegratorStepNotNeeded = attr->value();
        }

        if (auto attr = modelexchange_node->first_attribute("canBeInstantiatedOnlyOncePerProcess")) {
            info_modex_canBeInstantiatedOnlyOncePerProcess = attr->value();
        }
        if (auto attr = modelexchange_node->first_attribute("canNotUseMemoryManagementFunctions")) {
            info_modex_canNotUseMemoryManagementFunctions = attr->value();
        }
        if (auto attr = modelexchange_node->first_attribute("canGetAndSetFMUState")) {
            info_modex_canGetAndSetFMUState = attr->value();
        }
        if (auto attr = modelexchange_node->first_attribute("canSerializeFMUstate")) {
            info_modex_canSerializeFMUstate = attr->value();
        }
        if (auto attr = modelexchange_node->first_attribute("providesDirectionalDerivative")) {
            info_modex_providesDirectionalDerivative = attr->value();
        }
        modex = true;
    }

    if (!cosim && !modex) {
        throw std::runtime_error("Not a valid FMU. Missing both <CoSimulation> and <ModelExchange> in XML. \n");
    }

    // Find the variable container node
    auto variables_node = root_node->first_node("ModelVariables");
    if (!variables_node)
        throw std::runtime_error("Not a valid FMU. Missing <ModelVariables> in XML. \n");

    // Iterate over the variables (valueReference starts at 1)
    for (auto variable_node = variables_node->first_node("ScalarVariable"); variable_node;
         variable_node = variable_node->next_sibling()) {
        FmuVariable::Type mvar_type;
        std::string mvar_name;

        // fetch properties
        if (auto attr = variable_node->first_attribute("name"))
            mvar_name = attr->value();
        else
            throw std::runtime_error("Cannot find 'name' property in variable.\n");

        if (variable_node->first_node("Real"))
            mvar_type = FmuVariable::Type::Real;
        else if (variable_node->first_node("String"))
            mvar_type = FmuVariable::Type::String;
        else if (variable_node->first_node("Integer"))
            mvar_type = FmuVariable::Type::Integer;
        else if (variable_node->first_node("Boolean"))
            mvar_type = FmuVariable::Type::Boolean;
        else
            mvar_type = FmuVariable::Type::Real;

        fmi2ValueReference valref;
        std::string description = "";
        std::string variability = "";
        std::string causality = "";
        std::string initial = "";

        if (auto attr = variable_node->first_attribute("valueReference"))
            valref = std::stoul(attr->value());
        else
            throw std::runtime_error("Cannot find 'valueReference' property in variable.\n");

        if (auto attr = variable_node->first_attribute("description"))
            description = attr->value();

        if (auto attr = variable_node->first_attribute("variability"))
            variability = attr->value();

        if (auto attr = variable_node->first_attribute("causality"))
            causality = attr->value();

        if (auto attr = variable_node->first_attribute("initial"))
            initial = attr->value();

        FmuVariable::CausalityType causality_enum;
        FmuVariable::VariabilityType variability_enum;
        FmuVariable::InitialType initial_enum;

        if (causality.empty())
            causality_enum = FmuVariable::CausalityType::local;
        else if (!causality.compare("parameter"))
            causality_enum = FmuVariable::CausalityType::parameter;
        else if (!causality.compare("calculatedParameter"))
            causality_enum = FmuVariable::CausalityType::calculatedParameter;
        else if (!causality.compare("input"))
            causality_enum = FmuVariable::CausalityType::input;
        else if (!causality.compare("output"))
            causality_enum = FmuVariable::CausalityType::output;
        else if (!causality.compare("local"))
            causality_enum = FmuVariable::CausalityType::local;
        else if (!causality.compare("independent"))
            causality_enum = FmuVariable::CausalityType::independent;
        else
            throw std::runtime_error("causality is badly formatted.");

        if (variability.empty())
            variability_enum = FmuVariable::VariabilityType::continuous;
        else if (!variability.compare("constant"))
            variability_enum = FmuVariable::VariabilityType::constant;
        else if (!variability.compare("fixed"))
            variability_enum = FmuVariable::VariabilityType::fixed;
        else if (!variability.compare("tunable"))
            variability_enum = FmuVariable::VariabilityType::tunable;
        else if (!variability.compare("discrete"))
            variability_enum = FmuVariable::VariabilityType::discrete;
        else if (!variability.compare("continuous"))
            variability_enum = FmuVariable::VariabilityType::continuous;
        else
            throw std::runtime_error("variability is badly formatted.");

        if (initial.empty())
            initial_enum = FmuVariable::InitialType::none;
        else if (!initial.compare("exact"))
            initial_enum = FmuVariable::InitialType::exact;
        else if (!initial.compare("approx"))
            initial_enum = FmuVariable::InitialType::approx;
        else if (!initial.compare("calculated"))
            initial_enum = FmuVariable::InitialType::calculated;
        else
            throw std::runtime_error("variability is badly formatted.");

        FmuVariable mvar(mvar_name, mvar_type, causality_enum, variability_enum, initial_enum);
        mvar.SetValueReference(valref);

        scalarVariables[mvar.GetName()] = mvar;
    }

    delete doc_ptr;
}

void FmuUnit::LoadSharedLibrary() {
    std::string dynlib_dir = directory + "/" + binaries_dir;
    std::string dynlib_name = dynlib_dir + "/" + info_cosim_modelIdentifier + std::string(SHARED_LIBRARY_SUFFIX);

    dynlib_handle = RuntimeLinkLibrary(dynlib_dir, dynlib_name);

    if (!dynlib_handle)
        throw std::runtime_error("Could not locate the compiled FMU files: " + dynlib_name + "\n");

    // run time binding of functions
    LOAD_FMI_FUNCTION(fmi2SetDebugLogging);
    LOAD_FMI_FUNCTION(fmi2Instantiate);
    LOAD_FMI_FUNCTION(fmi2FreeInstance);
    LOAD_FMI_FUNCTION(fmi2GetVersion);
    LOAD_FMI_FUNCTION(fmi2GetTypesPlatform);
    LOAD_FMI_FUNCTION(fmi2SetupExperiment);
    LOAD_FMI_FUNCTION(fmi2EnterInitializationMode);
    LOAD_FMI_FUNCTION(fmi2ExitInitializationMode);
    LOAD_FMI_FUNCTION(fmi2Terminate);
    LOAD_FMI_FUNCTION(fmi2Reset);
    LOAD_FMI_FUNCTION(fmi2GetReal);
    LOAD_FMI_FUNCTION(fmi2GetInteger);
    LOAD_FMI_FUNCTION(fmi2GetBoolean);
    LOAD_FMI_FUNCTION(fmi2GetString);
    LOAD_FMI_FUNCTION(fmi2SetReal);
    LOAD_FMI_FUNCTION(fmi2SetInteger);
    LOAD_FMI_FUNCTION(fmi2SetBoolean);
    LOAD_FMI_FUNCTION(fmi2SetString);
    if (cosim) {
        LOAD_FMI_FUNCTION(fmi2DoStep);
    }
    if (modex) {
        LOAD_FMI_FUNCTION(fmi2SetTime);
        LOAD_FMI_FUNCTION(fmi2SetContinuousStates);
        LOAD_FMI_FUNCTION(fmi2GetDerivatives);
    }
}

void FmuUnit::BuildVariablesTree() {
    for (auto& iv : this->scalarVariables) {
        std::string token;
        std::istringstream ss(iv.second.GetName());

        // scan all tokens delimited by "."
        int ntokens = 0;
        FmuVariableTreeNode* tree_node = &this->tree_variables;
        while (std::getline(ss, token, '.') && ntokens < 300) {
            auto next_node = tree_node->children.find(token);
            if (next_node != tree_node->children.end())  // already added
            {
                tree_node = &(next_node->second);
            } else {
                FmuVariableTreeNode new_node;
                new_node.object_name = token;
                tree_node->children[token] = new_node;
                tree_node = &tree_node->children[token];
            }
            ++ntokens;
        }
        tree_node->leaf = &iv.second;
    }
}

void FmuUnit::DumpTree(FmuVariableTreeNode* mynode, int tab) {
    for (auto& in : mynode->children) {
        for (int itab = 0; itab < tab; ++itab) {
            std::cout << "\t";
        }
        // name of tree level
        std::cout << in.first;
        // level is a FMU variable (tree leaf)
        if (in.second.leaf) {
            std::cout << " -> FMU reference:" << in.second.leaf->GetValueReference();
        }

        std::cout << "\n";
        DumpTree(&in.second, tab + 1);
    }
}

std::string FmuUnit::GetVersion() const {
    auto version = _fmi2GetVersion();
    return std::string(version);
}

std::string FmuUnit::GetTypesPlatform() const {
    auto platform = _fmi2GetTypesPlatform();
    return std::string(platform);
}

void FmuUnit::Instantiate(std::string tool_name, std::string resource_dir, bool logging, bool visible) {
    callbacks.logger = LoggingUtilities::logger_default;
    callbacks.allocateMemory = calloc;
    callbacks.freeMemory = free;
    callbacks.stepFinished = NULL;
    callbacks.componentEnvironment = NULL;

    // Instantiate both slaves
    component = _fmi2Instantiate(tool_name.c_str(),                         // instance name
                                 fmi2CoSimulation,                          // type
                                 guid.c_str(),                              // guid
                                 resource_dir.c_str(),                      // resource dir
                                 (const fmi2CallbackFunctions*)&callbacks,  // function callbacks
                                 visible ? fmi2True : fmi2False,            // visible
                                 logging);                                  // logging

    if (!component)
        throw std::runtime_error("Failed to instantiate the FMU.");
}

void FmuUnit::Instantiate(std::string tool_name, bool logging, bool visible) {
    Instantiate(tool_name, "file:///" + directory + "/resources", logging, visible);
}

fmi2Status FmuUnit::SetDebugLogging(fmi2Boolean loggingOn, const std::vector<std::string>& logCategories) {
    std::vector<const char*> categories;
    for (const auto& category : logCategories) {
        categories.push_back(category.c_str());
    }
    return _fmi2SetDebugLogging(this->component, loggingOn, categories.size(), categories.data());
}

fmi2Status FmuUnit::SetupExperiment(fmi2Boolean toleranceDefined,
                                    fmi2Real tolerance,
                                    fmi2Real startTime,
                                    fmi2Boolean stopTimeDefined,
                                    fmi2Real stopTime) {
    auto status = _fmi2SetupExperiment(this->component,              //
                                       toleranceDefined, tolerance,  // define tolerance
                                       startTime,                    // start time
                                       stopTimeDefined, stopTime);   // use stop time
    return status;
}

fmi2Status FmuUnit::EnterInitializationMode() {
    return _fmi2EnterInitializationMode(this->component);
}
fmi2Status FmuUnit::ExitInitializationMode() {
    return _fmi2ExitInitializationMode(this->component);
}

fmi2Status FmuUnit::DoStep(fmi2Real currentCommunicationPoint,
                           fmi2Real communicationStepSize,
                           fmi2Boolean noSetFMUStatePriorToCurrentPoint) {
    if (!cosim)
        throw std::runtime_error("DoStep available only for a CoSimulation FMU. \n");

    auto status = _fmi2DoStep(this->component,                                   //
                              currentCommunicationPoint, communicationStepSize,  //
                              noSetFMUStatePriorToCurrentPoint);                 //
    return status;
}

fmi2Status FmuUnit::SetTime(const fmi2Real time) {
    if (!modex)
        throw std::runtime_error("SetTime available only for a ModelExchange FMU. \n");

    auto status = _fmi2SetTime(this->component, time);

    return status;
}

fmi2Status FmuUnit::SetContinuousStates(const fmi2Real x[], size_t nx) {
    if (!modex)
        throw std::runtime_error("SetContinuousStates available only for a ModelExchange FMU. \n");

    auto status = _fmi2SetContinuousStates(this->component, x, nx);

    return status;
}

fmi2Status FmuUnit::GetDerivatives(fmi2Real derivatives[], size_t nx) {
    if (!modex)
        throw std::runtime_error("GetDerivatives available only for a ModelExchange FMU. \n");

    auto status = _fmi2GetDerivatives(this->component, derivatives, nx);

    return status;
}

template <class T>
fmi2Status FmuUnit::GetVariable(fmi2ValueReference vr, T& value, FmuVariable::Type vartype) noexcept(false) {
    fmi2Status status = fmi2Status::fmi2Error;

    switch (vartype) {
        case FmuVariable::Type::Real:
            status = this->_fmi2GetReal(this->component, &vr, 1, (fmi2Real*)&value);
            break;
        case FmuVariable::Type::Integer:
            status = this->_fmi2GetInteger(this->component, &vr, 1, (fmi2Integer*)&value);
            break;
        case FmuVariable::Type::Boolean:
            status = this->_fmi2GetBoolean(this->component, &vr, 1, (fmi2Boolean*)&value);
            break;
        case FmuVariable::Type::String:
            status = this->_fmi2GetString(this->component, &vr, 1, (fmi2String*)&value);
            break;
        case FmuVariable::Type::Unknown:
            throw std::runtime_error("Fmu Variable type not initialized.");
            break;
        default:
            throw std::runtime_error("Fmu Variable type not valid.");
            break;
    }

    return status;
}

template <class T>
fmi2Status FmuUnit::SetVariable(fmi2ValueReference vr, const T& value, FmuVariable::Type vartype) noexcept(false) {
    fmi2Status status = fmi2Status::fmi2Error;

    switch (vartype) {
        case FmuVariable::Type::Real:
            status = this->_fmi2SetReal(this->component, &vr, 1, (fmi2Real*)&value);
            break;
        case FmuVariable::Type::Integer:
            status = this->_fmi2SetInteger(this->component, &vr, 1, (fmi2Integer*)&value);
            break;
        case FmuVariable::Type::Boolean:
            status = this->_fmi2SetBoolean(this->component, &vr, 1, (fmi2Boolean*)&value);
            break;
        case FmuVariable::Type::String:
            status = this->_fmi2SetString(this->component, &vr, 1, (fmi2String*)&value);
            break;
        case FmuVariable::Type::Unknown:
            throw std::runtime_error("Fmu Variable type not initialized.");
            break;
        default:
            throw std::runtime_error("Fmu Variable type not valid.");
            break;
    }

    return status;
}

template <class T>
fmi2Status FmuUnit::GetVariable(const std::string& varname, T& value, FmuVariable::Type vartype) noexcept(false) {
    return GetVariable(scalarVariables.at(varname).GetValueReference(), value, vartype);
}

template <class T>
fmi2Status FmuUnit::SetVariable(const std::string& varname, const T& value, FmuVariable::Type vartype) noexcept(false) {
    return SetVariable(scalarVariables.at(varname).GetValueReference(), value, vartype);
}

fmi2Status FmuUnit::GetVariable(const std::string& varname, std::string& value) noexcept(false) {
    fmi2String tmp;
    auto status = GetVariable(varname, tmp, FmuVariable::Type::String);
    value = std::string(tmp);
    return status;
}

fmi2Status FmuUnit::SetVariable(const std::string& varname, const std::string& value) noexcept(false) {
    return SetVariable(varname, value.c_str(), FmuVariable::Type::String);
}

fmi2Status FmuUnit::GetVariable(const std::string& varname, bool& value) noexcept(false) {
    fmi2Boolean tmp;
    auto status = GetVariable(varname, tmp, FmuVariable::Type::Boolean);
    value = (tmp != 0) ? true : false;
    return status;
}

fmi2Status FmuUnit::SetVariable(const std::string& varname, const bool& value) noexcept(false) {
    return SetVariable(varname, value ? 1 : 0, FmuVariable::Type::Boolean);
}
