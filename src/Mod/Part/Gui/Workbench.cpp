/***************************************************************************
 *   Copyright (c) 2005 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <qobject.h>
#include <QMessageBox>
#endif

#include "Workbench.h"
#include <Gui/MenuManager.h>
#include <Gui/ToolBarManager.h>
#include <Gui/MainWindow.h>
#include <Gui/Application.h>
#include <Gui/MDIView.h>

namespace PartGui {
    

App::Part *getPart(bool messageIfNot)
{
    App::Part * activePart = Gui::Application::Instance->activeView()->getActiveObject<App::Part*>(PARTKEY);

    if (!activePart && messageIfNot){
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("No active Part"),
            QObject::tr("In order to use Part you need an active Part object in the document. "
                        "Please make one active (double click) or create one. If you have a legacy document "
                        "with Part objects without Body, use the transfer function in "
                        "Part to put them into a Part."
                        ));
    }
    return activePart;
}

App::Part* getPartFor(App::DocumentObject* obj, bool messageIfNot) {

    if(!obj)
        return nullptr;

    //get the part the object belongs to
    for(App::Part* p : obj->getDocument()->getObjectsOfType<App::Part>()) {
        if(p->hasObject(obj)) {
            return p;
        }
    }

    if (messageIfNot){
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Feature is not in a part"),
            QObject::tr("In order to use this feature it needs to belong to a part object in the document."));
    }

    return nullptr;
}

}


using namespace PartGui;

#if 0 // needed for Qt's lupdate utility
    qApp->translate("Workbench", "&Part");
    qApp->translate("Workbench", "&Simple");
    qApp->translate("Workbench", "&Parametric");
    qApp->translate("Workbench", "Solids");
    qApp->translate("Workbench", "Part tools");
    qApp->translate("Workbench", "Boolean");
#endif

/// @namespace PartGui @class Workbench
TYPESYSTEM_SOURCE(PartGui::Workbench, Gui::StdWorkbench)

Workbench::Workbench()
{
}

Workbench::~Workbench()
{
}

Gui::MenuItem* Workbench::setupMenuBar() const
{
    Gui::MenuItem* root = StdWorkbench::setupMenuBar();
    Gui::MenuItem* item = root->findItem("&Windows");

    Gui::MenuItem* prim = new Gui::MenuItem;
    prim->setCommand("Primitives");
    *prim << "Part_Box" << "Part_Cylinder" << "Part_Sphere"
          << "Part_Cone" << "Part_Torus";

    Gui::MenuItem* bop = new Gui::MenuItem;
    bop->setCommand("Boolean");
    *bop << "Part_Boolean" << "Part_Cut" << "Part_Fuse" << "Part_Common";

    Gui::MenuItem* join = new Gui::MenuItem;
    join->setCommand("Join");
    *join << "Part_JoinConnect" << "Part_JoinEmbed" << "Part_JoinCutout";

    Gui::MenuItem* part = new Gui::MenuItem;
    root->insertItem(item, part);
    part->setCommand("&Part");
    *part << "Part_Part" << "Part_Import" << "Part_Export" << "Separator";
    *part << prim << "Part_Primitives" << "Part_Builder" << "Separator"
          << "Part_ShapeFromMesh" << "Part_MakeSolid" << "Part_ReverseShape"
          << "Part_SimpleCopy" << "Part_RefineShape" << "Part_CheckGeometry"
          << "Separator" << bop << join << "Separator"
          << "Part_CrossSections" << "Part_Compound" << "Part_Extrude"
          << "Part_Revolve" << "Part_Mirror" << "Part_Fillet" << "Part_Chamfer"
          << "Part_RuledSurface" << "Part_Loft" << "Part_Sweep"
          << "Part_Offset" << "Part_Thickness";

    Gui::MenuItem* measure = new Gui::MenuItem;
    root->insertItem(item,measure);
    measure->setCommand("Measure");
    *measure << "Part_Measure_Linear" << "Part_Measure_Angular" << "Separator" << "Part_Measure_Clear_All" << "Part_Measure_Toggle_All" <<
      "Part_Measure_Toggle_3d" << "Part_Measure_Toggle_Delta";

    // leave this for 0.14 until #0000477 is fixed
#if 0
    Gui::MenuItem* view = root->findItem("&View");
    if (view) {
        Gui::MenuItem* appr = view->findItem("Std_RandomColor");
        appr = view->afterItem(appr);
        Gui::MenuItem* face = new Gui::MenuItem();
        face->setCommand("Part_ColorPerFace");
        view->insertItem(appr, face);
    }
#endif

    return root;
}

Gui::ToolBarItem* Workbench::setupToolBars() const
{
    Gui::ToolBarItem* root = StdWorkbench::setupToolBars();

    Gui::ToolBarItem* solids = new Gui::ToolBarItem(root);
    solids->setCommand("Solids");
    *solids << "Part_Box" << "Part_Cylinder" << "Part_Sphere" << "Part_Cone"
            << "Part_Torus" << "Part_Primitives" << "Part_Builder";

    Gui::ToolBarItem* tool = new Gui::ToolBarItem(root);
    tool->setCommand("Part tools");
    *tool << "Part_Part"
          << "Part_Extrude" << "Part_Revolve" << "Part_Mirror" << "Part_Fillet"
          << "Part_Chamfer" << "Part_RuledSurface" << "Part_Loft" << "Part_Sweep"
          << "Part_Offset" << "Part_Thickness";

    Gui::ToolBarItem* boolop = new Gui::ToolBarItem(root);
    boolop->setCommand("Boolean");
    *boolop << "Part_Boolean" << "Part_Cut" << "Part_Fuse" << "Part_Common"
            << "Part_CompJoinFeatures" << "Part_CheckGeometry" << "Part_Section"
            << "Part_CrossSections";
	     
    Gui::ToolBarItem* measure = new Gui::ToolBarItem(root);
    measure->setCommand("Measure");
    *measure << "Part_Measure_Linear" << "Part_Measure_Angular"  << "Separator" << "Part_Measure_Clear_All" << "Part_Measure_Toggle_All"
             << "Part_Measure_Toggle_3d" << "Part_Measure_Toggle_Delta";

    return root;
}

Gui::ToolBarItem* Workbench::setupCommandBars() const
{
    // Part tools
    Gui::ToolBarItem* root = new Gui::ToolBarItem;
    return root;
}
