/***************************************************************************
 *   (c) J�rgen Riegel (juergen.riegel@web.de) 2002                        *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License (LGPL)   *
 *   as published by the Free Software Foundation; either version 2 of     *
 *   the License, or (at your option) any later version.                   *
 *   for detail see the LICENCE text file.                                 *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful,            *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with FreeCAD; if not, write to the Free Software        *
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
 *   USA                                                                   *
 *                                                                         *
 *   Juergen Riegel 2002                                                   *
 ***************************************************************************/


#include "PreCompiled.h"
#ifndef _PreComp_
#endif

#include <Base/Console.h>
#include <Base/Interpreter.h>
#include "StepShapePy.h"
#include "StepShape.h"


/* registration table  */
extern struct PyMethodDef Import_Import_methods[];

PyDoc_STRVAR(module_doc,
"This module is about import/export files formates.\n"
"\n");
extern "C" {
void ImportExport initImport()
{
    PyObject* importModule = Py_InitModule3("Import", Import_Import_methods, module_doc); /* mod name, table ptr */

    try {
        Base::Interpreter().loadModule("Part");
    }
    catch(const Base::Exception& e) {
        PyErr_SetString(PyExc_ImportError, e.what());
        return;
    }

    // add mesh elements
    Base::Interpreter().addType(&Import::StepShapePy  ::Type,importModule,"StepShape");

        // init Type system
    //Import::StepShape       ::init();

    Base::Console().Log("Loading Import module... done\n");
}


} // extern "C" {
