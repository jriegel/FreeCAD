/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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

#include "ViewProviderDatumShapeBinder.h"
#include "TaskDatumParameters.h"
#include "Workbench.h"
#include <Mod/PartDesign/App/DatumShape.h>
#include <Mod/Part/Gui/SoBrepFaceSet.h>
#include <Mod/Part/Gui/SoBrepEdgeSet.h>
#include <Mod/Part/Gui/SoBrepPointSet.h>
#include <Gui/Control.h>
#include <Gui/Command.h>
#include <Gui/Application.h>
#include <Mod/PartDesign/App/Body.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Mod/PartDesign/App/DatumShape.h>

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderDatumShapeBinder,PartGui::ViewProviderPart)

ViewProviderDatumShapeBinder::ViewProviderDatumShapeBinder()
{
    sPixmap = "PartDesign_ShapeBinder.svg";
    
    //make the viewprovider more datum like
    AngularDeflection.StatusBits.set(3, true);
    Deviation.StatusBits.set(3, true);
    DrawStyle.StatusBits.set(3, true);
    Lighting.StatusBits.set(3, true);
    LineColor.StatusBits.set(3, true);
    LineWidth.StatusBits.set(3, true);
    PointColor.StatusBits.set(3, true);
    PointSize.StatusBits.set(3, true);
    ShapeColor.StatusBits.set(3, true);
    Transparency.StatusBits.set(3, true);
    
    //get the datum coloring sheme
    ShapeColor.setValue(App::Color(0.9f, 0.9f, 0.13f, 0.5f));
    LineColor.setValue(App::Color(0.9f, 0.9f, 0.13f, 0.5f));
    PointColor.setValue(App::Color(0.9f, 0.9f, 0.13f, 0.5f));
    LineWidth.setValue(1);
}
ViewProviderDatumShapeBinder::~ViewProviderDatumShapeBinder()
{

}

bool ViewProviderDatumShapeBinder::setEdit(int ModNum) {
    return true;//PartGui::ViewProviderPartExt::setEdit(ModNum);
}

void ViewProviderDatumShapeBinder::unsetEdit(int ModNum) {
    return;//PartGui::ViewProviderPartExt::unsetEdit(ModNum);
}
