/***************************************************************************
 *   Copyright (c) 2011 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#include "PreCompiled.h"
#if defined(__MINGW32__)
# define WNT // avoid conflict with GUID
#endif
#ifndef _PreComp_
# include <Python.h>
# include <climits>
# include <QString>

#endif

#include <Base/PyObjectBase.h>
#include <Base/Console.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObjectPy.h>
#include <App/PartPy.h>




/* module functions */

static PyObject * readJtPartAsync(PyObject *self, PyObject *args)
{
    char* Name;
    PyObject* PartObject = 0;
    if (!PyArg_ParseTuple(args, "etO!", "utf-8", &Name, &(App::PartPy::Type), &PartObject))
        return 0;
    std::string Utf8Name = std::string(Name);
    PyMem_Free(Name);
    App::Part* Part = static_cast<App::PartPy*>(PartObject)->getPartPtr();

    PY_TRY {
        //Base::Console().Log("Insert in Part with %s",Name);
        Base::FileInfo file(Utf8Name.c_str());

 

    }
    PY_CATCH

    Py_Return;
}



/* registration table  */
struct PyMethodDef JtReaderGui_JtReader_methods[] = {
    { "readJtPartAsync", readJtPartAsync, METH_VARARGS,
     "readJtPartAsync(JtFilePath,PartObject) -- Read a Jt into a PartObject asynchronously via a thread"},
    {NULL, NULL}                   /* end of table marker */
};
