/***************************************************************************
 *   Copyright (c) Juergen Riegel          (juergen.riegel@web.de) 2012    *
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

#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/details/SoLineDetail.h>
# include <Inventor/SoPickedPoint.h>
# include <Inventor/details/SoFaceDetail.h>
# include <Inventor/details/SoLineDetail.h>
# include <Inventor/details/SoPointDetail.h>
# include <Inventor/events/SoMouseButtonEvent.h>
# include <Inventor/nodes/SoBaseColor.h>
# include <Inventor/nodes/SoCoordinate3.h>
# include <Inventor/nodes/SoDrawStyle.h>
# include <Inventor/nodes/SoIndexedFaceSet.h>
# include <Inventor/nodes/SoIndexedLineSet.h>
# include <Inventor/nodes/SoLocateHighlight.h>
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoMaterialBinding.h>
# include <Inventor/nodes/SoNormal.h>
# include <Inventor/nodes/SoNormalBinding.h>
# include <Inventor/nodes/SoPointSet.h>
# include <Inventor/nodes/SoPolygonOffset.h>
# include <Inventor/nodes/SoShapeHints.h>
# include <Inventor/nodes/SoSwitch.h>
# include <Inventor/nodes/SoGroup.h>
# include <Inventor/nodes/SoSphere.h>
# include <Inventor/nodes/SoScale.h>
# include <Inventor/nodes/SoLightModel.h>

#include "ViewProviderJtPart.h"

#include <Gui/SoObjectSeparator.h>
#include <Gui/Application.h>
#include <Gui/Document.h>

#include <App/PropertyGeo.h>
#include <App/PropertyStandard.h>
#include <Base/Console.h>

#include <Mod/JtReader/App/JtPart.h>
#include <Mod/JtReader/App/JtPartHandle.h>

using namespace JtReaderGui;

PROPERTY_SOURCE(JtReaderGui::ViewProviderJtPart, Gui::ViewProviderGeometryObject)


ViewProviderJtPart::ViewProviderJtPart()
    :partHandle(nullptr)
{
    // sets up base class with material and other standard stuff:
    Gui::ViewProviderGeometryObject::ViewProviderGeometryObject();

    pcJtRootGroup = new SoGroup();
    pcJtRootGroup->ref();

    
}

ViewProviderJtPart::~ViewProviderJtPart()
{
    pcJtRootGroup->unref();
  
}

void ViewProviderJtPart::onChanged(const App::Property* prop)
{
        //if (prop == &Size){
         //}
        //else
    ViewProviderGeometryObject::onChanged(prop);
}

std::vector<std::string> ViewProviderJtPart::getDisplayModes(void) const
{
    // add modes
    std::vector<std::string> StrList;
    StrList.push_back("Main");
    return StrList;
}

void ViewProviderJtPart::setDisplayMode(const char* ModeName)
{
    if (strcmp(ModeName, "Main") == 0)
        setDisplayMaskMode("Main");
    ViewProviderGeometryObject::setDisplayMode(ModeName);
}

void ViewProviderJtPart::attach(App::DocumentObject* pcObject)
{
    ViewProviderGeometryObject::attach(pcObject);

    addDisplayMaskMode(pcJtRootGroup, "Main");

}

void ViewProviderJtPart::updateData(const App::Property* prop)
{
    const JtReader::JtPartHandle* newPartHandle = static_cast<JtReader::JtPart*>(pcObject)->getPartPtr();

    if (newPartHandle != partHandle){
        partHandle = newPartHandle;
        updateJtPartGeometry(newPartHandle);
    }
    ViewProviderGeometryObject::updateData(prop);
}

std::string ViewProviderJtPart::getElement(const SoDetail* detail) const
{
    if (detail) {
        if (detail->getTypeId() == SoLineDetail::getClassTypeId()) {
            const SoLineDetail* line_detail = static_cast<const SoLineDetail*>(detail);
            int edge = line_detail->getLineIndex();
            if (edge == 0)
            {
                return std::string("Main");
            }
        }
    }

    return std::string("");
}

SoDetail* ViewProviderJtPart::getDetail(const char* subelement) const
{
    SoLineDetail* detail = 0;
    std::string subelem(subelement);
    int edge = -1;

    if(subelem == "Main") edge = 0;

    if(edge >= 0) {
         detail = new SoLineDetail();
         detail->setPartIndex(edge);
    }

    return detail;
}

bool ViewProviderJtPart::isSelectable(void) const
{
    return true;
}

void JtReaderGui::ViewProviderJtPart::updateJtPartGeometry(const JtReader::JtPartHandle* newPartHandle)
{
    pcJtRootGroup->removeAllChildren();

    std::vector<SoNode*> createdNodeVec = createSoNodesFromHandle(*newPartHandle);

    for (SoNode* node : createdNodeVec)
        pcJtRootGroup->addChild(node);

}





// ----------------------------------------------------------------------------

std::vector<SoNode*> createLod(int lodLevel, const JtReader::JtPartHandle& partHandle)
{
    std::vector<SoNode*> retNodes;

    int fragment;
    std::vector<float> Points;
    std::vector<float> Normals;
    std::vector<int> Topo;
    bool hasMat;
    JtReader::JtPartHandle::JtMat mat;

    int fragments = partHandle.getLodFragmentCount(lodLevel);

    assert(fragments >= 1);
    

    partHandle.getFaces(lodLevel, 0, Points, Normals, Topo, hasMat, mat);


    //assert(hasMat == false);


    SoNormalBinding *normb = new SoNormalBinding();
    normb->value = SoNormalBinding::PER_VERTEX_INDEXED;
 
    SoNormal *norm = new SoNormal();

    SoCoordinate3  *coords = new SoCoordinate3();

    SoIndexedFaceSet *faceset = new SoIndexedFaceSet;
 
    // create memory for the nodes and indexes
    coords->point.setNum(Points.size()/3);
    norm->vector.setNum(Normals.size()/3);
    faceset->coordIndex.setNum(Topo.size() + Topo.size()/3);
    // get the raw memory for fast fill up
    SbVec3f* verts = coords->point.startEditing();
    SbVec3f* norms = norm->vector.startEditing();
    int32_t* index = faceset->coordIndex.startEditing();
 
    // preset the normal vector with null vector
    for (int i = 0; i < Normals.size(); i += 3)
        norms[i / 3] = SbVec3f(Normals[i], Normals[i + 1], Normals[i + 2]);

    for (int i = 0; i < Points.size(); i += 3)
        verts[i / 3] = SbVec3f(Points[i], Points[i + 1], Points[i + 2]);

    for (int i = 0, l = 0; i < Topo.size(); i += 3, l += 4){
        index[l] = Topo[i];
        index[l+1] = Topo[i+1];
        index[l+2] = Topo[i+2];
        index[l+3] = -1;
    }
        

    // end the editing of the nodes
    coords->point.finishEditing();
    norm->vector.finishEditing();
    faceset->coordIndex.finishEditing();

    retNodes.push_back(normb);
    retNodes.push_back(norm);
    retNodes.push_back(coords);
    retNodes.push_back(faceset);


    return retNodes;
}

std::vector<SoNode*> createSoNodesFromHandle(const JtReader::JtPartHandle& partHandle)
{
    std::vector<SoNode*> retNodes;

    if (partHandle.getLodCount() >= 1)
        return createLod(0, partHandle);



    return retNodes;
}

