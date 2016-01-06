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

#include "JtPartHandle.h"


#include <JtData_Model.hxx>
#include <JtData_Object.hxx>

#include <JtNode_Partition.hxx>
#include <JtNode_Part.hxx>
#include <JtNode_Instance.hxx>
#include <JtNode_RangeLOD.hxx>
#include <JtNode_Shape_TriStripSet.hxx>

#include <JtElement_ShapeLOD_TriStripSet.hxx>




using namespace std;
using namespace JtReader;



JtPartHandle::JtPartHandle()
{
    instanceObj = new Handle_JtNode_Instance();
    lodObject = new Handle_JtNode_LOD();

}

JtPartHandle::~JtPartHandle()
{
    delete(instanceObj);
    delete(lodObject);

}

void JtPartHandle::getFaces(int lodLevel, int fragment, std::vector<Base::Vector3d> &Points, std::vector<Base::Vector3d> &Normals, std::vector<Data::ComplexGeoData::Facet> &Topo, bool &hasMat, JtMat &mat)
{
    Points.reserve(10);
    Topo.reserve(10);

}

void JtPartHandle::init(const Handle_JtNode_Part &jtPartObject, const Handle_JtData_Object &parentObject)
{
    // check on Instancing
    const Handle(JtNode_Instance) instance = Handle(JtNode_Instance)::DownCast(parentObject);
    if (!instance.IsNull())
        (*instanceObj) = instance;

    // retrieve name od part
    TCollection_ExtendedString name = jtPartObject->Name();

    if (name.Length() == 0 && !instance.IsNull())
        name = instance->Name();

    if (name.Length() > 0){
        char* str = new char[name.LengthOfCString() + 1];
        name.ToUTF8CString(str);
        partName = str;
        delete[] str;
    }

    // get the LOD structure
    Handle(JtNode_LOD) lod = Handle(JtNode_LOD)::DownCast(jtPartObject->Children()[0]);
    if (lod.IsNull())
        throw "JtPartHandle::init(): Nod LOD node found ";
    (*lodObject) = lod;
    
}

int JtPartHandle::getLodCount()
{
    return (*lodObject)->Children().Count();
}

const char* JtReader::JtPartHandle::getName()
{
    return 0;
}

int JtReader::JtPartHandle::getLodFragmentCount(int lodLevel)
{
    Handle(JtNode_Group) lod = Handle(JtNode_Group)::DownCast((*lodObject)->Children()[lodLevel]);

    // in case we have no group and directly a TriStrip
    if (lod.IsNull())
        return 1;
    else
        return lod->Children().Count();
}


