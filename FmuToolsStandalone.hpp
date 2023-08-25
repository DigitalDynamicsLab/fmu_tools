#pragma once
#include "FmuToolsCommon.h"
#include <string>
#include "rapidxml/rapidxml.hpp"
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <windows.h>
#include <libloaderapi.h>
#include <winbase.h>
#include <cstdarg>
#include <iostream>


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
    //char msg[2024];
    //va_list argp;
    //va_start(argp, message);
    //vsprintf(msg, message, argp);
    //if (!instanceName) instanceName = "?";
    //if (!category) category = "?";
    //printf("fmiStatus = %d;  %s (%s): %s\n", status, instanceName, category, msg);

    if (!instanceName) instanceName = "?";
    if (!category) category = "?";
    std::cout << "[" << instanceName << "|" << fmi2Status_toString(status) << "] " << message;
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
    FmuScalarVariable* leaf = nullptr;

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
/// It contains functions to load the DLL in run-time, to parse the XML,
/// to set/get variables, etc.

class FmuUnit {

public:
    /// Construction
    FmuUnit() {
        // default binaries directory in FMU unzipped directory
        binaries_dir = "/binaries/win64";
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

    std::map<std::string, FmuScalarVariable> flat_variables;

    FmuVariableTreeNode tree_variables;

    std::vector<FmuVisualShape> visualizers;
    std::vector<FmuBody> bodies;


    // wrappers for runtime dll linking:
private:
    HINSTANCE hGetProcIDDLL;
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
    void Load(const std::string& mdirectory) {
        this->directory = mdirectory;

    }


    /// Parse XML and create the list of variables.
    /// If something fails, throws and exception.
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

        // Iterate over the variables
        for (auto variable_node = variables_node->first_node("ScalarVariable"); variable_node; variable_node = variable_node->next_sibling())
        {
            FmuScalarVariable mvar;

            // fetch properties

            if (auto attr = variable_node->first_attribute("name"))
                mvar.name = attr->value();
            else
                throw std::runtime_error("cannot find 'name' property in variable \n");

            if (auto attr = variable_node->first_attribute("valueReference"))
                mvar.valueReference = std::stoul(attr->value());
            else
                throw std::runtime_error("cannot find 'reference' property in variable \n");

            if (auto attr = variable_node->first_attribute("description"))
                mvar.description = attr->value();

            if (auto attr = variable_node->first_attribute("variability"))
                mvar.variability = attr->value();

            if (auto attr = variable_node->first_attribute("causality"))
                mvar.causality = attr->value();

            if (auto attr = variable_node->first_attribute("initial"))
                mvar.initial = attr->value();

            // fetch type from sub node

            if (auto variables_type = variable_node->first_node("Real"))
                mvar.type = FmuScalarVariable::Type::FMU_REAL;

            if (auto variables_type = variable_node->first_node("String"))
                mvar.type = FmuScalarVariable::Type::FMU_STRING;

            if (auto variables_type = variable_node->first_node("Integer"))
                mvar.type = FmuScalarVariable::Type::FMU_INTEGER;

            if (auto variables_type = variable_node->first_node("Boolean"))
                mvar.type = FmuScalarVariable::Type::FMU_BOOLEAN;

            flat_variables[mvar.name] = mvar;

        }

        delete doc_ptr;
    }

    /// Load the DLL in run-time and do the dynamic linking to 
    /// the needed FMU functions.
    void LoadDLL() {

        std::string dll_dir = this->directory + this->binaries_dir;

        if (!SetDllDirectory(dll_dir.c_str()))
            throw std::runtime_error("Could not locate the DLL directory.");

        std::string dll_name;
        dll_name = dll_dir + "/" + this->info_cosimulation_modelIdentifier + ".dll";

        // run time loading of dll:
        this->hGetProcIDDLL = LoadLibrary(dll_name.c_str());

        if (!this->hGetProcIDDLL)
            throw std::runtime_error("Could not locate the DLL from its file name " + dll_name + "\n");

        // run time binding of functions

        this->_fmi2SetDebugLogging = (fmi2SetDebugLoggingTYPE*)GetProcAddress(this->hGetProcIDDLL, "fmi2SetDebugLogging");
        if (!this->_fmi2SetDebugLogging)
            throw std::runtime_error("Could not find fmi2SetDebugLogging() in the DLL. Wrong or outdated FMU dll?");

        this->_fmi2Instantiate = (fmi2InstantiateTYPE*)GetProcAddress(this->hGetProcIDDLL, "fmi2Instantiate");
        if (!this->_fmi2Instantiate)
            throw std::runtime_error("Could not find fmi2Instantiate() in the DLL. Wrong or outdated FMU dll?");

        this->_fmi2FreeInstance = (fmi2FreeInstanceTYPE*)GetProcAddress(this->hGetProcIDDLL, "fmi2FreeInstance");
        if (!this->_fmi2FreeInstance)
            throw std::runtime_error("Could not find fmi2FreeInstance() in the DLL. Wrong or outdated FMU dll?");

        this->_fmi2GetVersion = (fmi2GetVersionTYPE*)GetProcAddress(this->hGetProcIDDLL, "fmi2GetVersion");
        if (!this->_fmi2GetVersion)
            throw std::runtime_error("Could not find fmi2GetVersion() in the DLL. Wrong or outdated FMU dll?");

        this->_fmi2GetTypesPlatform = (fmi2GetTypesPlatformTYPE*)GetProcAddress(this->hGetProcIDDLL, "fmi2GetTypesPlatform");
        if (!this->_fmi2GetTypesPlatform)
            throw std::runtime_error("Could not find fmi2GetTypesPlatform() in the DLL. Wrong or outdated FMU dll?");

        this->_fmi2SetupExperiment = (fmi2SetupExperimentTYPE*)GetProcAddress(this->hGetProcIDDLL, "fmi2SetupExperiment");
        if (!this->_fmi2SetupExperiment)
            throw std::runtime_error("Could not find fmi2SetupExperiment() in the DLL. Wrong or outdated FMU dll?");

        this->_fmi2EnterInitializationMode = (fmi2EnterInitializationModeTYPE*)GetProcAddress(this->hGetProcIDDLL, "fmi2EnterInitializationMode");
        if (!this->_fmi2EnterInitializationMode)
            throw std::runtime_error("Could not find fmi2EnterInitializationMode() in the DLL. Wrong or outdated FMU dll?");

        this->_fmi2ExitInitializationMode = (fmi2ExitInitializationModeTYPE*)GetProcAddress(this->hGetProcIDDLL, "fmi2ExitInitializationMode");
        if (!this->_fmi2ExitInitializationMode)
            throw std::runtime_error("Could not find fmi2ExitInitializationMode() in the DLL. Wrong or outdated FMU dll?");

        this->_fmi2Terminate = (fmi2TerminateTYPE*)GetProcAddress(this->hGetProcIDDLL, "fmi2Terminate");
        if (!this->_fmi2Terminate)
            throw std::runtime_error("Could not find fmi2Terminate() in the DLL. Wrong or outdated FMU dll?");

        this->_fmi2Reset = (fmi2ResetTYPE*)GetProcAddress(this->hGetProcIDDLL, "fmi2Reset");
        if (!this->_fmi2Reset)
            throw std::runtime_error("Could not find fmi2Reset() in the DLL. Wrong or outdated FMU dll?");

        this->_fmi2GetReal = (fmi2GetRealTYPE*)GetProcAddress(this->hGetProcIDDLL, "fmi2GetReal");
        if (!this->_fmi2GetReal)
            throw std::runtime_error("Could not find fmi2GetReal() in the DLL. Wrong or outdated FMU dll?");

        this->_fmi2GetInteger = (fmi2GetIntegerTYPE*)GetProcAddress(this->hGetProcIDDLL, "fmi2GetInteger");
        if (!this->_fmi2GetInteger)
            throw std::runtime_error("Could not find fmi2GetInteger() in the DLL. Wrong or outdated FMU dll?");

        this->_fmi2GetBoolean = (fmi2GetBooleanTYPE*)GetProcAddress(this->hGetProcIDDLL, "fmi2GetBoolean");
        if (!this->_fmi2GetBoolean)
            throw std::runtime_error("Could not find fmi2GetBoolean() in the DLL. Wrong or outdated FMU dll?");

        this->_fmi2GetString = (fmi2GetStringTYPE*)GetProcAddress(this->hGetProcIDDLL, "fmi2GetString");
        if (!this->_fmi2GetString)
            throw std::runtime_error("Could not find fmi2GetString() in the DLL. Wrong or outdated FMU dll?");

        this->_fmi2SetReal = (fmi2SetRealTYPE*)GetProcAddress(this->hGetProcIDDLL, "fmi2SetReal");
        if (!this->_fmi2SetReal)
            throw std::runtime_error("Could not find fmi2SetReal() in the DLL. Wrong or outdated FMU dll?");

        this->_fmi2SetInteger = (fmi2SetIntegerTYPE*)GetProcAddress(this->hGetProcIDDLL, "fmi2SetInteger");
        if (!this->_fmi2SetInteger)
            throw std::runtime_error("Could not find fmi2SetInteger() in the DLL. Wrong or outdated FMU dll?");

        this->_fmi2SetBoolean = (fmi2SetBooleanTYPE*)GetProcAddress(this->hGetProcIDDLL, "fmi2SetBoolean");
        if (!this->_fmi2SetBoolean)
            throw std::runtime_error("Could not find fmi2SetBoolean() in the DLL. Wrong or outdated FMU dll?");

        this->_fmi2SetString = (fmi2SetStringTYPE*)GetProcAddress(this->hGetProcIDDLL, "fmi2SetString");
        if (!this->_fmi2SetString)
            throw std::runtime_error("Could not find fmi2SetString() in the DLL. Wrong or outdated FMU dll?");

        this->_fmi2DoStep = (fmi2DoStepTYPE*)GetProcAddress(this->hGetProcIDDLL, "fmi2DoStep");
        if (!this->_fmi2DoStep)
            throw std::runtime_error("Could not find fmi2DoStep() in the DLL. Wrong or outdated FMU dll?");

    }

    /// From the flat variable list build a tree of variables
    void BuildVariablesTree() {
        for (auto& iv : this->flat_variables) {
            std::string token;
            std::istringstream ss(iv.second.name);

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
                std::cout << " -> FMU reference:" << in.second.leaf->valueReference;
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
                my_v.pos_references[0] = found_r1->second.leaf->valueReference;
                my_v.pos_references[1] = found_r2->second.leaf->valueReference;
                my_v.pos_references[2] = found_r3->second.leaf->valueReference;
                my_v.pos_shape_references[0] = found_rshape1->second.leaf->valueReference;
                my_v.pos_shape_references[1] = found_rshape2->second.leaf->valueReference;
                my_v.pos_shape_references[2] = found_rshape3->second.leaf->valueReference;
                my_v.rot_references[0] = found_T11->second.leaf->valueReference;
                my_v.rot_references[1] = found_T12->second.leaf->valueReference;
                my_v.rot_references[2] = found_T13->second.leaf->valueReference;
                my_v.rot_references[3] = found_T21->second.leaf->valueReference;
                my_v.rot_references[4] = found_T22->second.leaf->valueReference;
                my_v.rot_references[5] = found_T23->second.leaf->valueReference;
                my_v.rot_references[6] = found_T31->second.leaf->valueReference;
                my_v.rot_references[7] = found_T32->second.leaf->valueReference;
                my_v.rot_references[8] = found_T33->second.leaf->valueReference;
                my_v.shapetype_reference = found_shtype->second.leaf->valueReference;
                my_v.l_references[0] = found_l1->second.leaf->valueReference;
                my_v.l_references[1] = found_l2->second.leaf->valueReference;
                my_v.l_references[2] = found_l3->second.leaf->valueReference;
                my_v.w_references[0] = found_w1->second.leaf->valueReference;
                my_v.w_references[1] = found_w2->second.leaf->valueReference;
                my_v.w_references[2] = found_w3->second.leaf->valueReference;
                my_v.color_references[0] = found_color1->second.leaf->valueReference;
                my_v.color_references[1] = found_color2->second.leaf->valueReference;
                my_v.color_references[2] = found_color3->second.leaf->valueReference;
                my_v.width_reference = found_width->second.leaf->valueReference;
                my_v.length_reference = found_length->second.leaf->valueReference;
                my_v.height_reference = found_height->second.leaf->valueReference;
                my_v.visualizer_node = mynode;
                this->visualizers.push_back(my_v);
            }
        }
        // recursion
        for (auto& in : mynode->children) {
            BuildVisualizersList(&in.second);
        }
    }



    /// Instantiate the model:
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



};
