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
# include <Precision.hxx>
# include <gp_Pnt.hxx>
# include <gp_Dir.hxx>
# include <gp_Pln.hxx>
#endif

#include "Workbench.h"
#include <App/Plane.h>
#include <App/Part.h>
#include <App/Placement.h>
#include <App/Application.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/MenuManager.h>
#include <Gui/ToolBarManager.h>
#include <Gui/Control.h>
#include <Gui/DlgCheckableMessageBox.h>
#include <Gui/ViewProviderPart.h>
#include <Gui/ActiveObjectList.h>


#include <Mod/Sketcher/Gui/Workbench.h>
#include <Mod/Part/App/Part2DObject.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/Feature.h>
#include <Mod/PartDesign/App/FeatureSketchBased.h>
#include <Mod/PartDesign/App/FeatureMultiTransform.h>
#include <Mod/Part/App/DatumFeature.h>
#include <Mod/Sketcher/App/SketchObject.h>

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

PartDesign::Body *getBody(bool messageIfNot)
{
    PartDesign::Body * activeBody = Gui::Application::Instance->activeView()->getActiveObject<PartDesign::Body*>(PDBODYKEY);

    if (!activeBody && messageIfNot){
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("No active Body"),
            QObject::tr("In order to use PartDesign you need an active Body object in the document. "
                        "Please make one active (double click) or create one. If you have a legacy document "
                        "with PartDesign objects without Body, use the transfer function in "
                        "PartDesign to put them into a Body."
                        ));
    }
    return activeBody;

}

PartDesign::Body *getBodyFor(App::DocumentObject* obj, bool messageIfNot)
{
    if(!obj)
        return nullptr;

    PartDesign::Body * activeBody = Gui::Application::Instance->activeView()->getActiveObject<PartDesign::Body*>(PDBODYKEY);
    if(activeBody && activeBody->hasFeature(obj))
        return activeBody;

    //try to find the part the object is in
    for(PartDesign::Body* b : obj->getDocument()->getObjectsOfType<PartDesign::Body>()) {
        if(b->hasFeature(obj)) {
            return b;
        }
    }

    if (messageIfNot){
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Feature is not in a body"),
            QObject::tr("In order to use this feature it needs to belong to a body object in the document."));
    }

    return nullptr;
}

App::Part* getPartFor(App::DocumentObject* obj, bool messageIfNot) {

    if(!obj)
        return nullptr;

    PartDesign::Body* body = getBodyFor(obj, false);
    if(body)
        obj = body;

    //get the part every body should belong to
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

/// @namespace PartDesignGui @class Workbench
TYPESYSTEM_SOURCE(PartDesignGui::Workbench, Gui::StdWorkbench)

Workbench::Workbench()
{
}

Workbench::~Workbench()
{
}

static void buildDefaultPartAndBody(const App::Document* doc)
{
  // This adds both the base planes and the body
    std::string PartName = doc->getUniqueObjectName("Part");
    //// create a PartDesign Part for now, can be later any kind of Part or an empty one
    Gui::Command::addModule(Gui::Command::Doc, "PartDesignGui");
    Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().Tip = App.activeDocument().addObject('App::Part','%s')", PartName.c_str());
    Gui::Command::doCommand(Gui::Command::Doc, "PartDesignGui.setUpPart(App.activeDocument().%s)", PartName.c_str());
    Gui::Command::doCommand(Gui::Command::Gui, "Gui.activeView().setActiveObject('Part',App.activeDocument().%s)", PartName.c_str());
}

PartDesign::Body *Workbench::setUpPart(const App::Part *part)
{
    // first do the general Part setup
    Gui::ViewProviderPart::setUpPart(part);

    // check for Bodies
    std::vector<App::DocumentObject*> bodies = part->getObjectsOfType(PartDesign::Body::getClassTypeId());
    assert(bodies.size() == 0);

    std::string PartName = part->getNameInDocument();
    std::string BodyName = part->getDocument()->getUniqueObjectName("MainBody");

    Gui::Command::addModule(Gui::Command::Doc, "PartDesign");
    Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().addObject('PartDesign::Body','%s')", BodyName.c_str());
    Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.addObject(App.activeDocument().ActiveObject)", part->getNameInDocument());
    Gui::Command::doCommand(Gui::Command::Gui, "Gui.activeView().setActiveObject('%s', App.activeDocument().%s)", PDBODYKEY, BodyName.c_str());
    Gui::Command::updateActive();

    return NULL;
}

void Workbench::_doMigration(const App::Document* doc)
{/*
    bool groupCreated = false;

    if(doc->countObjects() != 0) {
         // show a warning about the convertion
         Gui::Dialog::DlgCheckableMessageBox::showMessage(
            QString::fromLatin1("PartDesign conversion warning"),
            QString::fromLatin1(
            "<h2>Converting PartDesign features to new Body centric schema</h2>"
            "If you are unsure what that mean save the document under a new name.<br>"
            "You will not be able to load your work in an older Version of FreeCAD,<br>"
            "After the translation took place...<br><br>"
            "More information you will find here:<br>"
            " <a href=\"http://www.freecadweb.org/wiki/index.php?title=Assembly_project\">http://www.freecadweb.org/wiki/index.php?title=Assembly_project</a> <br>"
            "Or the Assembly dedicated portion of our forum:<br>"
            " <a href=\"http://forum.freecadweb.org/viewforum.php?f=20&sid=2a1a326251c44576f450739e4a74c37d\">http://forum.freecadweb.org/</a> <br>"
                                ),
            false,
            QString::fromLatin1("Don't tell me again, I know!")
                                                    );
    }

    Gui::Command::openCommand("Migrate part to Body feature");

    // Get the objects now, before adding the Body and the base planes
    std::vector<App::DocumentObject*> features = doc->getObjects();

    // Assign all non-PartDesign features to a new group
    for (std::vector<App::DocumentObject*>::iterator f = features.begin(); f != features.end(); ) {
        if ((*f)->getTypeId().isDerivedFrom(PartDesign::Feature::getClassTypeId()) ||
            (*f)->getTypeId().isDerivedFrom(Part::Part2DObject::getClassTypeId())) {
             ++f;
        } else {
            if (!groupCreated) {
                Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().addObject('App::DocumentObjectGroup','NonBodyFeatures')");
               Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().ActiveObject.Label = '%s'",
                                        QObject::tr("NonBodyFeatures").toStdString().c_str());
                groupCreated = true;
            }
            Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().NonBodyFeatures.addObject(App.activeDocument().getObject('%s'))",
                                    (*f)->getNameInDocument());
            f = features.erase(f);
        }
    }
    // TODO: Fold the group (is that possible through the Python interface?)

    // Try to find the root(s) of the model tree (the features that depend on no other feature)
    // Note: We assume a linear graph, except for MultiTransform features
    std::vector<App::DocumentObject*> roots;
    for (std::vector<App::DocumentObject*>::iterator f = features.begin(); f != features.end(); f++) {
        // Note: The dependency list always contains at least the object itself
        std::vector<App::DocumentObject*> ftemp;
        ftemp.push_back(*f);
        if (doc->getDependencyList(ftemp).size() == 1)
            roots.push_back(*f);
    }

    // Always create at least the first body, even if the document is empty
    buildDefaultPartAndBody(doc);
    PartDesign::Body *activeBody = Gui::Application::Instance->activeView()->getActiveObject<PartDesign::Body*>(PDBODYKEY);
    assert(activeBody);

    // Create one Body for every root and put the appropriate features into it
    for (std::vector<App::DocumentObject*>::iterator r = roots.begin(); r != roots.end(); r++) {
        if (r != roots.begin()) {
            Gui::Command::runCommand(Gui::Command::Doc, "FreeCADGui.runCommand('PartDesign_Body')");
            activeBody = Gui::Application::Instance->activeView()->getActiveObject<PartDesign::Body*>(PDBODYKEY);
        }

        std::set<App::DocumentObject*> inList;
        inList.insert(*r); // start with the root feature
        std::vector<App::DocumentObject*> bodyFeatures;
        std::string modelString = "";
        do {
            for (std::set<App::DocumentObject*>::const_iterator o = inList.begin(); o != inList.end(); o++) {
                std::vector<App::DocumentObject*>::iterator feat = std::find(features.begin(), features.end(), *o);
                if (feat != features.end()) {
                    bodyFeatures.push_back(*o);
                    modelString += std::string(modelString.empty() ? "" : ",") + "App.ActiveDocument." + (*o)->getNameInDocument();
                    features.erase(feat);
                } else {
                    QMessageBox::critical(Gui::getMainWindow(), QObject::tr("Non-linear tree"),
                        QObject::tr("Please look at '%1' and make sure that the migration result is what you"
                                    " would expect.").arg(QString::fromAscii((*o)->getNameInDocument())));
                }
            }
            std::set<App::DocumentObject*> newInList;
            for (std::set<App::DocumentObject*>::const_iterator o = inList.begin(); o != inList.end(); o++) {
                // Omit members of a MultiTransform from the inList, to avoid migration errors
                if (PartDesign::Body::isMemberOfMultiTransform(*o))
                    continue;
                std::vector<App::DocumentObject*> iL = doc->getInList(*o);
                newInList.insert(iL.begin(), iL.end());
            }
            inList = newInList; // TODO: Memory leak? Unnecessary copying?
        } while (!inList.empty());

        if (!modelString.empty()) {
            modelString = std::string("[") + modelString + "]";
            Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().%s.Model = %s", activeBody->getNameInDocument(), modelString.c_str());
            // Set the Tip, but not to a member of a MultiTransform!
            for (std::vector<App::DocumentObject*>::const_reverse_iterator f = bodyFeatures.rbegin(); f != bodyFeatures.rend(); f++) {
                if (PartDesign::Body::isMemberOfMultiTransform(*f))
                    continue;
                Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().%s.Tip = App.activeDocument().%s",
                                        activeBody->getNameInDocument(), (*f)->getNameInDocument());
                break;
            }
        }

        // Initialize the BaseFeature property of all PartDesign solid features
        App::DocumentObject* baseFeature = NULL;
        for (std::vector<App::DocumentObject*>::const_iterator f = bodyFeatures.begin(); f != bodyFeatures.end(); f++) {
            if (PartDesign::Body::isSolidFeature(*f)) {
                Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().%s.BaseFeature = %s",
                                        (*f)->getNameInDocument(),
                                        baseFeature == NULL ?
                                            "None" :
                                            (std::string("App.activeDocument().") + baseFeature->getNameInDocument()).c_str());

                baseFeature = *f;
            }
        }

        // Re-route all sketches without support to the base planes
        std::vector<App::DocumentObject*>::const_iterator prevf;

        for (std::vector<App::DocumentObject*>::const_iterator f = bodyFeatures.begin(); f != bodyFeatures.end(); f++) {
            if ((*f)->getTypeId().isDerivedFrom(Sketcher::SketchObject::getClassTypeId())) {
                Sketcher::SketchObject *sketch = static_cast<Sketcher::SketchObject*>(*f);
                try {
                    fixSketchSupport(sketch);
                } catch (Base::Exception &) {
                    QMessageBox::critical(Gui::getMainWindow(), QObject::tr("Sketch plane cannot be migrated"),
                        QObject::tr("Please edit '%1' and redefine it to use a Base or Datum plane as the sketch plane.").
                            arg(QString::fromAscii(sketch->getNameInDocument()) ) );
                }
            }

            prevf = f;
        }
    }*/

}


void Workbench::fixSketchSupport (Sketcher::SketchObject* sketch)
{
    App::DocumentObject* support = sketch->Support.getValue();

    if (support)
        return; // Sketch is on a face of a solid, do nothing

    const App::Document* doc = sketch->getDocument();
    PartDesign::Body *body = getBodyFor(sketch, /*messageIfNot*/ 0);

    if (!body) {
        throw Base::Exception ("Coudn't find body for the sketch");
    }

    Base::Placement plm = sketch->Placement.getValue();
    Base::Vector3d pnt = plm.getPosition();

    // Currently we only handle positions that are parallel to the base planes
    Base::Rotation rot = plm.getRotation();
    Base::Vector3d sketchVector(0,0,1);
    rot.multVec(sketchVector, sketchVector);
    bool reverseSketch = (sketchVector.x + sketchVector.y + sketchVector.z) < 0.0 ;
    if (reverseSketch) sketchVector *= -1.0;
    int index;

    if (sketchVector == Base::Vector3d(0,0,1))
        index = 0;
    else if (sketchVector == Base::Vector3d(0,1,0))
        index = 1;
    else if (sketchVector == Base::Vector3d(1,0,0))
        index = 2;
    else {
        throw Base::Exception("Sketch plane cannot be migrated");
    }

    // Find the normal distance from origin to the sketch plane
    gp_Pln pln(gp_Pnt (pnt.x, pnt.y, pnt.z), gp_Dir(sketchVector.x, sketchVector.y, sketchVector.z));
    double offset = pln.Distance(gp_Pnt(0,0,0));

    if (fabs(offset) < Precision::Confusion()) {
        // One of the base planes
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().%s.Support = (App.activeDocument().%s,[''])",
                sketch->getNameInDocument(), App::Part::BaseplaneTypes[index]);
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().%s.MapReversed = %s",
                sketch->getNameInDocument(), reverseSketch ? "True" : "False");
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().%s.MapMode = '%s'",
                sketch->getNameInDocument(), Attacher::AttachEngine::eMapModeStrings[Attacher::mmFlatFace]);

    } else {
        // Offset to base plane
        // Find out which direction we need to offset
        double a = sketchVector.GetAngle(pnt);
        if ((a < -M_PI_2) || (a > M_PI_2))
            offset *= -1.0;

        std::string Datum = doc->getUniqueObjectName("DatumPlane");
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().addObject('PartDesign::Plane','%s')",Datum.c_str());
        QString refStr = QString::fromAscii("[(App.activeDocument().") + QString::fromAscii(App::Part::BaseplaneTypes[index]) +
            QString::fromAscii(",'')]");
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().%s.Support = %s",Datum.c_str(), refStr.toStdString().c_str());
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().%s.MapMode = '%s'",Datum.c_str(), AttachEngine::eMapModeStrings[Attacher::mmFlatFace]);
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().%s.superPlacement.Base.z = %f",Datum.c_str(), offset);
        Gui::Command::doCommand(Gui::Command::Doc,
                "App.activeDocument().%s.insertFeature(App.activeDocument().%s, App.activeDocument().%s)",
                body->getNameInDocument(), Datum.c_str(), sketch->getNameInDocument());
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().%s.Support = (App.activeDocument().%s,[''])",
                sketch->getNameInDocument(), Datum.c_str());
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().%s.MapReversed = %s",
                sketch->getNameInDocument(), reverseSketch ? "True" : "False");
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().%s.MapMode = '%s'",
                sketch->getNameInDocument(),Attacher::AttachEngine::eMapModeStrings[Attacher::mmFlatFace]);
        Gui::Command::doCommand(Gui::Command::Gui,"App.activeDocument().recompute()");  // recompute the feature based on its references
    }
}


void Workbench::_switchToDocument(const App::Document* doc)
{/*
    bool groupCreated = false;


    if (doc == NULL) return;

    PartDesign::Body* activeBody = NULL;
    std::vector<App::DocumentObject*> bodies = doc->getObjectsOfType(PartDesign::Body::getClassTypeId());

    // No tip, so build up structure or migrate
    if (!doc->Tip.getValue())
    {
        if (doc->countObjects() == 0){
            buildDefaultPartAndBody(doc);
            activeBody = Gui::Application::Instance->activeView()->getActiveObject<PartDesign::Body*>(PDBODYKEY);
            assert(activeBody);
        } else {
            // empty document with no tip, so do migration
            _doMigration(doc);
                        activeBody = Gui::Application::Instance->activeView()->getActiveObject<PartDesign::Body*>(PDBODYKEY);
                        assert(activeBody);
                }
    }
    else
    {
      App::Part *docPart = dynamic_cast<App::Part *>(doc->Tip.getValue());
      assert(docPart);
      App::Part *viewPart = Gui::Application::Instance->activeView()->getActiveObject<App::Part *>("Part");
      if (viewPart != docPart)
        Gui::Application::Instance->activeView()->setActiveObject(docPart, "Part");
      if (docPart->countObjectsOfType(PartDesign::Body::getClassTypeId()) < 1)
        setUpPart(docPart);

      PartDesign::Body *tempBody = dynamic_cast<PartDesign::Body *> (docPart->getObjectsOfType(PartDesign::Body::getClassTypeId()).front());
      assert(tempBody);
      PartDesign::Body *viewBody = Gui::Application::Instance->activeView()->getActiveObject<PartDesign::Body*>(PDBODYKEY);
      activeBody = viewBody;
      if (!viewBody)
        activeBody = tempBody;
      else if (!docPart->hasObject(viewBody))
        activeBody = tempBody;

      if (activeBody != viewBody)
        Gui::Application::Instance->activeView()->setActiveObject(activeBody, PDBODYKEY);
    }

    if (activeBody == NULL) {
        QMessageBox::critical(Gui::getMainWindow(), QObject::tr("Could not create body"),
            QObject::tr("No body was found in this document, and none could be created. Please report this bug."
                        "We recommend you do not use this document with the PartDesign workbench until the bug has been fixed."
                        ));
    }*/
}

void Workbench::slotActiveDocument(const Gui::Document& Doc)
{
//     _switchToDocument(Doc.getDocument());
}

void Workbench::slotNewDocument(const App::Document& Doc)
{
//     _switchToDocument(&Doc);
}

void Workbench::slotFinishRestoreDocument(const App::Document& Doc)
{
//     _switchToDocument(&Doc);
}

void Workbench::slotDeleteDocument(const App::Document&)
{
    //ActivePartObject = 0;
    //ActiveGuiDoc = 0;
    //ActiveAppDoc = 0;
    //ActiveVp = 0;
}
/*
  This does not work for Std_DuplicateSelection:
  Tree.cpp gives: "Cannot reparent unknown object", probably because the signalNewObject is emitted
  before the duplication of the object has been completely finished

void Workbench::slotNewObject(const App::DocumentObject& obj)
{
    if ((obj.getDocument() == ActiveAppDoc) && (ActivePartObject != NULL)) {
        // Add the new object to the active Body
        // Note: Will this break Undo? But how else can we catch Edit->Duplicate selection?
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().%s.addFeature(App.activeDocument().%s)",
                                ActivePartObject->getNameInDocument(), obj.getNameInDocument());
    }
}
*/

void Workbench::setupContextMenu(const char* recipient, Gui::MenuItem* item) const
{
    if (strcmp(recipient,"Tree") == 0)
    {
        if (Gui::Selection().countObjectsOfType(PartDesign::Body::getClassTypeId()) +
            Gui::Selection().countObjectsOfType(PartDesign::Feature::getClassTypeId()) +
            Gui::Selection().countObjectsOfType(Part::Datum::getClassTypeId()) +
            Gui::Selection().countObjectsOfType(Part::Part2DObject::getClassTypeId()) > 0 )
            *item << "PartDesign_MoveTip";
        if (Gui::Selection().countObjectsOfType(PartDesign::Feature::getClassTypeId()) +
            Gui::Selection().countObjectsOfType(Part::Datum::getClassTypeId()) +
            Gui::Selection().countObjectsOfType(Part::Part2DObject::getClassTypeId()) > 0 )
            *item << "PartDesign_MoveFeature"
                  << "PartDesign_MoveFeatureInTree";
        if (Gui::Selection().countObjectsOfType(PartDesign::Transformed::getClassTypeId()) -
            Gui::Selection().countObjectsOfType(PartDesign::MultiTransform::getClassTypeId()) == 1 )
            *item << "PartDesign_MultiTransform";
    }
}

void Workbench::activated()
{
    Gui::Workbench::activated();


    std::vector<Gui::TaskView::TaskWatcher*> Watcher;

    const char* Vertex[] = {
        "PartDesign_Point",
        "PartDesign_Line",
        "PartDesign_Plane",
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
        "PartDesign_Point",
        "PartDesign_Line",
        "PartDesign_Plane",
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
        "PartDesign_Thickness",
        "PartDesign_Point",
        "PartDesign_Line",
        "PartDesign_Plane",
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

    const char* Body2[] = {
        "PartDesign_Boolean",
        0};
    Watcher.push_back(new Gui::TaskView::TaskWatcherCommands(
        "SELECT PartDesign::Body COUNT 1..",
        Body2,
        "Start Boolean",
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
        "PartDesign_Point",
        "PartDesign_Line",
        "PartDesign_Plane",
        0};
    Watcher.push_back(new Gui::TaskView::TaskWatcherCommands(
        "SELECT PartDesign::Plane COUNT 1",
        Plane2,
        "Start Part",
        "Part_Box"
    ));

    const char* Line[] = {
        "PartDesign_Point",
        "PartDesign_Line",
        "PartDesign_Plane",
        0};
    Watcher.push_back(new Gui::TaskView::TaskWatcherCommands(
        "SELECT PartDesign::Line COUNT 1",
        Line,
        "Start Part",
        "Part_Box"
    ));

    const char* Point[] = {
        "PartDesign_Point",
        "PartDesign_Line",
        "PartDesign_Plane",
        0};
    Watcher.push_back(new Gui::TaskView::TaskWatcherCommands(
        "SELECT PartDesign::Point COUNT 1",
        Point,
        "Start Part",
        "Part_Box"
    ));

    const char* NoSel[] = {
        "PartDesign_Body",
        "PartDesign_Part",
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
        "PartDesign_Thickness",
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
        "PartDesign_AdditivePipe",
        "PartDesign_SubtractivePipe",
        "PartDesign_AdditiveLoft",
        "PartDesign_SubtractiveLoft",
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
    //PartDesignGui::ActivePartObject = NULL;
    _switchToDocument(App::GetApplication().getActiveDocument());

    addTaskWatcher(Watcher);
    Gui::Control().showTaskView();

    // Let us be notified when a document is activated, so that we can update the ActivePartObject
    Gui::Application::Instance->signalActiveDocument.connect(boost::bind(&Workbench::slotActiveDocument, this, _1));
    App::GetApplication().signalNewDocument.connect(boost::bind(&Workbench::slotNewDocument, this, _1));
    App::GetApplication().signalFinishRestoreDocument.connect(boost::bind(&Workbench::slotFinishRestoreDocument, this, _1));
    App::GetApplication().signalDeleteDocument.connect(boost::bind(&Workbench::slotDeleteDocument, this, _1));
    // Watch out for objects being added to the active document, so that we can add them to the body
    //App::GetApplication().signalNewObject.connect(boost::bind(&Workbench::slotNewObject, this, _1));
}

void Workbench::deactivated()
{
    // Let us be notified when a document is activated, so that we can update the ActivePartObject
    Gui::Application::Instance->signalActiveDocument.disconnect(boost::bind(&Workbench::slotActiveDocument, this, _1));
    App::GetApplication().signalNewDocument.disconnect(boost::bind(&Workbench::slotNewDocument, this, _1));
    App::GetApplication().signalFinishRestoreDocument.disconnect(boost::bind(&Workbench::slotFinishRestoreDocument, this, _1));
    App::GetApplication().signalDeleteDocument.disconnect(boost::bind(&Workbench::slotDeleteDocument, this, _1));
    //App::GetApplication().signalNewObject.disconnect(boost::bind(&Workbench::slotNewObject, this, _1));

    removeTaskWatcher();
    // reset the active Body
    Gui::Command::doCommand(Gui::Command::Doc,"import PartDesignGui");

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
    *part << "PartDesign_Part"
          << "PartDesign_Body"
          << "PartDesign_NewSketch"
          << "Sketcher_LeaveSketch"
          << "Sketcher_ViewSketch"
          << "Sketcher_MapSketch"
          << "Sketcher_ReorientSketch"
          << "Sketcher_ValidateSketch"
          << geom
          << cons
          << "Separator"
          << "PartDesign_Point"
          << "PartDesign_Line"
          << "PartDesign_Plane"
          << "PartDesign_Shape"
          << "Separator"
          << "PartDesign_Pad"
          << "PartDesign_Pocket"
          << "PartDesign_Revolution"
          << "PartDesign_Groove"
          << "PartDesign_AdditivePipe"
          << "PartDesign_SubtractivePipe"
          << "PartDesign_AdditiveLoft"
          << "PartDesign_SubtractiveLoft"
          << "PartDesign_Fillet"
          << "PartDesign_Chamfer"
          << "PartDesign_Draft"
          << "PartDesign_Thickness"
          << "PartDesign_Mirrored"
          << "PartDesign_LinearPattern"
          << "PartDesign_PolarPattern"
//          << "PartDesign_Scaled"
          << "PartDesign_MultiTransform"
          << "Separator"
          << "PartDesign_Boolean"
          << "Separator"
          //<< "PartDesign_Hole"
          << "PartDesign_InvoluteGear";

    // For 0.13 a couple of python packages like numpy, matplotlib and others
    // are not deployed with the installer on Windows. Thus, the WizardShaft is
    // not deployed either hence the check for the existence of the command.
    if (Gui::Application::Instance->commandManager().getCommandByName("PartDesign_InvoluteGear")) {
        *part << "PartDesign_InvoluteGear";
    }
    if (Gui::Application::Instance->commandManager().getCommandByName("PartDesign_WizardShaft")) {
        *part << "Separator" << "PartDesign_WizardShaft";
    }

    // Replace the "Duplicate selection" menu item with a replacement that is compatible with Body
    item = root->findItem("&Edit");
    Gui::MenuItem* dup = item->findItem("Std_DuplicateSelection");
    dup->setCommand("PartDesign_DuplicateSelection");

    return root;
}

Gui::ToolBarItem* Workbench::setupToolBars() const
{
    Gui::ToolBarItem* root = StdWorkbench::setupToolBars();
    Gui::ToolBarItem* part = new Gui::ToolBarItem(root);
    part->setCommand("Part Design");
    *part << "PartDesign_Part"
          << "PartDesign_Body"
          << "PartDesign_NewSketch"
          << "Sketcher_ViewSketch"
          << "Sketcher_MapSketch"
          << "Sketcher_LeaveSketch"
          << "Separator"
          << "PartDesign_Point"
          << "PartDesign_Line"
          << "PartDesign_Plane"
          << "PartDesign_Shape"
          << "Separator"
          << "PartDesign_CompPrimitiveAdditive"
          << "PartDesign_CompPrimitiveSubtractive"
          << "Separator"
          << "PartDesign_Pad"
          << "PartDesign_Pocket"
          << "PartDesign_Revolution"
          << "PartDesign_Groove"
          << "PartDesign_AdditivePipe"
          << "PartDesign_SubtractivePipe"
          << "PartDesign_AdditiveLoft"
          << "PartDesign_SubtractiveLoft"
          << "PartDesign_Fillet"
          << "PartDesign_Chamfer"
          << "PartDesign_Draft"
          << "PartDesign_Thickness"
          << "PartDesign_Mirrored"
          << "PartDesign_LinearPattern"
          << "PartDesign_PolarPattern"
//          << "PartDesign_Scaled"
          << "PartDesign_MultiTransform"
          << "Separator"
          << "PartDesign_Boolean";

    return root;
}

Gui::ToolBarItem* Workbench::setupCommandBars() const
{
    // Part tools
    Gui::ToolBarItem* root = new Gui::ToolBarItem;
    return root;
}

