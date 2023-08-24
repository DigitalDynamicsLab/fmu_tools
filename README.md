# Fmu Generator

# Layout
FMUs are required to have some set of standard functions exposed at their interface, as declared in _fmi2Functions.h_. Their definitions, on the contrary, are up to the user to be implemented.

The library, mainly through its main class `ChFmuComponent`, offers a higher C++ layer that should ease this process, taking care of the most trivial tasks and leaving to the user only those problem-specific parts.

The CMake infrastructure takes care of the building of the source code, the generation of the _modelDescription.xml_, the creation of the standard FMU directory layout, and the packaging into a _.fmu_ ZIP archive.


## How to build an FMU

While the library offers some generic features and tools, the user is asked to customize them for its specific problem. This is done by inheriting from `ChFmuComponent`, plus some additional customization.

1. derive from `ChFmuComponent`