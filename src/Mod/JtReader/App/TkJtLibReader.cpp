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
#include "JtPartHandle.h"


#include <JtData_Model.hxx>
#include <JtNode_Partition.hxx>
#include <JtData_Object.hxx>
#include <JtNode_Part.hxx>
#include <JtNode_RangeLOD.hxx>
#include <JtElement_ShapeLOD_TriStripSet.hxx>
#include <JtNode_Shape_TriStripSet.hxx>
#include <JtNode_Instance.hxx>

#include <QFileInfo>
#include <QDir>



using namespace std;
using namespace JtReader;


TkJtLibReader::TkJtLibReader(const char* jtFileName)
    :doDeepRead(false)
{
    // only needed when reed multipart:
    QFileInfo fileInfo(QString::fromUtf8(jtFileName));
    QDir dir(fileInfo.absoluteDir());
    QDir::setCurrent(dir.absolutePath());
    

    model = new Handle_JtData_Model();
    partition = new Handle(JtNode_Partition);

    *model = new JtData_Model(TCollection_ExtendedString(jtFileName));
	//Base::Console().Log("FcLodHandler::startLod()");

    (*partition) = (*model)->Init(); // inti reads the TOC

}

bool TkJtLibReader::isValid() const 
{ 
    return !(*partition).IsNull(); 
}

void TkJtLibReader::Dump( std::ostream &output)
{
    traverseDump((*partition),output);

}

const char* indentStr = "   ";

void TkJtLibReader::traverseDump(const Handle_JtData_Object& obj, std::ostream &output, int indent){

    // write the indention level to console
    for (int j = 0; j < indent; j++)
        output << indentStr;
    obj->Dump(output); 
    // if it is a Node_Base object it has a name...
    const Handle(JtNode_Base) node = Handle(JtNode_Base)::DownCast(obj);
    if (!node.IsNull()){
        output << " - \"";
        node->Name().Print(output);
        output << "\"";
    }
    output << endl;

    if (!node.IsNull()){
        // get the attributes
        const JtData_Object::VectorOfObjects& attibutes = node->Attributes();
        for (JtData_Object::VectorOfObjects::SizeType i = 0; i < attibutes.Count(); i++){
            for (int j = 0; j < indent + 1; j++)
                output << indentStr;
            output << "-(" << attibutes[i]->DynamicType()->Name() << ")";
            attibutes[i].Dump(output); output << attibutes[i]->DynamicType() << endl;
        }
    }


    // get the type name and switch behavior
    string typeName(obj->DynamicType()->Name());

    if (typeName == "JtNode_Partition"){
        const Handle(JtNode_Partition) partition = Handle(JtNode_Partition)::DownCast(obj);

        // load the rest of the referenced jt files:
        if (doDeepRead && partition->Children().Count() <= 0)
            partition->Load(); // TODO reading referenced jt files still buggy
 
    }
    else if (typeName == "JtNode_Part"){
        const Handle(JtNode_Part) part = Handle(JtNode_Part)::DownCast(obj);
 
        const JtData_Object::VectorOfLateLoads& lateLoads = part->LateLoads();
        for (JtData_Object::VectorOfObjects::SizeType i = 0; i < lateLoads.Count(); i++){
            const Handle(JtProperty_LateLoaded) lateLoad = lateLoads[i];
            lateLoad->Load();
            Handle(JtData_Object) anObject = lateLoad->DefferedObject();
            if (!anObject.IsNull())
            {
                for (int j = 0; j < indent+1; j++)
                    output << indentStr;
                output << "#(" << anObject->DynamicType()->Name() << ")";
                anObject->Dump(output); output << endl;

                lateLoad->Unload();
            }
        }
    }
    else if (typeName == "JtNode_RangeLOD"){
        const Handle(JtNode_RangeLOD) lod = Handle(JtNode_RangeLOD)::DownCast(obj);

    }
    else if (typeName == "JtNode_LOD"){
        const Handle(JtNode_LOD) lod = Handle(JtNode_LOD)::DownCast(obj);

    }
    else if (typeName == "JtNode_Group"){
        const Handle(JtNode_Group) group = Handle(JtNode_Group)::DownCast(obj);

    }
    else if (typeName == "JtNode_Instance"){
        const Handle(JtNode_Instance) instance = Handle(JtNode_Instance)::DownCast(obj);

        const Handle(JtData_Object)& instancedObj = instance->Object();
        traverseDump(instancedObj,output, indent + 1);

    }
    else if (typeName == "JtNode_Shape_TriStripSet"){
        const Handle(JtNode_Shape_TriStripSet) stripSet = Handle(JtNode_Shape_TriStripSet)::DownCast(obj);

        const JtData_Object::VectorOfLateLoads& lateLoads = stripSet->LateLoads();
        for (JtData_Object::VectorOfObjects::SizeType i = 0; i < lateLoads.Count(); i++){
            const Handle(JtProperty_LateLoaded) lateLoad = lateLoads[i];
            lateLoad->Load();
            Handle(JtData_Object) anObject = lateLoad->DefferedObject();
            if (!anObject.IsNull())
            {
                for (int j = 0; j < indent+1; j++)
                    output << indentStr;
                output << "#(" << anObject->DynamicType()->Name() << ")";
                anObject->Dump(output); output << endl;

                Handle(JtElement_ShapeLOD_TriStripSet) aLOD =
                    Handle(JtElement_ShapeLOD_TriStripSet)::DownCast(anObject);

                if (!aLOD.IsNull())
                {
                    for (int j = 0; j < indent+1; j++)
                        output << indentStr;

                    output << "+ Indexes: " << aLOD->Indices().Count()
                        << " Vertexes:" << aLOD->Vertices().Count()
                        << " Normals:" << aLOD->Normals().Count() << endl;
                }

                lateLoad->Unload();
            }
        }
    }

    // is it a group type traverse further down:
    const Handle(JtNode_Group) group = Handle(JtNode_Group)::DownCast(obj);
    if (!group.IsNull()){
        const JtData_Object::VectorOfObjects& objVector = group->Children();
        for (JtData_Object::VectorOfObjects::SizeType i = 0; i < objVector.Count(); i++){
            const Handle(JtData_Object)& childObj = objVector[i];
            traverseDump(childObj, output, indent + 1);
        }
    }

}

int traverseTypeCount(const Handle_JtData_Object& obj, const char* typeNameToCount){

    int returnVal = 0;
    // get the type name and switch behavior
    string typeName(obj->DynamicType()->Name());

    if (typeName == typeNameToCount)
        returnVal++;

    // is it a group type traverse further down:
    const Handle(JtNode_Group) group = Handle(JtNode_Group)::DownCast(obj);
    if (!group.IsNull()){
        const JtData_Object::VectorOfObjects& objVector = group->Children();
        for (JtData_Object::VectorOfObjects::SizeType i = 0; i < objVector.Count(); i++){
            const Handle(JtData_Object)& childObj = objVector[i];
            returnVal += traverseTypeCount(childObj, typeNameToCount);
        }
    }

    return returnVal;
}

int TkJtLibReader::countParts()
{
    return traverseTypeCount((*partition), "JtNode_Part");
}

int JtReader::TkJtLibReader::countInstances()
{
    return traverseTypeCount((*partition), "JtNode_Instance");
}

int JtReader::TkJtLibReader::countPartitions()
{
    int returnVal = 0;

    // ignore the root Partition and traverse the children:
    const Handle(JtNode_Group) group = Handle(JtNode_Group)::DownCast((*partition));
    if (!group.IsNull()){
        const JtData_Object::VectorOfObjects& objVector = group->Children();
        for (JtData_Object::VectorOfObjects::SizeType i = 0; i < objVector.Count(); i++){
            const Handle(JtData_Object)& childObj = objVector[i];
            returnVal += traverseTypeCount(childObj, "JtNode_Partition");
        }
    }

    return returnVal;
}

// traverse the tree and collect all Parts into a corsoponding PartHandle
void collectPartHandles(const Handle_JtData_Object& obj, const Handle_JtData_Object& parentObj, std::vector<JtPartHandle*>& partHandlerVector)
{
    // get the type name and switch behavior
    string typeName(obj->DynamicType()->Name());

    if (typeName == "JtNode_Part"){
        const Handle(JtNode_Part) part = Handle(JtNode_Part)::DownCast(obj);
        JtPartHandle *newHandle = new JtPartHandle();
        newHandle->init(part,parentObj);
        partHandlerVector.push_back(newHandle);

        // assume no further Part below a Part
        return;
    }

    if (typeName == "JtNode_Instance"){
        const Handle(JtNode_Instance) instance = Handle(JtNode_Instance)::DownCast(obj);

        const Handle(JtData_Object)& instancedObj = instance->Object();
        collectPartHandles(instancedObj, obj, partHandlerVector);

    }
    else{
       // is it a group type traverse further down:
        const Handle(JtNode_Group) group = Handle(JtNode_Group)::DownCast(obj);
        if (!group.IsNull()){
            const JtData_Object::VectorOfObjects& objVector = group->Children();
            for (JtData_Object::VectorOfObjects::SizeType i = 0; i < objVector.Count(); i++){
                const Handle(JtData_Object)& childObj = objVector[i];
                collectPartHandles(childObj, obj , partHandlerVector);
            }
        }
    }
        
 }

std::vector<JtPartHandle*> JtReader::TkJtLibReader::extractPartHandles()
{
    std::vector<JtPartHandle*> retVec;
    retVec.reserve(30);

    collectPartHandles((*partition), Handle_JtData_Object(), retVec);

    return retVec;
}


