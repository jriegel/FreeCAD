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

#include <JtAttribute_Material.hxx>



using namespace std;
using namespace JtReader;



JtPartHandle::JtPartHandle()
{
    instanceObj = new Handle_JtNode_Instance();
    lodObject = new Handle_JtNode_LOD();
    _hasPartMatrial = false;
}

JtPartHandle::~JtPartHandle()
{
    delete(instanceObj);
    delete(lodObject);

}

void JtPartHandle::getFaces(int lodLevel, int fragment, std::vector<float> &Points, std::vector<float> &Normals, std::vector<int> &Topo, bool &hasMat, JtMat &mat) const
{
    hasMat = false;

    assert(lodLevel < getLodCount());
       

    Handle(JtNode_Group) lod = Handle(JtNode_Group)::DownCast((*lodObject)->Children()[lodLevel]);

    Handle(JtNode_Shape_TriStripSet) stripSet;
    // in case we have no group and directly a TriStrip
    if (lod.IsNull())
        stripSet = Handle(JtNode_Shape_TriStripSet)::DownCast((*lodObject)->Children()[lodLevel]);
    else
        stripSet = Handle(JtNode_Shape_TriStripSet)::DownCast(lod->Children()[fragment]);
 
    assert(!stripSet.IsNull());

    const JtData_Object::VectorOfLateLoads& lateLoads = stripSet->LateLoads();

    // assume on element per tri stripset node
    assert(lateLoads.Count() == 1);
    const Handle(JtProperty_LateLoaded) lateLoad = lateLoads[0];

    // load the element
    lateLoad->Load();
    Handle(JtData_Object) anObject = lateLoad->DefferedObject();
    if (anObject.IsNull())
        return;
    
    Handle(JtElement_ShapeLOD_TriStripSet) aLOD = Handle(JtElement_ShapeLOD_TriStripSet)::DownCast(anObject);
    if (aLOD.IsNull())
        return;

     
    // transfer the triangle indexes (3 per triangle):
    Topo.reserve(aLOD->Indices().Count());
    const Jt_I32 *idxBuf = aLOD->Indices().Data();
    Topo.assign(idxBuf, idxBuf + aLOD->Indices().Count());

    //for (int i = 0; i < aLOD->Indices().Count(); i += 3)
    //    Topo[i / 3] = { idxBuf[i], idxBuf[i + 1], idxBuf[i + 2] };
 
    // transfer the Vertex floats
    Points.reserve(aLOD->Vertices().Count()*3);
    const Jt_F32 *vertBuf = aLOD->Vertices().Data();
    Points.assign(vertBuf, vertBuf + aLOD->Vertices().Count()*3);

    /*for (int i = 0; i < aLOD->Vertices().Count(); i += 3)
        Points[i / 3] = Base::Vector3f(vertBuf[i], vertBuf[i + 1], vertBuf[i + 2]);*/

    // transfer the Normals floats
    Normals.reserve(aLOD->Normals().Count()*3);
    const Jt_F32 *normBuf = aLOD->Normals().Data();
    Normals.assign(normBuf, normBuf + aLOD->Normals().Count()*3);

    // unload the element (free memory in the JT-Reader)
    aLOD.Nullify();
    lateLoad->Unload();

    // get the material attribute
    const JtData_Object::VectorOfObjects& attibutes = stripSet->Attributes();
    for (JtData_Object::VectorOfObjects::SizeType i = 0; i < attibutes.Count(); i++){
        if (string(attibutes[i]->DynamicType()->Name()) == " JtAttribute_Material"){
            Handle(JtAttribute_Material) stripMat = Handle(JtAttribute_Material)::DownCast(attibutes[i]);
            mat.set(stripMat->AmbientColor(), stripMat->DiffuseColor(), stripMat->SpecularColor(), stripMat->EmissionColor(), stripMat->Shininess(), stripMat->Reflectivity());
            hasMat = true;
            break;
        }
    }

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

    // get the material attribute
    const JtData_Object::VectorOfObjects& attibutes = jtPartObject->Attributes();
    for (JtData_Object::VectorOfObjects::SizeType i = 0; i < attibutes.Count(); i++){
        if (string(attibutes[i]->DynamicType()->Name()) == " JtAttribute_Material"){
            Handle(JtAttribute_Material) stripMat = Handle(JtAttribute_Material)::DownCast(attibutes[i]);
            _partMaterial.set(stripMat->AmbientColor(), stripMat->DiffuseColor(), stripMat->SpecularColor(), stripMat->EmissionColor(), stripMat->Shininess(), stripMat->Reflectivity());
            _hasPartMatrial = true;
            break;
        }
    }


    
}

int JtPartHandle::getLodCount() const
{
    return (*lodObject)->Children().Count();
}

const char* JtReader::JtPartHandle::getName() const
{
    return partName.c_str();
}

int JtReader::JtPartHandle::getLodFragmentCount(int lodLevel) const 
{
    Handle(JtNode_Group) lod = Handle(JtNode_Group)::DownCast((*lodObject)->Children()[lodLevel]);

    // in case we have no group and directly a TriStrip
    if (lod.IsNull())
        return 1;
    else
        return lod->Children().Count();
}


const JtReader::JtPartHandle::JtMat &JtReader::JtPartHandle::getPartMaterial() const
{ 
    return _partMaterial; 
}