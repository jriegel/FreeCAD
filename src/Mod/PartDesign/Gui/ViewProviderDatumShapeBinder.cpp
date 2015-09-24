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
#include "TaskDatumShapeBinder.h"
#include <Mod/PartDesign/App/DatumShape.h>
#include <Mod/Part/Gui/SoBrepFaceSet.h>
#include <Mod/Part/Gui/SoBrepEdgeSet.h>
#include <Mod/Part/Gui/SoBrepPointSet.h>
#include <Gui/Control.h>
#include <Gui/Command.h>
#include <Gui/Application.h>
#include <Mod/PartDesign/App/Body.h>
#include <Inventor/nodes/SoSeparator.h>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <QMessageBox>
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
    
    
    if (ModNum == ViewProvider::Default || ModNum == 1 ) {
        
        // When double-clicking on the item for this pad the
        // object unsets and sets its edit mode without closing
        // the task panel
        Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
        TaskDlgDatumShapeBinder *sbDlg = qobject_cast<TaskDlgDatumShapeBinder*>(dlg);
        if (sbDlg)
            sbDlg = 0; // another pad left open its task panel
        if (dlg && !sbDlg) {
            QMessageBox msgBox;
            msgBox.setText(QObject::tr("A dialog is already open in the task panel"));
            msgBox.setInformativeText(QObject::tr("Do you want to close this dialog?"));
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            msgBox.setDefaultButton(QMessageBox::Yes);
            int ret = msgBox.exec();
            if (ret == QMessageBox::Yes)
                Gui::Control().reject();
            else
                return false;
        }

        // clear the selection (convenience)
        Gui::Selection().clearSelection();

        // start the edit dialog
        if (sbDlg)
            Gui::Control().showDialog(sbDlg);
        else
            Gui::Control().showDialog(new TaskDlgDatumShapeBinder(this,ModNum == 1));

        return true;
    }
    else {
        return ViewProviderPart::setEdit(ModNum);
    }
}

void ViewProviderDatumShapeBinder::unsetEdit(int ModNum) {
    
    PartGui::ViewProviderPart::unsetEdit(ModNum);
}

void ViewProviderDatumShapeBinder::highlightReferences(const bool on, bool auxillery) {
    
    Part::Feature* obj;
    std::vector<std::string> subs;
    
    if(getObject()->isDerivedFrom(PartDesign::ShapeBinder::getClassTypeId()))
        PartDesign::ShapeBinder::getFilterdReferences(&static_cast<PartDesign::ShapeBinder*>(getObject())->Support, obj, subs);
    else if(getObject()->isDerivedFrom(PartDesign::ShapeBinder2D::getClassTypeId()))
        PartDesign::ShapeBinder::getFilterdReferences(&static_cast<PartDesign::ShapeBinder2D*>(getObject())->Support, obj, subs);
    else 
        return;
        
    PartGui::ViewProviderPart* svp = dynamic_cast<PartGui::ViewProviderPart*>(
                Gui::Application::Instance->getViewProvider(obj));
    if (svp == NULL) return;

    if (on) {        
         if (!subs.empty() && originalLineColors.empty()) {
            TopTools_IndexedMapOfShape eMap;
            TopExp::MapShapes(obj->Shape.getValue(), TopAbs_EDGE, eMap);
            originalLineColors = svp->LineColorArray.getValues();
            std::vector<App::Color> lcolors = originalLineColors;
            lcolors.resize(eMap.Extent(), svp->LineColor.getValue());
            
            TopExp::MapShapes(obj->Shape.getValue(), TopAbs_FACE, eMap);
            originalFaceColors = svp->DiffuseColor.getValues();
            std::vector<App::Color> fcolors = originalFaceColors;
            fcolors.resize(eMap.Extent(), svp->ShapeColor.getValue());

            for (std::string e : subs) {
                if(e.substr(4) == "Edge") {
                    
                    int idx = atoi(e.substr(4).c_str()) - 1;
                    if (idx < lcolors.size())
                        lcolors[idx] = App::Color(1.0,0.0,1.0); // magenta
                }
                else if(e.substr(4) == "Face")  {
                    
                    int idx = atoi(e.substr(4).c_str()) - 1;
                    if (idx < fcolors.size())
                        fcolors[idx] = App::Color(1.0,0.0,1.0); // magenta
                }
            }
            svp->LineColorArray.setValues(lcolors);
            svp->DiffuseColor.setValues(fcolors);
        }
    } else {
        if (!subs.empty() && !originalLineColors.empty()) {
            svp->LineColorArray.setValues(originalLineColors);
            originalLineColors.clear();
            
            svp->DiffuseColor.setValues(originalFaceColors);
            originalFaceColors.clear();
        }
    }
}
