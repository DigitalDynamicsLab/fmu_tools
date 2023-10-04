#pragma once

#include "chrono/physics/ChSystemNSC.h"
#include "chrono/physics/ChBodyEasy.h"
#include "chrono/physics/ChLinkMate.h"
#include "chrono/assets/ChTexture.h"
#include "chrono/assets/ChColorAsset.h"
#include "chrono_irrlicht/ChIrrApp.h"
#include "chrono_irrlicht/ChIrrApp.h"
#include "chrono_thirdparty/rapidxml/rapidxml.hpp"

#include "chrono_irrlicht/ChIrrTools.h"
#include "fmi2/fmi2FunctionTypes.h"

// Use the namespace of Chrono

using namespace chrono;
using namespace chrono::irrlicht;

// Use the main namespaces of Irrlicht

using namespace irr;
using namespace irr::core;
using namespace irr::scene;
using namespace irr::video;
using namespace irr::io;
using namespace irr::gui;

using namespace rapidxml;
using namespace std;

// 
// AUXILIARY CALLBACKS FOR FMU
// 

//typedef void      (*fmi2CallbackLogger)        (fmi2ComponentEnvironment, fmi2String, fmi2Status, fmi2String, fmi2String, ...);
//typedef void*     (*fmi2CallbackAllocateMemory)(size_t, size_t);
//typedef void      (*fmi2CallbackFreeMemory)    (void*);
//typedef void      (*fmi2StepFinished)          (fmi2ComponentEnvironment, fmi2Status);
void mylogger(fmi2ComponentEnvironment c, fmi2String instanceName, fmi2Status status, fmi2String category, fmi2String message, ...)
{
    char msg[2024];
    va_list argp;
    va_start(argp, message);
    vsprintf(msg, message, argp);
    if (!instanceName) instanceName = "?";
    if (!category) category = "?";
    printf("fmiStatus = %d;  %s (%s): %s\n", status, instanceName, category, msg);
}

// Type of structure with all pointers to callbacks
typedef struct {
    fmi2CallbackLogger         logger;
    fmi2CallbackAllocateMemory allocateMemory;
    fmi2CallbackFreeMemory     freeMemory;
    fmi2StepFinished           stepFinished;
    fmi2ComponentEnvironment   componentEnvironment;
} fmi2_callback_functions_t_aux;



/////////////////////////////////////////////////////////////////////////////////////////////////


/// Class holding a reference to a single FMU variable
/// Note that this is retreived from the information from the XML

class FMU_variable {
public:
    std::string name;
    fmi2ValueReference valueReference = 0;
    std::string description;
    std::string variability;
    std::string causality;
    std::string initial;

    enum class e_fmu_variable_type {
        Unknown,
        Real,
        Integer,
        Boolean,
        String
    };

    e_fmu_variable_type type = e_fmu_variable_type::Unknown;

    FMU_variable()
    {
    }
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

class FMU_variable_tree_node {
public:
    std::string object_name;

    std::map<std::string, FMU_variable_tree_node> children;
    FMU_variable* leaf = nullptr;

    FMU_variable_tree_node() {}
};


/////////////////////////////////////////////////////////////////////////////////////////////////

/// Class holding a set of scalar variables (3xposition, 9xrotation) for the coordinate
/// system of a visualizer in the FMU. 
/// The visualizer could be a cylinder, a sphere, a mesh, etc.

class FMU_visualizer {
public:
    FMU_variable_tree_node * visualizer_node;

    unsigned int pos_references[3];
    unsigned int rot_references[9];
    unsigned int pos_shape_references[3];
    unsigned int shapetype_reference;
    unsigned int l_references[3];
    unsigned int w_references[3];
    unsigned int color_references[3];
    unsigned int width_reference;
    unsigned int height_reference;
    unsigned int length_reference;

    std::string type;
    std::string filename;
};


/////////////////////////////////////////////////////////////////////////////////////////////////

/// Class holding a set of scalar variables (3xposition, 9xrotation) for the coordinate
/// system of a visualizer in the FMU. 

class FMU_body {
public:

    unsigned int pos_references[3];
    unsigned int rot_references[9];

    std::string name;
};


/////////////////////////////////////////////////////////////////////////////////////////////////

/// Class for managing a FMU.
/// It contains functions to load the DLL in run-time, to parse the XML,
/// to set/get variables, etc.

class FMU_unit {

public:
    /// Construction
    FMU_unit() {
        // default binaries direcotry in FMU unzipped directory
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

    std::map<std::string, FMU_variable> flat_variables;

    FMU_variable_tree_node tree_variables;

    std::vector<FMU_visualizer> visualizers;
    std::vector<FMU_body> bodies;


    // wrappers for runtime dll linking:
private:
    HINSTANCE hGetProcIDDLL;
public:
    //private:
    fmi2_callback_functions_t_aux       callbacks;

    fmi2Component                       component;

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

        xml_document<> doc;
        // Read the xml file into a vector
        ifstream theFile(xml_filename);
        vector<char> buffer((istreambuf_iterator<char>(theFile)), istreambuf_iterator<char>());
        buffer.push_back('\0');
        // Parse the buffer using the xml file parsing library into doc 
        doc.parse<0>(&buffer[0]);

        // Find our root node
        auto root_node = doc.first_node("fmiModelDescription");
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
            FMU_variable mvar;

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
                mvar.type = FMU_variable::e_fmu_variable_type::Real;

            if (auto variables_type = variable_node->first_node("String"))
                mvar.type = FMU_variable::e_fmu_variable_type::String;

            if (auto variables_type = variable_node->first_node("Integer"))
                mvar.type = FMU_variable::e_fmu_variable_type::Integer;

            if (auto variables_type = variable_node->first_node("Boolean"))
                mvar.type = FMU_variable::e_fmu_variable_type::Boolean;

            flat_variables[mvar.name] = mvar;

        }
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
            FMU_variable_tree_node* tree_node = &this->tree_variables;
            while (std::getline(ss, token, '.') && ntokens < 300)
            {
                auto next_node = tree_node->children.find(token);
                if (next_node != tree_node->children.end()) // already added
                {
                    tree_node = &(next_node->second);
                }
                else
                {
                    FMU_variable_tree_node new_node;
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
    void DumpTree(FMU_variable_tree_node* mynode, int tab) {
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

    void BuildBodyList(FMU_variable_tree_node* mynode) {

    }

    void BuildVisualizersList(FMU_variable_tree_node* mynode) {
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

                FMU_visualizer my_v;
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

        this->callbacks.logger = mylogger;
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


    /// CHRONO-SPECIFIC FUNCTIONS

    void FMU_visualizer_fetch(FMU_visualizer& mvis,
        ChVector<>& pos,
        ChMatrix33<>& mat,
        ChVector<>& pos_shape,
        ChVector<>& lengthDirection,
        ChVector<>& widthDirection,
        ChVector<>& color,
        double& width,
        double& height,
        double& length) {
        fmi2Status st;

        st = this->_fmi2GetReal(component, mvis.pos_references, 3, &pos[0]);
        if (st != fmi2OK)
            GetLog() << " ERROR FMU_visualizer_to_csys pos \n";

        st = this->_fmi2GetReal(component, mvis.rot_references, 9, mat.GetAddress());
        if (st != fmi2OK)
            GetLog() << " ERROR FMU_visualizer_to_csys rot \n";

        st = this->_fmi2GetReal(component, mvis.pos_shape_references, 3, &pos_shape[0]);
        if (st != fmi2OK)
            GetLog() << " ERROR FMU_visualizer_to_csys pos_shape \n";

        st = this->_fmi2GetReal(component, mvis.l_references, 3, &lengthDirection[0]);
        if (st != fmi2OK)
            GetLog() << " ERROR FMU_visualizer_to_csys lengthDirection \n";

        st = this->_fmi2GetReal(component, mvis.w_references, 3, &widthDirection[0]);
        if (st != fmi2OK)
            GetLog() << " ERROR FMU_visualizer_to_csys widthDirection \n";

        st = this->_fmi2GetReal(component, mvis.color_references, 3, &color[0]);
        if (st != fmi2OK)
            GetLog() << " ERROR FMU_visualizer_to_csys color \n";

        st = this->_fmi2GetReal(component, &mvis.width_reference, 1, &width);
        if (st != fmi2OK)
            GetLog() << " ERROR FMU_visualizer_to_csys width \n";

        st = this->_fmi2GetReal(component, &mvis.height_reference, 1, &height);
        if (st != fmi2OK)
            GetLog() << " ERROR FMU_visualizer_to_csys height \n";

        st = this->_fmi2GetReal(component, &mvis.length_reference, 1, &length);
        if (st != fmi2OK)
            GetLog() << " ERROR FMU_visualizer_to_csys length \n";

    }

    void FMU_visualizers_create(irr::scene::ISceneNode* shapes_root, irr::scene::ISceneManager* scenemanager) {

        auto sphereMesh = createEllipticalMesh(0.5, 0.5, -2, +2, 0, 15, 8); // rad,rad,..
        auto cubeMesh = createCubeMesh(core::vector3df(1, 1, 1));  // -/+ 0.5 unit each xyz axis
        auto cylinderMesh = createCylinderMesh(0.5, 0.5, 32); // rad, half height, slices
                                                              //auto coneMesh     = createConeMesh(0.5, 0.5, 32); // rad, half height, slices

        for (auto& vis : this->visualizers) {

            // fetch type of shape
            fmi2String m_str;
            auto st = this->_fmi2GetString(component, &vis.shapetype_reference, 1, &m_str);
            if (st != fmi2OK)
                GetLog() << " ERROR getting shapeType \n";
            std::string stype(m_str);

            // create the proper mesh 
            ISceneNode* mchildnode = nullptr;

            if (stype == "sphere") {
                mchildnode = scenemanager->addMeshSceneNode(sphereMesh, shapes_root);
            }
            if (stype == "cylinder") {
                mchildnode = scenemanager->addMeshSceneNode(cylinderMesh, shapes_root);
            }
            //if (stype == "cone") {
            //    mchildnode = scenemanager->addMeshSceneNode(coneMesh, shapes_root);
            //}
            if (stype == "gear") {
                mchildnode = scenemanager->addMeshSceneNode(cylinderMesh, shapes_root); // to improve, with teeths
            }
            if (stype == "pipe") {
                mchildnode = scenemanager->addMeshSceneNode(cylinderMesh, shapes_root); // to improve, with hollow
            }
            if (stype == "box") {
                mchildnode = scenemanager->addMeshSceneNode(cubeMesh, shapes_root);
            }
            if (stype == "beam") {
                mchildnode = scenemanager->addMeshSceneNode(cubeMesh, shapes_root); // to improve, with start-end rounding
            }
            if (stype.find("file://") == 0) {
                std::string filename = stype.substr(7, std::string::npos);
                GetLog() << " FILE AS SHAPE TYPE AT ADDRESS:" << filename << "\n";
                IAnimatedMesh* genericMesh = scenemanager->getMesh(filename.c_str());
                if (genericMesh) {
                    mchildnode = scenemanager->addMeshSceneNode(genericMesh, shapes_root);
                }
                else {
                    GetLog() << " WARNING could not load " << filename << "\n";
                    mchildnode = scenemanager->addMeshSceneNode(cubeMesh, shapes_root);
                }
            }
        } // end loop on visualizers
    }


    void FMU_visualizers_update(irr::scene::ISceneNode* shapes_root, irr::scene::ISceneManager* scenemanager) {

        auto imesh = shapes_root->getChildren().begin();

        for (auto& vis : this->visualizers) {

            if (imesh == shapes_root->getChildren().end())
                break;

            // fetch type of shape
            fmi2String m_str;
            auto st = this->_fmi2GetString(component, &vis.shapetype_reference, 1, &m_str);
            if (st != fmi2OK)
                GetLog() << " ERROR getting shapeType \n";
            std::string stype(m_str);

            ISceneNode* mchildnode = *imesh;

            ChVector<> pos;
            ChMatrix33<> mat;
            ChVector<> pos_shape;
            ChVector<> lengthDirection;
            ChVector<> widthDirection;
            ChVector<> color;
            double width;
            double height;
            double length;

            FMU_visualizer_fetch(vis, pos, mat, pos_shape, lengthDirection, widthDirection, color, width, height, length);

            mat.MatrTranspose();

            ChVector<> Z_shape = Vcross(widthDirection, lengthDirection);

            // Do not assume that FMU widthDirection and lengthDirection are 
            // orthogonal, in some case they aren't, so:
            if (Z_shape.Length() < 0.001) {
                ChVector<> aux_width(widthDirection.y(), widthDirection.z(), widthDirection.x());
                Z_shape = Vcross(aux_width, lengthDirection);
            }
            // Additional safety:
            ChVector<> X_shape = Vcross(lengthDirection, Z_shape);

            ChMatrix33<> mat_shape;
            mat_shape.Set_A_axis(X_shape.GetNormalized(), lengthDirection.GetNormalized(), Z_shape.GetNormalized());

            ChMatrix33<> matTot = mat * mat_shape;

            bool updated_irr_shape = false;

            if (stype == "sphere") {

                ChVector<> meshpos = pos + mat * pos_shape + matTot * (VECT_Y*0.5*length);
                ChCoordsys<> mcoords(meshpos, matTot.Get_A_quaternion());

                irrlicht::ChIrrTools::alignIrrlichtNodeToChronoCsys(mchildnode, mcoords);

                mchildnode->setScale(core::vector3dfCH(ChVector<>(width, length, height)));
                mchildnode->setMaterialFlag(video::EMF_NORMALIZE_NORMALS, true);

                updated_irr_shape = true;
            }
            if (stype == "cylinder" || stype == "gear" || stype == "pipe" || stype == "cone") {

                ChVector<> meshpos = pos + mat * pos_shape + matTot * (VECT_Y*0.5*length);
                ChCoordsys<> mcoords(meshpos, matTot.Get_A_quaternion());

                irrlicht::ChIrrTools::alignIrrlichtNodeToChronoCsys(mchildnode, mcoords);

                mchildnode->setScale(core::vector3dfCH(ChVector<>(width, length, height)));
                mchildnode->setMaterialFlag(video::EMF_NORMALIZE_NORMALS, true);

                updated_irr_shape = true;
            }
            if (stype == "box" || stype == "beam") {

                mchildnode->setScale(core::vector3dfCH(ChVector<>(width, length, height)));

                ChVector<> meshpos = pos + mat * pos_shape + matTot * (VECT_Y*0.5*length);
                ChCoordsys<> mcoords(meshpos, matTot.Get_A_quaternion());

                irrlicht::ChIrrTools::alignIrrlichtNodeToChronoCsys(mchildnode, mcoords);

                updated_irr_shape = true;
            }
            if (stype.find("file://") == 0) {

                mchildnode->setScale(core::vector3dfCH(ChVector<>(width, length, height)));

                ChVector<> meshpos = pos;
                ChCoordsys<> mcoords(meshpos, matTot.Get_A_quaternion());

                irrlicht::ChIrrTools::alignIrrlichtNodeToChronoCsys(mchildnode, mcoords);

                updated_irr_shape = true;
            }

            if (updated_irr_shape) {

                for (unsigned int im = 0; im < mchildnode->getMaterialCount(); ++im) {
                    mchildnode->getMaterial(im).ColorMaterial = irr::video::ECM_NONE;
                    mchildnode->getMaterial(im).DiffuseColor.set(
                        (irr::u32)(255 * 1), (irr::u32)(color.x()),
                        (irr::u32)(color.y()), (irr::u32)(color.z()));
                }

                ++imesh;
            }


        } // end loop on visualizers
    }





};
