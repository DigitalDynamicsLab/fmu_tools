# Fmu Generator

FMUs are required to have some set of standard functions exposed at their interface, as declared in _fmi2Functions.h_. Their definitions, on the contrary, are up to the user to be implemented.

The library, mainly through its main class `ChFmuComponent`, offers a higher C++ layer that should ease this process, taking care of the most trivial tasks and leaving to the user the implementation only those problem-specific parts.

The CMake infrastructure take care of the building of the source code, the generation of the _modelDescription.xml_, the creation of the standard FMU directory layout, and the packaging into a _.fmu_ ZIP archive.


## How to build an FMU

While the library offers some generic features and tools, the user is asked to customize them for its specific problem. This is done by inheriting from `ChFmuComponent`, plus some additional customization.

Looking at the `FmuInstance` class should give a lot of useful hints.

More in detail:

0. in CMake, set the `FMU_MODEL_IDENTIFIER` to any valid name (consider your operating system and C-function naming standards);
1. derive from `ChFmuComponent` your own class; please refer to `FmuInstance` for an example;
2. the derived class should:
   - in the constructor, remember to call `ChFmuComponent::instantiateType(_fmuType)`;
   - in the constructor, add all the relevant variables of the model to the FMU through `addFmuVariableXXX` functions; variable units are supported and some default units are already declared;
   - override `ChFmuComponent::is_cosimulation_available()` and `ChFmuComponent::is_modelexchange_available()` so that they would return the proper answer;
   - override `DoStep` method of the base class with the problem-specific implementation.
3. provide the implementation of `fmi2Instantiate_getPointer` similarly to:
   ```
   ChFmuComponent* fmi2Instantiate_getPointer(
     fmi2String instanceName,
     fmi2Type fmuType,
     fmi2String fmuGUID)
   {
     return new FmuInstance(instanceName, fmuType, fmuGUID);
   }
    ```

While adding new FMU variables, the user can associate a measurement unit to it (otherwise the adimensional unit "1" will be set). However, units needs to be defined _before_ any FMU variable could use them.

Measurement units are defined through the `UnitDefinitionType` class, that stores the name of the unit (e.g. "rad/s2") together with the exponents of each SI base unit (e.g. rad=1, s=-2). The user should create its own object of type `UnitDefinitionType` and then pass it to `ChFmuComponent` through its method `AddUnitDefinition`. After this step, the user can use the unit name in any following call to `addFmuVariableXXX`.

When everything is set up, build the **PACK_FMU** target to generate the FMU file.