
#ifndef FMUTOOLSEXPORT_H
#define FMUTOOLSEXPORT_H
#include "FmuToolsCommon.h"
#include "fmi2_headers/fmi2Functions.h"
#include <algorithm>
#include <cassert>
#include <vector>
#include <array>
#include <map>
#include <iostream>
#include <string>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <list>


#ifndef FMITYPESPLATFORM_CUSTOM
    #include "TypesVariantsDefault.h"
#else
    #include "TypesVariantsCustom.h"
#endif




struct UnitDefinitionType{

    UnitDefinitionType(const std::string& _name = "1"): name(_name){}

    UnitDefinitionType(const std::string& _name, int _kg, int _m, int _s, int _A, int _K, int _mol, int _cd, int _rad):
        name(_name),
        kg(_kg),
        m(_m),
        s(_s),
        A(_A),
        K(_K),
        mol(_mol),
        cd(_cd),
        rad(_rad){}

    virtual ~UnitDefinitionType(){}

    std::string name;
    int kg = 0;
    int m = 0;
    int s = 0;
    int A = 0;
    int K = 0;
    int mol = 0;
    int cd = 0;
    int rad = 0;

    struct Hash {
        size_t operator()(const UnitDefinitionType& p) const {
            return std::hash<std::string>()(p.name);
        }
    };

    bool operator==(const UnitDefinitionType& other) const {
        return name == other.name;
    }

};


extern const std::unordered_set<UnitDefinitionType, UnitDefinitionType::Hash> common_unitdefinitions;


#ifdef __cplusplus
extern "C" {
#endif

    void FMI2_Export createModelDescription(const std::string& path, fmi2Type fmutype, fmi2String guid);

#ifdef __cplusplus
}
#endif


// Default UnitDefinitionTypes          |name|kg, m, s, A, K,mol,cd,rad
static const UnitDefinitionType UD_kg  ("kg",  1, 0, 0, 0, 0, 0, 0, 0 );
static const UnitDefinitionType UD_m   ("m",   0, 1, 0, 0, 0, 0, 0, 0 );
static const UnitDefinitionType UD_s   ("s",   0, 0, 1, 0, 0, 0, 0, 0 );
static const UnitDefinitionType UD_A   ("A",   0, 0, 0, 1, 0, 0, 0, 0 );
static const UnitDefinitionType UD_K   ("K",   0, 0, 0, 0, 1, 0, 0, 0 );
static const UnitDefinitionType UD_mol ("mol", 0, 0, 0, 0, 0, 1, 0, 0 );
static const UnitDefinitionType UD_cd  ("cd",  0, 0, 0, 0, 0, 0, 1, 0 );
static const UnitDefinitionType UD_rad ("rad", 0, 0, 0, 0, 0, 0, 0, 1 );

static const UnitDefinitionType UD_m_s    ("m/s",    0, 1, -1, 0, 0, 0, 0, 0 );
static const UnitDefinitionType UD_m_s2   ("m/s2",   0, 1, -2, 0, 0, 0, 0, 0 );
static const UnitDefinitionType UD_rad_s  ("rad/s",  0, 0, -1, 0, 0, 0, 0, 1 );
static const UnitDefinitionType UD_rad_s2 ("rad/s2", 0, 0, -2, 0, 0, 0, 0, 1 );


#define MAKE_GETSET_PAIR(returnType, codeGet, codeSet) \
    std::make_pair(std::function<returnType()>([this]() -> returnType  \
        codeGet \
    ), \
    std::function<void(returnType)>([this](returnType val)  \
        codeSet \
    ))

template<class T>
using FunGetSet = std::pair<std::function<T(void)>, std::function<void(T)>>;


bool is_pointer_variant(const FmuVariableBindType& myVariant);


class FmuVariableExport : public FmuVariable {
public:


    using VarbindType = FmuVariableBindType;
    using StartType = FmuVariableStartType;


    FmuVariableExport(
        const VarbindType& varbind,
        const std::string& _name,
        FmuVariable::Type _type,
        CausalityType _causality = CausalityType::local,
        VariabilityType _variability = VariabilityType::continuous,
        InitialType _initial = InitialType::none):
        FmuVariable(_name, _type, _causality, _variability, _initial)
    {

        // From FMI Reference
        // If initial = 'exact' or 'approx', or causality = 'input',       a start value MUST be provided.
        // If initial = 'calculated',        or causality = 'independent', a start value CANNOT be provided. 
        if (initial == InitialType::calculated || causality == CausalityType::independent){
            allowed_start = false;
            required_start = false;
        }

        if (initial == InitialType::exact || initial == InitialType::approx || causality == CausalityType::input){
            allowed_start = true;
            required_start = true;
        }

        this->varbind = varbind;

    }


    FmuVariableExport(const FmuVariableExport& other) : FmuVariable(other) {
        
        varbind = other.varbind;
        start = other.start;
        allowed_start = other.allowed_start;
        required_start = other.required_start;

    }


    // Copy assignment operator
    FmuVariableExport& operator=(const FmuVariableExport& other) {
        if (this == &other) {
            return *this; // Self-assignment guard
        }

        FmuVariable::operator=(other);

        varbind = other.varbind;
        start = other.start;
        allowed_start = other.allowed_start;
        required_start = other.required_start;

        return *this;
    }

    
    void Bind(VarbindType newvarbind){
        varbind = newvarbind;
    }

    template <class T>
    void SetValue(const T& val) const {
        if (is_pointer_variant(this->varbind))
            *varns::get<T*>(this->varbind) = val;
        else
            varns::get<FunGetSet<T>>(this->varbind).second(val);
    }


    template <typename T>
    void GetValue(T* varptr) const {
        *varptr = is_pointer_variant(this->varbind) ? *varns::get<T*>(this->varbind) : varns::get<FunGetSet<T>>(this->varbind).first();
    }


    template <typename T, typename = typename std::enable_if<!std::is_same<T, fmi2String>::value>::type>
    void SetStartVal(T startval){
        if (allowed_start)
            has_start = true;
        else
            return;

        this->start = startval;
    }

    void SetStartVal(fmi2String startval){
        if (allowed_start)
            has_start = true;
        else
            return;
        this->start = std::string(startval);
    }


    void ExposeCurrentValueAsStart(){
        if (required_start){

            varns::visit([this](auto&& arg){
                    this->setStartFromVar(arg);
                }, this->varbind);


            //if (is_pointer_variant(this->varbind)){
            //    // check if string TODO: check if this check is needed .-)
            //    if (varns::holds_alternative<fmi2String*>(this->varbind)) {
            //        // TODO
            //        varns::visit([this](auto& arg){ return this->start = arg; }, this->varbind);
            //    }
            //    else{
            //        varns::visit([this](auto& arg){ return this->SetStartVal(*arg); }, this->varbind);
            //    }
            //}
            //else{

            //}
        }
    }


    std::string GetStartVal_toString() const {

        // TODO: C++17 would allow the overload operator in lambda
        //std::string start_string;
        //varns::visit([&start_string](auto&& arg) -> std::string {return start_string = std::to_string(*start_ptr)});

        if (const fmi2Real* start_ptr = varns::get_if<fmi2Real>(&this->start))
            return std::to_string(*start_ptr);
        if (const fmi2Integer* start_ptr = varns::get_if<fmi2Integer>(&this->start))
            return std::to_string(*start_ptr);
        if (const fmi2Boolean* start_ptr = varns::get_if<fmi2Boolean>(&this->start))
            return std::to_string(*start_ptr);
        if (const std::string* start_ptr = varns::get_if<std::string>(&this->start))
            return *start_ptr;
        return "";
    }



protected:

    bool allowed_start = true;
    bool required_start = false;

    VarbindType varbind;
    StartType start;


    // TODO: in C++17 should be possible to either use constexpr or use lambda with 'overload' keyword
    template <typename T>
    void setStartFromVar(FunGetSet<T> funPair){
        if (allowed_start)
            has_start = true;
        else
            return;

        this->start = funPair.first();
    }

    template <typename T>
    void setStartFromVar(T* var_ptr){
        if (allowed_start)
            has_start = true;
        else
            return;

        this->start = *var_ptr;
    }

};



class FmuComponentBase{

public:
    FmuComponentBase(fmi2String _instanceName, fmi2Type _fmuType, fmi2String _fmuGUID):
        callbackFunctions({nullptr, nullptr, nullptr, nullptr, nullptr}),
        instanceName(_instanceName),
        fmuGUID(FMU_GUID),
        modelIdentifier(FMU_MODEL_IDENTIFIER),
        fmuMachineState(FmuMachineStateType::instantiated)
    {


        unitDefinitions["1"] = UnitDefinitionType("1"); // guarantee the existence of the default unit
        unitDefinitions[""] = UnitDefinitionType(""); // guarantee the existence of the unassigned unit

        AddFmuVariable(&time, "time", FmuVariable::Type::Real, "s", "time");

        if(std::string(_fmuGUID).compare(fmuGUID) && ENABLE_GUID_CHECK)
            throw std::runtime_error("GUID used for instantiation not matching with source.");

    }

    virtual ~FmuComponentBase(){}

    void SetResourceLocation(fmi2String resloc){
        resourceLocation = std::string(resloc);
    }
    

    void SetDefaultExperiment(fmi2Boolean _toleranceDefined, fmi2Real _tolerance, fmi2Real _startTime, fmi2Boolean _stopTimeDefined, fmi2Real _stopTime){
        startTime = _startTime;
        stopTime = _stopTime;
        tolerance = _tolerance;
        toleranceDefined = _toleranceDefined;
        stopTimeDefined = _stopTimeDefined;
    }

    const std::set<FmuVariableExport>& GetScalarVariables() const { return scalarVariables; }

    void EnterInitializationMode(){
        fmuMachineState = FmuMachineStateType::initializationMode;
        _enterInitializationMode();
    };

    void ExitInitializationMode(){
        _exitInitializationMode();
        fmuMachineState = FmuMachineStateType::stepCompleted; // TODO: introduce additional state when after initialization and before step?
    };

    void SetCallbackFunctions(const fmi2CallbackFunctions* functions){ callbackFunctions = *functions; }

    void SetLogging(bool val){loggingOn = val; };

    void AddCallbackLoggerCategory(std::string cat){
        if (logCategories_available.find(cat) == logCategories_available.end())
            throw std::runtime_error(std::string("Log category \"") + cat + std::string("\" is not valid."));
        logCategories.insert(cat);
    }

    fmi2Status DoStep(fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize, fmi2Boolean noSetFMUStatePriorToCurrentPoint) {
        fmi2Status doStep_status = _doStep(currentCommunicationPoint, communicationStepSize, noSetFMUStatePriorToCurrentPoint);

        // once the step is done make sure all the auxiliary variables are updated as well
        updateVars();

        switch (doStep_status)
        {
        case fmi2OK:
            fmuMachineState = FmuMachineStateType::stepCompleted;
            break;
        case fmi2Warning:
            fmuMachineState = FmuMachineStateType::stepCompleted;
            break;
        case fmi2Discard:
            fmuMachineState = FmuMachineStateType::stepFailed;
            break;
        case fmi2Error:
            fmuMachineState = FmuMachineStateType::error;
            break;
        case fmi2Fatal:
            fmuMachineState = FmuMachineStateType::fatal;
            break;
        case fmi2Pending:
            fmuMachineState = FmuMachineStateType::stepInProgress;
            break;
        default:
            throw std::runtime_error("Developer error: unexpected status from _doStep");
            break;
        }

        return doStep_status;
    }

    void ExportModelDescription(std::string path);

    double GetTime() const {return time;}

    void updateVars(){
        for (auto& callb : updateVarsCallbacks){
            callb();
        }
    }

    template <class T>
    fmi2Status fmi2GetVariable(const fmi2ValueReference vr[], size_t nvr, T value[], FmuVariable::Type vartype) {
        //TODO, when multiple variables are requested it might be better to iterate through scalarVariables just once
        // and check if they match any of the nvr requested variables 
        for (size_t s = 0; s<nvr; ++s){
            auto it = this->findByValrefType(vr[s], vartype);
            if (it != this->scalarVariables.end()){
                it->GetValue(&value[s]);
            }
            else
                return fmi2Status::fmi2Error; // requested a variable that does not exist
        }
        return fmi2Status::fmi2OK;
    }

    template <class T>
    fmi2Status fmi2SetVariable(const fmi2ValueReference vr[], size_t nvr, const T value[], FmuVariable::Type vartype) {
        for (size_t s = 0; s<nvr; ++s){
            std::set<FmuVariableExport>::iterator it = this->findByValrefType(vr[s], vartype);
            if (it != this->scalarVariables.end() && it->IsSetAllowed(this->fmuType, this->fmuMachineState)){
                it->SetValue(value[s]);
            }
            else
                return fmi2Status::fmi2Error; // requested a variable that does not exist or that cannot be set
        }
        return fmi2Status::fmi2OK;
    }


    // DEV: unfortunately it is not possible to retrieve the fmi2 type based on the var_ptr only; the reason is that:
    // e.g. both fmi2Integer and fmi2Boolean are actually alias of type int, thus impeding any possible splitting depending on type
    // if we accept to have both fmi2Integer and fmi2Boolean considered as the same type we can drop the 'scalartype' argument
    // but the risk is that a variable might end up being flagged as Integer while it's actually a Boolean and it is not nice
    // At least, in this way, we do not have any redundant code at least
    const FmuVariable& AddFmuVariable(
            const FmuVariableExport::VarbindType& varbind,
            std::string name,
            FmuVariable::Type scalartype = FmuVariable::Type::Real,
            std::string unitname = "",
            std::string description = "",
            FmuVariable::CausalityType causality = FmuVariable::CausalityType::local,
            FmuVariable::VariabilityType variability = FmuVariable::VariabilityType::continuous,
            FmuVariable::InitialType initial = FmuVariable::InitialType::none)
    {

        // check if unit definition exists
        auto match_unit = unitDefinitions.find(unitname);
        if (match_unit == unitDefinitions.end()){
            auto predicate_samename = [unitname](const UnitDefinitionType& var) { return var.name == unitname; };
            auto match_commonunit = std::find_if(common_unitdefinitions.begin(), common_unitdefinitions.end(), predicate_samename);
            if (match_commonunit == common_unitdefinitions.end()){
                throw std::runtime_error("Variable unit is not registered within this FmuComponentBase. Call 'addUnitDefinition' first.");
            }
            else{
                addUnitDefinition(*match_commonunit);
            }
        }


        // create new variable
        // check if same-name variable exists
        std::set<FmuVariableExport>::iterator it = this->findByName(name);
        if (it!=scalarVariables.end())
            throw std::runtime_error("Cannot add two Fmu variables with the same name.");




        FmuVariableExport newvar(varbind, name, scalartype, causality, variability, initial);
        newvar.SetUnitName(unitname);
        newvar.SetValueReference(++valueReferenceCounter[scalartype]);
        newvar.SetDescription(description);

        // check that the attributes of the variable would allow a no-set variable
        const FmuMachineStateType tempFmuState = FmuMachineStateType::anySettableState;

        newvar.ExposeCurrentValueAsStart();
        //varns::visit([&newvar](auto var_ptr_expanded) { newvar.SetStartValIfRequired(var_ptr_expanded);}, var_ptr);



        std::pair<std::set<FmuVariableExport>::iterator, bool> ret = scalarVariables.insert(newvar);
        if(!ret.second)
            throw std::runtime_error("Developer error: cannot insert new variable into FMU.");

        return *(ret.first);
    }

    bool RebindVariable(FmuVariableExport::VarbindType varbind, std::string name){
        std::set<FmuVariableExport>::iterator it = this->findByName(name);
        if (it != scalarVariables.end()){

            FmuVariableExport newvar(*it);
            newvar.Bind(varbind);
            scalarVariables.erase(*it);

            std::pair<std::set<FmuVariableExport>::iterator, bool> ret = scalarVariables.insert(newvar);

            return ret.second;
        }
    }

protected:
    virtual fmi2Status _doStep(fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize, fmi2Boolean noSetFMUStatePriorToCurrentPoint) = 0;

    virtual void _preModelDescriptionExport() {}
    virtual void _postModelDescriptionExport() {}

    virtual void _enterInitializationMode() {}

    virtual void _exitInitializationMode() {}

    void initializeType(fmi2Type _fmuType){
        switch (_fmuType)
        {
        case fmi2Type::fmi2CoSimulation:
            if (!is_cosimulation_available())
                throw std::runtime_error("Requested CoSimulation FMU mode but it is not available.");
            fmuType = fmi2Type::fmi2CoSimulation;
            break;
        case fmi2Type::fmi2ModelExchange:
            if (!is_modelexchange_available())
                throw std::runtime_error("Requested ModelExchange FMU mode but it is not available.");
            fmuType = fmi2Type::fmi2ModelExchange;
            break;
        default:
            throw std::runtime_error("Requested unrecognized FMU type.");
            break;
        }
    }

    std::string instanceName;
    std::string fmuGUID;
    std::string resourceLocation;

    static const std::set<std::string> logCategories_available;
    std::set<std::string> logCategories;

    // DefaultExperiment
    fmi2Real startTime = 0;
    fmi2Real stopTime = 1;
    fmi2Real tolerance = -1;
    fmi2Boolean toleranceDefined = 0;
    fmi2Boolean stopTimeDefined = 0;

    fmi2Real stepSize = 1e-3;
    fmi2Real time = 0;

    const std::string modelIdentifier;

    fmi2Type fmuType;

    std::map<FmuVariable::Type, unsigned int> valueReferenceCounter;

    std::set<FmuVariableExport> scalarVariables;
    std::unordered_map<std::string, UnitDefinitionType> unitDefinitions;

    std::set<FmuVariableExport>::iterator findByValrefType(fmi2ValueReference vr, FmuVariable::Type vartype);
    std::set<FmuVariableExport>::iterator findByName(const std::string& name);

    std::list<std::function<void(void)>> updateVarsCallbacks;


    fmi2CallbackFunctions callbackFunctions;
    bool loggingOn = true;
    FmuMachineStateType fmuMachineState;

    
    virtual bool is_cosimulation_available() const = 0;
    virtual bool is_modelexchange_available() const = 0;

    void addUnitDefinition(const UnitDefinitionType& newunitdefinition){
        unitDefinitions[newunitdefinition.name] = newunitdefinition;
    }

    void clearUnitDefinitions(){
        unitDefinitions.clear();
    }


    
    void sendToLog(std::string msg, fmi2Status status, std::string msg_cat){
        if (logCategories_available.find(msg_cat) == logCategories_available.end())
            throw std::runtime_error(std::string("Log category \"") + msg_cat + std::string("\" is not valid."));

        if (logCategories.find(msg_cat) != logCategories.end()){
            callbackFunctions.logger(callbackFunctions.componentEnvironment, instanceName.c_str(), status, msg_cat.c_str(), msg.c_str());
        }
    }

};


FmuComponentBase* fmi2Instantiate_getPointer(fmi2String instanceName, fmi2Type fmuType, fmi2String fmuGUID);





#endif