
#include "PreCompiled.h"

#include "Mod/JtReader/App/JtPart.h"

// inclusion of the generated files (generated out of JtPartPy.xml)
#include "JtPartPy.h"
#include "JtPartPy.cpp"

using namespace JtReader;

// returns a string which represents the object e.g. when printed in python
std::string JtPartPy::representation(void) const
{
    return std::string("<JtPart object>");
}







PyObject *JtPartPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int JtPartPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}


