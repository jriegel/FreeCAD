/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <boost/bind.hpp>
#endif

#include "Workbench.h"
#include <App/Application.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/MenuManager.h>
#include <Gui/ToolBarManager.h>
#include <Gui/Control.h>

#include <Mod/Sketcher/Gui/Workbench.h>
#include <Mod/Part/App/Part2DObject.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/Feature.h>

using namespace PartDesignGui;

#if 0 // needed for Qt's lupdate utility
    qApp->translate("Workbench", "Part Design");
    qApp->translate("Gui::TaskView::TaskWatcherCommands", "Face tools");
    qApp->translate("Gui::TaskView::TaskWatcherCommands", "Sketch tools");
    qApp->translate("Gui::TaskView::TaskWatcherCommands", "Create Geometry");
#endif

namespace PartDesignGui {
//===========================================================================
// Helper for Body
//===========================================================================

PartDesign::Body *getBody(void)
{
    if(!PartDesignGui::ActivePartObject){
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("No active Body"),
            QObject::tr("In order to use PartDesign you need an active Body object in the document. "
                        "Please make one active or create one. If you have a legacy document "
                        "with PartDesign objects without Body, use the transfer function in "
                        "PartDesign to put them into a Body."
                        ));
    }
    return PartDesignGui::ActivePartObject;

}

}

/// @namespace PartDesignGui @class Workbench
TYPESYSTEM_SOURCE(PartDesignGui::Workbench, Gui::StdWorkbench)

Workbench::Workbench()
{
}

Workbench::~Workbench()
{
}

void switchToDocument(const App::Document* doc)
{
    if (doc == NULL) return;

    PartDesign::Body* activeBody = NULL;
    std::vector<App::DocumentObject*> bodies = doc->getObjectsOfType(PartDesign::Body::getClassTypeId());

    for (std::vector<App::DocumentObject*>::const_iterator b = bodies.begin(); b != bodies.end(); b++) {
        PartDesign::Body* body = static_cast<PartDesign::Body*>(*b);
        if (body->IsActive.getValue()) {
            activeBody = body;
            break;
        }
    }

    // If there is only one body, make it active
    if ((activeBody == NULL) && (bodies.size() == 1))
        activeBody = static_cast<PartDesign::Body*>(bodies.front());

    if (activeBody != NULL) {
        Gui::Command::doCommand(Gui::Command::Doc,"import PartDesignGui");
        Gui::Command::doCommand(Gui::Command::Gui,"PartDesignGui.setActivePart(App.activeDocument().%s)", activeBody->getNameInDocument());
    }
}

void Workbench::slotActiveDocument(const Gui::Document& Doc)
{
    switchToDocument(Doc.getDocument());
}

void Workbench::slotNewDocument(const App::Document& Doc)
{
    Gui::Command::runCommand(Gui::Command::Doc, "FreeCADGui.runCommand('PartDesign_Body')");
    switchToDocument(&Doc);
}

void Workbench::slotFinishRestoreDocument(const App::Document& Doc)
{
    switchToDocument(&Doc);
}

void Workbench::slotDeleteDocument(const App::Document&)
{
    ActivePartObject = 0;
    ActiveGuiDoc = 0;
    ActiveAppDoc = 0;
    ActiveVp = 0;
}

void Workbench::setupContextMenu(const char* recipient, Gui::MenuItem* item) const
{
    if (strcmp(recipient,"Tree") == 0)
    {
        if (Gui::Selection().countObjectsOfType(PartDesign::Feature::getClassTypeId()) +
            Gui::Selection().countObjectsOfType(Part::Part2DObject::getClassTypeId()) > 0 )
            *item << "PartDesign_MoveTip";
    }
}

void Workbench::activated()
{
    Gui::Workbench::activated();


    std::vector<Gui::TaskView::TaskWatcher*> Watcher;

    const char* Vertex[] = {
        "PartDesign_Plane",
        "PartDesign_Line",
        "PartDesign_Point",
        0};
    Watcher.push_back(new Gui::TaskView::TaskWatcherCommands(
        "SELECT Part::Feature SUBELEMENT Vertex COUNT 1..",
        Vertex,
        "Vertex tools",
        "Part_Box"
    ));
 
    const char* Edge[] = {
        "PartDesign_Fillet",
        "PartDesign_Chamfer",
        "PartDesign_Plane",
        "PartDesign_Line",
        "PartDesign_Point",
        0};
    Watcher.push_back(new Gui::TaskView::TaskWatcherCommands(
        "SELECT Part::Feature SUBELEMENT Edge COUNT 1..",
        Edge,
        "Edge tools",
        "Part_Box"
    ));

    const char* Face[] = {
        "PartDesign_NewSketch",        
        "PartDesign_Fillet",
        "PartDesign_Chamfer",
        "PartDesign_Draft",
        "PartDesign_Plane",
        "PartDesign_Line",
        "PartDesign_Point",
        0};
    Watcher.push_back(new Gui::TaskView::TaskWatcherCommands(
        "SELECT Part::Feature SUBELEMENT Face COUNT 1",
        Face,
        "Face tools",
        "Part_Box"
    ));

    const char* Body[] = {
        "PartDesign_NewSketch",
        0};
    Watcher.push_back(new Gui::TaskView::TaskWatcherCommands(
        "SELECT PartDesign::Body COUNT 1",
        Body,
        "Start Body",
        "Part_Box"
    ));

    const char* Plane1[] = {
        "PartDesign_NewSketch",
        "PartDesign_Plane",
        "PartDesign_Line",
        "PartDesign_Point",
        0};
    Watcher.push_back(new Gui::TaskView::TaskWatcherCommands(
        "SELECT App::Plane COUNT 1",
        Plane1,
        "Start Part",
        "Part_Box"
    ));
    const char* Plane2[] = {
        "PartDesign_NewSketch",
        "PartDesign_Plane",
        "PartDesign_Line",
        "PartDesign_Point",
        0};
    Watcher.push_back(new Gui::TaskView::TaskWatcherCommands(
        "SELECT PartDesign::Plane COUNT 1",
        Plane2,
        "Start Part",
        "Part_Box"
    ));

    const char* Line[] = {
        "PartDesign_Plane",
        "PartDesign_Line",
        "PartDesign_Point",
        0};
    Watcher.push_back(new Gui::TaskView::TaskWatcherCommands(
        "SELECT PartDesign::Line COUNT 1",
        Line,
        "Start Part",
        "Part_Box"
    ));

    const char* Point[] = {
        "PartDesign_Plane",
        "PartDesign_Line",
        "PartDesign_Point",
        0};
    Watcher.push_back(new Gui::TaskView::TaskWatcherCommands(
        "SELECT PartDesign::Point COUNT 1",
        Point,
        "Start Part",
        "Part_Box"
    ));

    const char* NoSel[] = {
        "PartDesign_Body",
        0};
    Watcher.push_back(new Gui::TaskView::TaskWatcherCommandsEmptySelection(
        NoSel,
        "Start Part",
        "Part_Box"
    ));

    const char* Faces[] = {
        "PartDesign_Fillet",
        "PartDesign_Chamfer",
        "PartDesign_Draft",
        0};
    Watcher.push_back(new Gui::TaskView::TaskWatcherCommands(
        "SELECT Part::Feature SUBELEMENT Face COUNT 2..",
        Faces,
        "Face tools",
        "Part_Box"
    ));

    const char* Sketch[] = {
        "PartDesign_NewSketch",
        "PartDesign_Pad",
        "PartDesign_Pocket",
        "PartDesign_Revolution",
        "PartDesign_Groove",
        0};
    Watcher.push_back(new Gui::TaskView::TaskWatcherCommands(
        "SELECT Sketcher::SketchObject COUNT 1",
        Sketch,
        "Sketch tools",
        "Part_Box"
    ));

    const char* Transformed[] = {
        "PartDesign_Mirrored",
        "PartDesign_LinearPattern",
        "PartDesign_PolarPattern",
//        "PartDesign_Scaled",
        "PartDesign_MultiTransform",
        0};
    Watcher.push_back(new Gui::TaskView::TaskWatcherCommands(
        "SELECT PartDesign::SketchBased",
        Transformed,
        "Transformation tools",
        "PartDesign_MultiTransform"
    ));

    // make the previously used active Body active again
    PartDesignGui::ActivePartObject = NULL;
    switchToDocument(App::GetApplication().getActiveDocument());

    addTaskWatcher(Watcher);
    Gui::Control().showTaskView();

    // Let us be notified when a document is activated, so that we can update the ActivePartObject
    Gui::Application::Instance->signalActiveDocument.connect(boost::bind(&Workbench::slotActiveDocument, this, _1));
    App::GetApplication().signalNewDocument.connect(boost::bind(&Workbench::slotNewDocument, this, _1));
    App::GetApplication().signalFinishRestoreDocument.connect(boost::bind(&Workbench::slotFinishRestoreDocument, this, _1));
    App::GetApplication().signalDeleteDocument.connect(boost::bind(&Workbench::slotDeleteDocument, this, _1));
}

void Workbench::deactivated()
{
    // Let us be notified when a document is activated, so that we can update the ActivePartObject
    Gui::Application::Instance->signalActiveDocument.disconnect(boost::bind(&Workbench::slotActiveDocument, this, _1));
    App::GetApplication().signalNewDocument.disconnect(boost::bind(&Workbench::slotNewDocument, this, _1));
    App::GetApplication().signalFinishRestoreDocument.disconnect(boost::bind(&Workbench::slotFinishRestoreDocument, this, _1));
    App::GetApplication().signalDeleteDocument.disconnect(boost::bind(&Workbench::slotDeleteDocument, this, _1));

    removeTaskWatcher();
    // reset the active Body
    Gui::Command::doCommand(Gui::Command::Doc,"import PartDesignGui");
    Gui::Command::doCommand(Gui::Command::Doc,"PartDesignGui.setActivePart(None)");

    Gui::Workbench::deactivated();

}

Gui::MenuItem* Workbench::setupMenuBar() const
{
    Gui::MenuItem* root = StdWorkbench::setupMenuBar();
    Gui::MenuItem* item = root->findItem("&Windows");

    Gui::MenuItem* geom = new Gui::MenuItem();
    geom->setCommand("Sketcher geometries");
    SketcherGui::addSketcherWorkbenchGeometries( *geom );

    Gui::MenuItem* cons = new Gui::MenuItem();
    cons->setCommand("Sketcher constraints");
    SketcherGui::addSketcherWorkbenchConstraints( *cons );
    
    Gui::MenuItem* consaccel = new Gui::MenuItem();
    consaccel->setCommand("Sketcher tools");
    SketcherGui::addSketcherWorkbenchTools(*consaccel);

    Gui::MenuItem* part = new Gui::MenuItem;
    root->insertItem(item, part);
    part->setCommand("&Part Design");
    SketcherGui::addSketcherWorkbenchSketchActions( *part );
    *part << "PartDesign_Body"
          << "PartDesign_NewSketch"
          << "Sketcher_LeaveSketch"
          << "Sketcher_ViewSketch"
          << "Sketcher_MapSketch"
          << "Sketcher_ReorientSketch"
          << geom
          << cons
          << "Separator"
          << "PartDesign_Plane"
          << "PartDesign_Line"
          << "PartDesign_Point"
          << "Separator"
          << "PartDesign_Pad"
          << "PartDesign_Pocket"
          << "PartDesign_Revolution"
          << "PartDesign_Groove"
          << "PartDesign_Fillet"
          << "PartDesign_Chamfer"
          << "PartDesign_Draft"
          << "PartDesign_Mirrored"
          << "PartDesign_LinearPattern"
          << "PartDesign_PolarPattern"
//          << "PartDesign_Scaled"
          << "PartDesign_MultiTransform";
    // For 0.13 a couple of python packages like numpy, matplotlib and others
    // are not deployed with the installer on Windows. Thus, the WizardShaft is
    // not deployed either hence the check for the existence of the command.
    if (Gui::Application::Instance->commandManager().getCommandByName("PartDesign_InvoluteGear")) {
        *part << "PartDesign_InvoluteGear";
    }
    if (Gui::Application::Instance->commandManager().getCommandByName("PartDesign_WizardShaft")) {
        *part << "Separator" << "PartDesign_WizardShaft";
    }

    return root;
}

Gui::ToolBarItem* Workbench::setupToolBars() const
{
    Gui::ToolBarItem* root = StdWorkbench::setupToolBars();
    Gui::ToolBarItem* part = new Gui::ToolBarItem(root);
    part->setCommand("Part Design");
//    SketcherGui::addSketcherWorkbenchSketchActions( *part );
    *part << "PartDesign_Body"
          << "PartDesign_NewSketch"
          << "Sketcher_ViewSketch"
          << "Sketcher_MapSketch"
          << "Sketcher_LeaveSketch"
          << "Separator"
          << "PartDesign_Plane"
          << "PartDesign_Line"
          << "PartDesign_Point"
          << "Separator"
          << "PartDesign_Pad"
          << "PartDesign_Pocket"
          << "PartDesign_Revolution"
          << "PartDesign_Groove"
          << "PartDesign_Fillet"
          << "PartDesign_Chamfer"
          << "PartDesign_Draft"
          << "PartDesign_Mirrored"
          << "PartDesign_LinearPattern"
          << "PartDesign_PolarPattern"
//          << "PartDesign_Scaled"
          << "PartDesign_MultiTransform";

    Gui::ToolBarItem* geom = new Gui::ToolBarItem(root);
    geom->setCommand("Sketcher geometries");
    SketcherGui::addSketcherWorkbenchGeometries( *geom );

    Gui::ToolBarItem* cons = new Gui::ToolBarItem(root);
    cons->setCommand("Sketcher constraints");
    SketcherGui::addSketcherWorkbenchConstraints( *cons );

    Gui::ToolBarItem* consaccel = new Gui::ToolBarItem(root);
    consaccel->setCommand("Sketcher tools");
    SketcherGui::addSketcherWorkbenchTools( *consaccel );
    
    return root;
}

Gui::ToolBarItem* Workbench::setupCommandBars() const
{
    // Part tools
    Gui::ToolBarItem* root = new Gui::ToolBarItem;
    return root;
}

