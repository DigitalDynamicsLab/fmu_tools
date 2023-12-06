#pragma once
#include "FmuToolsCommon.h"
#include "FmuToolsRuntimeLinking.h"

#include "rapidxml/rapidxml.hpp"
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <cstdarg>
#include <iostream>
#include <system_error>


#include "miniz-cpp/zip_file.hpp"

#include "filesystem.hpp"

#define LOAD_FMI_FUNCTION(funcName) \
    this->_##funcName = ( funcName##TYPE* ) get_function_ptr( this->dynlib_handle, #funcName ); \
    if (!this->_##funcName) \
        throw std::runtime_error(std::string(std::string("Could not find ") + std::string(#funcName) + std::string(" in the FMU library. Wrong or outdated FMU?")));


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


void logger_default(fmi2ComponentEnvironment c, fmi2String instanceName, fmi2Status status, fmi2String category, fmi2String message, ...)
{
    char msg[2024];
    va_list argp;
    va_start(argp, message);
    vsprintf(msg, message, argp);
    if (!instanceName) instanceName = "?";
    if (!category) category = "?";
    printf("[%s|%s] %s: %s", instanceName, fmi2Status_toString(status).c_str(), category, msg);
}

static fmi2CallbackFunctions callbackFunctions_default = {
    logger_default,
    calloc,
    free,
    nullptr,
    nullptr
};




/////////////////////////////////////////////////////////////////////////////////////////////////

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
/// 

class FmuVariableTreeNode {
public:
    std::string object_name;

    std::map<std::string, FmuVariableTreeNode> children;
    FmuVariable* leaf = nullptr;

    FmuVariableTreeNode() {}
};


/////////////////////////////////////////////////////////////////////////////////////////////////

/// Class holding a set of scalar variables (3xposition, 9xrotation) for the coordinate
/// system of a visualizer in the FMU. 
/// The visualizer could be a cylinder, a sphere, a mesh, etc.

struct FmuVisualShape {
    FmuVariableTreeNode* visualizer_node = nullptr;

    unsigned int pos_references[3] = {0,0,0};
    unsigned int rot_references[9] = {0,0,0};
    unsigned int pos_shape_references[3] = {0,0,0};
    unsigned int shapetype_reference = 0;
    unsigned int l_references[3] = {0,0,0};
    unsigned int w_references[3] = {0,0,0};
    unsigned int color_references[3] = {0,0,0};
    unsigned int width_reference = 0;
    unsigned int height_reference = 0;
    unsigned int length_reference = 0;

    std::string type;
    std::string filename;
};


/////////////////////////////////////////////////////////////////////////////////////////////////

/// Class holding a set of scalar variables (3xposition, 9xrotation) for the coordinate
/// system of a visualizer in the FMU. 

struct FmuBody {

    unsigned int pos_references[3];
    unsigned int rot_references[9];

    std::string name;
};


/////////////////////////////////////////////////////////////////////////////////////////////////

/// Class for managing a FMU.
/// It contains functions to load the shared library in run-time, to parse the XML,
/// to set/get variables, etc.

class FmuUnit {

public:
    /// Construction
    FmuUnit() {
        // default binaries directory in FMU unzipped directory
        binaries_dir = "/binaries/" + std::string(FMU_OS_SUFFIX);
    };

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
    std::string info_cosimulation_modelIdentifier;
    std::string info_cosimulation_needsExecutionTool;
    std::string info_cosimulation_canHandleVariableCommunicationStepSize;
    std::string info_cosimulation_canInterpolateInputs;
    std::string info_cosimulation_maxOutputDerivativeOrder;
    std::string info_cosimulation_canRunAsynchronuously;
    std::string info_cosimulation_canBeInstantiatedOnlyOncePerProcess;
    std::string info_cosimulation_canNotUseMemoryManagementFunctions;
    std::string info_cosimulation_canGetAndSetFMUstate;
    std::string info_cosimulation_canSerializeFMUstate;

    std::map<std::string, FmuVariable> scalarVariables;

    FmuVariableTreeNode tree_variables;

    std::vector<FmuVisualShape> visualizers;
    std::vector<FmuBody> bodies;


    // wrappers for runtime library linking:
private:
    DYNLIB_HANDLE dynlib_handle;
public:
    //private:
    fmi2CallbackFunctions       callbacks;

    fmi2Component                       component;

    fmi2SetDebugLoggingTYPE*            _fmi2SetDebugLogging;

    fmi2InstantiateTYPE*                _fmi2Instantiate;
    fmi2FreeInstanceTYPE*               _fmi2FreeInstance;
    fmi2GetVersionTYPE*                 _fmi2GetVersion;
    fmi2GetTypesPlatformTYPE*           _fmi2GetTypesPlatform;

    fmi2SetupExperimentTYPE*            _fmi2SetupExperiment;
    fmi2EnterInitializationModeTYPE*    _fmi2EnterInitializationMode;
    fmi2ExitInitializationModeTYPE*     _fmi2ExitInitializationMode;
    fmi2TerminateTYPE*                  _fmi2Terminate;
    fmi2ResetTYPE*                      _fmi2Reset;

    fmi2GetRealTYPE*                    _fmi2GetReal;
    fmi2GetIntegerTYPE*                 _fmi2GetInteger;
    fmi2GetBooleanTYPE*                 _fmi2GetBoolean;
    fmi2GetStringTYPE*                  _fmi2GetString;

    fmi2SetRealTYPE*                    _fmi2SetReal;
    fmi2SetIntegerTYPE*                 _fmi2SetInteger;
    fmi2SetBooleanTYPE*                 _fmi2SetBoolean;
    fmi2SetStringTYPE*                  _fmi2SetString;

    fmi2DoStepTYPE*                     _fmi2DoStep;

public:
    //private:
    /// Load the FMU from the directory, assuming it has been unzipped
    void LoadUnzipped(const std::string& mdirectory) {
        this->directory = mdirectory;

    }

    void Load(const std::string& filepath, const std::string& unzipdir = fs::temp_directory_path().generic_string() + std::string("/_fmu_temp")) {
        
        std::cout << "Unzipping FMU: " << filepath << " in: " << unzipdir << std::endl;
        std::error_code ec;
        fs::remove_all(unzipdir, ec);
        fs::create_directories(unzipdir);
        miniz_cpp::zip_file fmufile(filepath);
        fmufile.extractall(unzipdir);
        this->directory = unzipdir;

        LoadXML();
        LoadSharedLibrary();
    }


    /// Parse XML and create the list of variables.
    /// If something fails, throws an exception.
    void LoadXML() {

        std::string xml_filename = this->directory + "/modelDescription.xml";

        rapidxml::xml_document<>* doc_ptr = new rapidxml::xml_document<>();

        // Read the xml file into a vector
        std::ifstream theFile(xml_filename);

        if (!theFile.is_open())
            throw std::runtime_error("Cannot find file: " + xml_filename + "\n");

        std::vector<char> buffer((std::istreambuf_iterator<char>(theFile)), std::istreambuf_iterator<char>());
        buffer.push_back('\0');
        // Parse the buffer using the xml file parsing library into doc 
        doc_ptr->parse<0>(&buffer[0]);

        // Find our root node
        auto root_node = doc_ptr->first_node("fmiModelDescription");
        if (!root_node)
            throw std::runtime_error("Not a valid FMU. Missing <fmiModelDescription> in XML. \n");

        if (auto attr = root_node->first_attribute("modelName")) {
            this->modelName = attr->value();
        }
        if (auto attr = root_node->first_attribute("guid")) {
            this->guid = attr->value();
        }
        if (auto attr = root_node->first_attribute("fmiVersion")) {
            this->fmiVersion = attr->value();
        }
        if (auto attr = root_node->first_attribute("description")) {
            this->description = attr->value();
        }
        if (auto attr = root_node->first_attribute("generationTool")) {
            this->generationTool = attr->value();
        }
        if (auto attr = root_node->first_attribute("generationDateAndTime")) {
            this->generationDateAndTime = attr->value();
        }
        if (auto attr = root_node->first_attribute("variableNamingConvention")) {
            this->variableNamingConvention = attr->value();
        }
        if (auto attr = root_node->first_attribute("numberOfEventIndicators")) {
            this->numberOfEventIndicators = attr->value();
        }

        // Find our cosimulation node
        auto cosimulation_node = root_node->first_node("CoSimulation");
        if (cosimulation_node) {
            if (auto attr = cosimulation_node->first_attribute("modelIdentifier")) {
                this->info_cosimulation_modelIdentifier = attr->value();
            }
            if (auto attr = cosimulation_node->first_attribute("needsExecutionTool")) {
                this->info_cosimulation_needsExecutionTool = attr->value();
            }
            if (auto attr = cosimulation_node->first_attribute("canHandleVariableCommunicationStepSize")) {
                this->info_cosimulation_canHandleVariableCommunicationStepSize = attr->value();
            }
            if (auto attr = cosimulation_node->first_attribute("canInterpolateInputs")) {
                this->info_cosimulation_canInterpolateInputs = attr->value();
            }
            if (auto attr = cosimulation_node->first_attribute("maxOutputDerivativeOrder")) {
                this->info_cosimulation_maxOutputDerivativeOrder = attr->value();
            }
            if (auto attr = cosimulation_node->first_attribute("canRunAsynchronuously")) {
                this->info_cosimulation_canRunAsynchronuously = attr->value();
            }
            if (auto attr = cosimulation_node->first_attribute("canBeInstantiatedOnlyOncePerProcess")) {
                this->info_cosimulation_canBeInstantiatedOnlyOncePerProcess = attr->value();
            }
            if (auto attr = cosimulation_node->first_attribute("canNotUseMemoryManagementFunctions")) {
                this->info_cosimulation_canNotUseMemoryManagementFunctions = attr->value();
            }
            if (auto attr = cosimulation_node->first_attribute("canGetAndSetFMUstate")) {
                this->info_cosimulation_canGetAndSetFMUstate = attr->value();
            }
            if (auto attr = cosimulation_node->first_attribute("canSerializeFMUstate")) {
                this->info_cosimulation_canSerializeFMUstate = attr->value();
            }
        }

        // Find our variable container node
        auto variables_node = root_node->first_node("ModelVariables");
        if (!variables_node)
            throw std::runtime_error("Not a valid FMU. Missing <ModelVariables> in XML. \n");

        // Iterate over the variables (valueReference starts at 1)
        for (auto variable_node = variables_node->first_node("ScalarVariable"); variable_node; variable_node = variable_node->next_sibling())
        {
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

    /// Load the shared library in run-time and do the dynamic linking to the needed FMU functions.
    void LoadSharedLibrary() {

        std::string dynlib_dir = this->directory + this->binaries_dir;
        std::string dynlib_name = dynlib_dir + "/" + this->info_cosimulation_modelIdentifier + std::string(SHARED_LIBRARY_SUFFIX);

        this->dynlib_handle = RuntimeLinkLibrary(dynlib_dir, dynlib_name);


        if (!this->dynlib_handle)
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
        LOAD_FMI_FUNCTION(fmi2DoStep);

    }

    /// From the flat variable list build a tree of variables
    void BuildVariablesTree() {
        for (auto& iv : this->scalarVariables) {
            std::string token;
            std::istringstream ss(iv.second.GetName());

            // scan all tokens delimited by "."
            int ntokens = 0;
            FmuVariableTreeNode* tree_node = &this->tree_variables;
            while (std::getline(ss, token, '.') && ntokens < 300)
            {
                auto next_node = tree_node->children.find(token);
                if (next_node != tree_node->children.end()) // already added
                {
                    tree_node = &(next_node->second);
                }
                else
                {
                    FmuVariableTreeNode new_node;
                    new_node.object_name = token;
                    tree_node->children[token] = new_node;
                    tree_node = &tree_node->children[token];
                }
                ++ntokens;
            }
            //GetLog() << "Leaf! " << iv.second.name << " tree node name:" << token << "ref=" << iv.second.valueReference <<"\n";
            tree_node->leaf = &iv.second;
        }
    }

    /// Dump the tree of variables (recursive)
    void DumpTree(FmuVariableTreeNode* mynode, int tab) {
        for (auto& in : mynode->children) {
            for (int itab = 0; itab<tab; ++itab) {
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

    void BuildBodyList(FmuVariableTreeNode* mynode) {

    }

    void BuildVisualizersList(FmuVariableTreeNode* mynode) {
        auto found_shtype = mynode->children.find("shapeType");
        auto found_R = mynode->children.find("R");
        auto found_r1 = mynode->children.find("r[1]");
        auto found_r2 = mynode->children.find("r[2]");
        auto found_r3 = mynode->children.find("r[3]");
        auto found_rshape1 = mynode->children.find("r_shape[1]");
        auto found_rshape2 = mynode->children.find("r_shape[2]");
        auto found_rshape3 = mynode->children.find("r_shape[3]");
        auto found_l1 = mynode->children.find("lengthDirection[1]");
        auto found_l2 = mynode->children.find("lengthDirection[2]");
        auto found_l3 = mynode->children.find("lengthDirection[3]");
        auto found_w1 = mynode->children.find("widthDirection[1]");
        auto found_w2 = mynode->children.find("widthDirection[2]");
        auto found_w3 = mynode->children.find("widthDirection[3]");
        auto found_color1 = mynode->children.find("color[1]");
        auto found_color2 = mynode->children.find("color[2]");
        auto found_color3 = mynode->children.find("color[3]");
        auto found_length = mynode->children.find("length");
        auto found_width = mynode->children.find("width");
        auto found_height = mynode->children.find("height");
        if (found_shtype != mynode->children.end() &&
            found_R != mynode->children.end() &&
            found_r1 != mynode->children.end() &&
            found_r2 != mynode->children.end() &&
            found_r3 != mynode->children.end() &&
            found_l1 != mynode->children.end() &&
            found_l2 != mynode->children.end() &&
            found_l3 != mynode->children.end() &&
            found_w1 != mynode->children.end() &&
            found_w2 != mynode->children.end() &&
            found_w3 != mynode->children.end()
            ) {
            auto found_T11 = found_R->second.children.find("T[1,1]");
            auto found_T12 = found_R->second.children.find("T[1,2]");
            auto found_T13 = found_R->second.children.find("T[1,3]");
            auto found_T21 = found_R->second.children.find("T[2,1]");
            auto found_T22 = found_R->second.children.find("T[2,2]");
            auto found_T23 = found_R->second.children.find("T[2,3]");
            auto found_T31 = found_R->second.children.find("T[3,1]");
            auto found_T32 = found_R->second.children.find("T[3,2]");
            auto found_T33 = found_R->second.children.find("T[3,3]");
            if (found_T11 != found_R->second.children.end() &&
                found_T12 != found_R->second.children.end() &&
                found_T13 != found_R->second.children.end() &&
                found_T21 != found_R->second.children.end() &&
                found_T22 != found_R->second.children.end() &&
                found_T23 != found_R->second.children.end() &&
                found_T31 != found_R->second.children.end() &&
                found_T32 != found_R->second.children.end() &&
                found_T33 != found_R->second.children.end()) {

                FmuVisualShape my_v;
                my_v.pos_references[0] = found_r1->second.leaf->GetValueReference();
                my_v.pos_references[1] = found_r2->second.leaf->GetValueReference();
                my_v.pos_references[2] = found_r3->second.leaf->GetValueReference();
                my_v.pos_shape_references[0] = found_rshape1->second.leaf->GetValueReference();
                my_v.pos_shape_references[1] = found_rshape2->second.leaf->GetValueReference();
                my_v.pos_shape_references[2] = found_rshape3->second.leaf->GetValueReference();
                my_v.rot_references[0] = found_T11->second.leaf->GetValueReference();
                my_v.rot_references[1] = found_T12->second.leaf->GetValueReference();
                my_v.rot_references[2] = found_T13->second.leaf->GetValueReference();
                my_v.rot_references[3] = found_T21->second.leaf->GetValueReference();
                my_v.rot_references[4] = found_T22->second.leaf->GetValueReference();
                my_v.rot_references[5] = found_T23->second.leaf->GetValueReference();
                my_v.rot_references[6] = found_T31->second.leaf->GetValueReference();
                my_v.rot_references[7] = found_T32->second.leaf->GetValueReference();
                my_v.rot_references[8] = found_T33->second.leaf->GetValueReference();
                my_v.shapetype_reference = found_shtype->second.leaf->GetValueReference();
                my_v.l_references[0] = found_l1->second.leaf->GetValueReference();
                my_v.l_references[1] = found_l2->second.leaf->GetValueReference();
                my_v.l_references[2] = found_l3->second.leaf->GetValueReference();
                my_v.w_references[0] = found_w1->second.leaf->GetValueReference();
                my_v.w_references[1] = found_w2->second.leaf->GetValueReference();
                my_v.w_references[2] = found_w3->second.leaf->GetValueReference();
                my_v.color_references[0] = found_color1->second.leaf->GetValueReference();
                my_v.color_references[1] = found_color2->second.leaf->GetValueReference();
                my_v.color_references[2] = found_color3->second.leaf->GetValueReference();
                my_v.width_reference = found_width->second.leaf->GetValueReference();
                my_v.length_reference = found_length->second.leaf->GetValueReference();
                my_v.height_reference = found_height->second.leaf->GetValueReference();
                my_v.visualizer_node = mynode;
                this->visualizers.push_back(my_v);
            }
        }
        // recursion
        for (auto& in : mynode->children) {
            BuildVisualizersList(&in.second);
        }
    }



    /// Instantiate the model: simplified function wrapping fmi2Instantiate
    void Instantiate(std::string tool_name = std::string("tool1"),
        std::string resource_dir = std::string("file:///C:/temp"),
        bool logging = false) {

        this->callbacks.logger = logger_default;
        this->callbacks.allocateMemory = calloc;
        this->callbacks.freeMemory = free;
        this->callbacks.stepFinished = NULL;
        this->callbacks.componentEnvironment = NULL;

        //Instantiate both slaves 
        this->component = _fmi2Instantiate(
            tool_name.c_str(),  // instance name
            fmi2CoSimulation,   // type
            this->guid.c_str(), // guid
            resource_dir.c_str(), // resource dir
            (const fmi2CallbackFunctions*)&callbacks,  // function callbacks    
            fmi2False,          // visible
            logging);           // logging 
        if (!this->component)
            throw std::runtime_error("Failed to instantiate the FMU.");
    }

    template <class T>
    fmi2Status GetVariable(fmi2ValueReference vr, T& value, FmuVariable::Type vartype) noexcept(false) {
      fmi2Status status = fmi2Status::fmi2Error;

      switch (vartype)
      {
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
    fmi2Status SetVariable(fmi2ValueReference vr, const T& value, FmuVariable::Type vartype) noexcept(false) {
      fmi2Status status = fmi2Status::fmi2Error;

      switch (vartype)
      {
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
    fmi2Status GetVariable(const std::string& varname, T& value, FmuVariable::Type vartype) noexcept(false) {
        return GetVariable(scalarVariables.at(varname).GetValueReference(), value, vartype);
    }

    template <class T>
    fmi2Status SetVariable(const std::string& varname, const T& value, FmuVariable::Type vartype) noexcept(false) {
        return SetVariable(scalarVariables.at(varname).GetValueReference(), value, vartype);
    }

};
