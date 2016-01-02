/***************************************************************************
*   Copyright (c) Juergen Riegel         (juergen.riegel@web.de) 2015     *
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
#ifndef _PreComp_
#endif

#include <Base/Console.h>

#include "TkJtLibReader.h"


#include <JtData_Model.hxx>
#include <JtNode_Partition.hxx>
#include <JtData_Object.hxx>
#include <JtNode_Part.hxx>
#include <JtNode_LOD.hxx>

using namespace std;

TkJtLibReader::TkJtLibReader(const char* jtFileName)
{
    model = new Handle_JtData_Model();
    partition = new Handle(JtNode_Partition);

    *model = new JtData_Model(TCollection_ExtendedString(jtFileName));
	//Base::Console().Log("FcLodHandler::startLod()");

    (*partition) = (*model)->Init(); // inti reads the TOC

}


void TkJtLibReader::Dump()
{
    traverse((*partition));

}

void TkJtLibReader::traverse(const Handle_JtData_Object& obj, int indent){

    // write the indention level to console
    for (int i = 0; i < indent; i++)
        cout << "    ";
    obj->Dump(cout); 
    cout << endl;

    // get the type name and switch behavior
    string typeName(obj->DynamicType()->Name());

    if (typeName == "JtNode_Partition"){
        const Handle(JtNode_Partition) partition = Handle(JtNode_Partition)::DownCast(obj);
        const JtData_Object::VectorOfObjects& objVector = partition->Children();
        for (JtData_Object::VectorOfObjects::SizeType i = 0; i < objVector.Count(); i++){
            const Handle(JtData_Object)& childObj = objVector[i];
            traverse(childObj, ++indent);
        }

    }
    else if (typeName == "JtNode_Part"){
        const Handle(JtNode_Part) part = Handle(JtNode_Part)::DownCast(obj);
        const JtData_Object::VectorOfObjects& objVector = part->Children();
        for (JtData_Object::VectorOfObjects::SizeType i = 0; i < objVector.Count(); i++){
            const Handle(JtData_Object)& childObj = objVector[i];
            traverse(childObj, ++indent);
        }

    }
    else if (typeName == "JtNode_LOD"){
        const Handle(JtNode_LOD) lod = Handle(JtNode_LOD)::DownCast(obj);
        const JtData_Object::VectorOfObjects& objVector = lod->Children();
        for (JtData_Object::VectorOfObjects::SizeType i = 0; i < objVector.Count(); i++){
            const Handle(JtData_Object)& childObj = objVector[i];
            traverse(childObj, ++indent);
        }

    }

}