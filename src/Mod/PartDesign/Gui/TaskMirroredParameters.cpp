/******************************************************************************
 *   Copyright (c)2012 Jan Rheinlaender <jrheinlaender@users.sourceforge.net> *
 *                                                                            *
 *   This file is part of the FreeCAD CAx development system.                 *
 *                                                                            *
 *   This library is free software; you can redistribute it and/or            *
 *   modify it under the terms of the GNU Library General Public              *
 *   License as published by the Free Software Foundation; either             *
 *   version 2 of the License, or (at your option) any later version.         *
 *                                                                            *
 *   This library  is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Library General Public License for more details.                     *
 *                                                                            *
 *   You should have received a copy of the GNU Library General Public        *
 *   License along with this library; see the file COPYING.LIB. If not,       *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,            *
 *   Suite 330, Boston, MA  02111-1307, USA                                   *
 *                                                                            *
 ******************************************************************************/


#include "PreCompiled.h"

#ifndef _PreComp_
# include <QMessageBox>
#endif

#include "ui_TaskMirroredParameters.h"
#include "TaskMirroredParameters.h"
#include "TaskMultiTransformParameters.h"
#include "Workbench.h"
#include "ReferenceSelection.h"
#include <App/Application.h>
#include <App/Document.h>
#include <App/Part.h>
#include <App/Plane.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Base/Console.h>
#include <Gui/Selection.h>
#include <Gui/Command.h>
#include <Gui/ViewProviderOrigin.h>
#include <Mod/PartDesign/App/DatumPlane.h>
#include <Mod/PartDesign/App/FeatureMirrored.h>
#include <Mod/Sketcher/App/SketchObject.h>

using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskMirroredParameters */

TaskMirroredParameters::TaskMirroredParameters(ViewProviderTransformed *TransformedView, QWidget *parent)
        : TaskTransformedParameters(TransformedView, parent)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskMirroredParameters();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    this->groupLayout()->addWidget(proxy);

    ui->buttonOK->hide();
    ui->checkBoxUpdateView->setEnabled(true);

    selectionMode = none;

    blockUpdate = false; // Hack, sometimes it is NOT false although set to false in Transformed::Transformed()!!
    setupUI();
}

TaskMirroredParameters::TaskMirroredParameters(TaskMultiTransformParameters *parentTask, QLayout *layout)
        : TaskTransformedParameters(parentTask)
{
    proxy = new QWidget(parentTask);
    ui = new Ui_TaskMirroredParameters();
    ui->setupUi(proxy);
    connect(ui->buttonOK, SIGNAL(pressed()),
            parentTask, SLOT(onSubTaskButtonOK()));
    QMetaObject::connectSlotsByName(this);

    layout->addWidget(proxy);

    ui->buttonOK->setEnabled(true);
    ui->buttonAddFeature->hide();
    ui->buttonRemoveFeature->hide();
    ui->listWidgetFeatures->hide();
    ui->checkBoxUpdateView->hide();

    selectionMode = none;

    blockUpdate = false; // Hack, sometimes it is NOT false although set to false in Transformed::Transformed()!!
    setupUI();
}

void TaskMirroredParameters::setupUI()
{
    connect(ui->buttonAddFeature, SIGNAL(toggled(bool)), this, SLOT(onButtonAddFeature(bool)));
    connect(ui->buttonRemoveFeature, SIGNAL(toggled(bool)), this, SLOT(onButtonRemoveFeature(bool)));
    // Create context menu
    QAction* action = new QAction(tr("Remove"), this);
    ui->listWidgetFeatures->addAction(action);
    connect(action, SIGNAL(triggered()), this, SLOT(onFeatureDeleted()));
    ui->listWidgetFeatures->setContextMenuPolicy(Qt::ActionsContextMenu);

    connect(ui->comboPlane, SIGNAL(activated(int)),
            this, SLOT(onPlaneChanged(int)));
    connect(ui->checkBoxUpdateView, SIGNAL(toggled(bool)),
            this, SLOT(onUpdateView(bool)));

    // Get the feature data
    PartDesign::Mirrored* pcMirrored = static_cast<PartDesign::Mirrored*>(getObject());
    std::vector<App::DocumentObject*> originals = pcMirrored->Originals.getValues();

    // Fill data into dialog elements
    for (std::vector<App::DocumentObject*>::const_iterator i = originals.begin(); i != originals.end(); i++)
    {
        if ((*i) != NULL)
            ui->listWidgetFeatures->addItem(QString::fromAscii((*i)->getNameInDocument()));
    }
    // ---------------------

    ui->comboPlane->setEnabled(true);
    
    App::DocumentObject* sketch = getSketchObject();
    if (sketch) {
        ui->comboPlane->addItem(QString::fromAscii("Horizontal sketch axis"));
        ui->comboPlane->addItem(QString::fromAscii("Vertical sketch axis"));
    }
    //add the base axes to the selection combo box
    ui->comboPlane->addItem(QString::fromAscii("Base XY axis"));
    ui->comboPlane->addItem(QString::fromAscii("Base XZ axis"));    
    ui->comboPlane->addItem(QString::fromAscii("Base YZ axis")); 
    ui->comboPlane->addItem(QString::fromAscii("Select reference..."));
    updateUI();
    
    //show the parts coordinate system axis for selection
    App::Part* part = getPartFor(getObject(), false);
    if(part) {        
        auto app_origin = part->getObjectsOfType(App::Origin::getClassTypeId());
        if(!app_origin.empty()) {
            ViewProviderOrigin* origin;
            origin = static_cast<ViewProviderOrigin*>(Gui::Application::Instance->activeDocument()->getViewProvider(app_origin[0]));
            origin->setTemporaryVisibilityMode(true, Gui::Application::Instance->activeDocument());
            origin->setTemporaryVisibilityAxis(true);
        }            
     }
}

void TaskMirroredParameters::updateUI()
{
    if (blockUpdate)
        return;
    blockUpdate = true;

    PartDesign::Mirrored* pcMirrored = static_cast<PartDesign::Mirrored*>(getObject());

    App::DocumentObject* mirrorPlaneFeature = pcMirrored->MirrorPlane.getValue();
    std::vector<std::string> mirrorPlanes = pcMirrored->MirrorPlane.getSubValues();

    // Add user-defined sketch axes to the reference selection combo box
    App::DocumentObject* sketch = getSketchObject();
    int maxcount=3;
    if (sketch) {
        maxcount = 5;
        maxcount += static_cast<Part::Part2DObject*>(sketch)->getAxisCount();
    }

    for (int i=ui->comboPlane->count()-1; i >= (sketch ? 5 : 3); i--)
        ui->comboPlane->removeItem(i);
    for (int i=ui->comboPlane->count(); i < maxcount; i++)
        ui->comboPlane->addItem(QString::fromAscii("Sketch axis %1").arg(i-5));

    bool undefined = false;
    if (mirrorPlaneFeature != NULL && !mirrorPlanes.empty()) {
        bool is_base_plane = mirrorPlaneFeature->isDerivedFrom(App::Plane::getClassTypeId());
        
        if (mirrorPlanes.front() == "H_Axis")
            ui->comboPlane->setCurrentIndex(0);
        else if (mirrorPlanes.front() == "V_Axis")
            ui->comboPlane->setCurrentIndex(1);
        else if (is_base_plane && strcmp(static_cast<App::Plane*>(mirrorPlaneFeature)->PlaneType.getValue(), App::Part::BaseplaneTypes[0]) == 0)
            ui->comboPlane->setCurrentIndex((sketch ? 2 : 0));
        else if (is_base_plane && strcmp(static_cast<App::Plane*>(mirrorPlaneFeature)->PlaneType.getValue(), App::Part::BaseplaneTypes[1]) == 0)
            ui->comboPlane->setCurrentIndex((sketch ? 3 : 1));
        else if (is_base_plane && strcmp(static_cast<App::Plane*>(mirrorPlaneFeature)->PlaneType.getValue(), App::Part::BaseplaneTypes[2]) == 0)
            ui->comboPlane->setCurrentIndex((sketch ? 4 : 2));
        else if (mirrorPlanes.front().size() > (sketch ? 4 : 2) && mirrorPlanes.front().substr(0,4) == "Axis") {
            int pos = (sketch ? 5 : 3) + std::atoi(mirrorPlanes.front().substr(4,4000).c_str());
            if (pos <= maxcount)
                ui->comboPlane->setCurrentIndex(pos);
            else
                undefined = true;
        } else {
            ui->comboPlane->addItem(getRefStr(mirrorPlaneFeature, mirrorPlanes));
            ui->comboPlane->setCurrentIndex(maxcount);
        }
    } else {
        undefined = true;
    }

    if (selectionMode == reference) {
        ui->comboPlane->addItem(tr("Select a face or datum plane"));
        ui->comboPlane->setCurrentIndex(ui->comboPlane->count() - 1);
    } else if (undefined) {
        ui->comboPlane->addItem(tr("Undefined"));
        ui->comboPlane->setCurrentIndex(ui->comboPlane->count() - 1);
    } else
        ui->comboPlane->addItem(tr("Select reference..."));

    blockUpdate = false;
}

void TaskMirroredParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (msg.Type == Gui::SelectionChanges::AddSelection) {

        if (originalSelected(msg)) {
            if (selectionMode == addFeature)
                ui->listWidgetFeatures->addItem(QString::fromAscii(msg.pObjectName));
            else
                removeItemFromListWidget(ui->listWidgetFeatures, msg.pObjectName);
            exitSelectionMode();
        } else if (selectionMode == reference) {
            // Note: ReferenceSelection has already checked the selection for validity
            exitSelectionMode();
            if (!blockUpdate) {
                std::vector<std::string> mirrorPlanes;
                App::DocumentObject* selObj;
                PartDesign::Mirrored* pcMirrored = static_cast<PartDesign::Mirrored*>(getObject());
                getReferencedSelection(pcMirrored, msg, selObj, mirrorPlanes);
                pcMirrored->MirrorPlane.setValue(selObj, mirrorPlanes);

                recomputeFeature();
                updateUI();
            }
            else {
                App::DocumentObject* sketch = getSketchObject();
                int maxcount=3;
                if (sketch) {
                    maxcount = 5;
                    maxcount += static_cast<Part::Part2DObject*>(sketch)->getAxisCount();
                }
                for (int i=ui->comboPlane->count()-1; i >= maxcount; i--)
                    ui->comboPlane->removeItem(i);

                std::vector<std::string> mirrorPlanes;
                App::DocumentObject* selObj;
                PartDesign::Mirrored* pcMirrored = static_cast<PartDesign::Mirrored*>(getObject());
                getReferencedSelection(pcMirrored, msg, selObj, mirrorPlanes);
                ui->comboPlane->addItem(getRefStr(selObj, mirrorPlanes));
                ui->comboPlane->setCurrentIndex(maxcount);
                ui->comboPlane->addItem(tr("Select reference..."));
            }
        } else if( strstr(msg.pObjectName, App::Part::BaseplaneTypes[0]) == nullptr || 
                   strstr(msg.pObjectName, App::Part::BaseplaneTypes[1]) == nullptr ||
                   strstr(msg.pObjectName, App::Part::BaseplaneTypes[2]) == nullptr) {
           
            std::vector<std::string> planes;
            App::DocumentObject* selObj;
            PartDesign::Mirrored* pcMirrored = static_cast<PartDesign::Mirrored*>(getObject());
            getReferencedSelection(pcMirrored, msg, selObj, planes);
            pcMirrored->MirrorPlane.setValue(selObj, planes);

            recomputeFeature();
            updateUI();
        }
    }
}

void TaskMirroredParameters::clearButtons()
{
    ui->buttonAddFeature->setChecked(false);
    ui->buttonRemoveFeature->setChecked(false);
}

void TaskMirroredParameters::onPlaneChanged(int num) {
    if (blockUpdate)
        return;
    PartDesign::Mirrored* pcMirrored = static_cast<PartDesign::Mirrored*>(getObject());

    App::DocumentObject* pcSketch = getSketchObject();
    int maxcount=3;
    if (pcSketch) {
        maxcount = 5;
        maxcount += static_cast<Part::Part2DObject*>(pcSketch)->getAxisCount();
    }

    if(pcSketch) {
        if (num == 0) {
            pcMirrored->MirrorPlane.setValue(pcSketch, std::vector<std::string>(1,"H_Axis"));
            exitSelectionMode();
        }
        else if (num == 1) {
            pcMirrored->MirrorPlane.setValue(pcSketch, std::vector<std::string>(1,"V_Axis"));
            exitSelectionMode();
        }
    }
    if (num == (pcSketch ? 2 : 0)) {
        pcMirrored->MirrorPlane.setValue(getObject()->getDocument()->getObject(App::Part::BaseplaneTypes[0]),
                                         std::vector<std::string>(1,""));
        exitSelectionMode();
    }
    else if (num == (pcSketch ? 3 : 1)) {
        pcMirrored->MirrorPlane.setValue(getObject()->getDocument()->getObject(App::Part::BaseplaneTypes[1]),
                                         std::vector<std::string>(1,""));
        exitSelectionMode();
    }
    else if (num == (pcSketch ? 4 : 2)) {
        pcMirrored->MirrorPlane.setValue(getObject()->getDocument()->getObject(App::Part::BaseplaneTypes[2]),
                                         std::vector<std::string>(1,""));
        exitSelectionMode();
    }
    else if (num >= (pcSketch ? 5 : 3) && num < maxcount) {
        QString buf = QString::fromUtf8("Axis%1").arg(num-(pcSketch ? 5 : 3));
        std::string str = buf.toStdString();
        pcMirrored->MirrorPlane.setValue(pcSketch, std::vector<std::string>(1,str));
        exitSelectionMode();
    }
    else if (num == ui->comboPlane->count() - 1) {
        // enter reference selection mode
        hideObject();
        showBase();
        selectionMode = reference;
        Gui::Selection().clearSelection();
        addReferenceSelectionGate(false, true);
    }
    else if (num == maxcount)
        exitSelectionMode();

    recomputeFeature();
}

void TaskMirroredParameters::onUpdateView(bool on)
{
    blockUpdate = !on;
    if (on) {
        // Do the same like in TaskDlgMirroredParameters::accept() but without doCommand
        PartDesign::Mirrored* pcMirrored = static_cast<PartDesign::Mirrored*>(getObject());
        std::vector<std::string> mirrorPlanes;
        App::DocumentObject* obj;

        getMirrorPlane(obj, mirrorPlanes);
        pcMirrored->MirrorPlane.setValue(obj,mirrorPlanes);

        recomputeFeature();
    }
}

void TaskMirroredParameters::onFeatureDeleted(void)
{
    PartDesign::Transformed* pcTransformed = getObject();
    std::vector<App::DocumentObject*> originals = pcTransformed->Originals.getValues();
    originals.erase(originals.begin() + ui->listWidgetFeatures->currentRow());
    pcTransformed->Originals.setValues(originals);
    ui->listWidgetFeatures->model()->removeRow(ui->listWidgetFeatures->currentRow());
    recomputeFeature();
}

void TaskMirroredParameters::getMirrorPlane(App::DocumentObject*& obj, std::vector<std::string>& sub) const
{
    obj = getSketchObject();
    sub = std::vector<std::string>(1,"");
    int maxcount=3;
    if (obj) {
        maxcount = 5;
        maxcount += static_cast<Part::Part2DObject*>(obj)->getAxisCount();
    }

    int num = ui->comboPlane->currentIndex();
    if(obj) {
        if (num == 0)
            sub[0] = "H_Axis";
        else if (num == 1)
            sub[0] = "V_Axis";
    }
    if (num == (obj ? 2 : 0))
        obj = getObject()->getDocument()->getObject(App::Part::BaseplaneTypes[0]);
    else if (num == (obj ? 3 : 1))
        obj = getObject()->getDocument()->getObject(App::Part::BaseplaneTypes[1]);
    else if (num == (obj ? 4 : 2))
        obj = getObject()->getDocument()->getObject(App::Part::BaseplaneTypes[2]);
    else if (num >= (obj ? 5 : 3) && num < maxcount) {
        QString buf = QString::fromUtf8("Axis%1").arg(num-(obj ? 5 : 3));
        sub[0] = buf.toStdString();
    } else if (num == maxcount && ui->comboPlane->count() == maxcount + 2) {
        QStringList parts = ui->comboPlane->currentText().split(QChar::fromAscii(':'));
        obj = getObject()->getDocument()->getObject(parts[0].toStdString().c_str());
        if (parts.size() > 1)
            sub[0] = parts[1].toStdString();
    } else {
        obj = NULL;
    }
}

TaskMirroredParameters::~TaskMirroredParameters()
{
    //hide the parts coordinate system axis for selection
    App::Part* part = getPartFor(getObject(), false);
    if(part) {
        auto app_origin = part->getObjectsOfType(App::Origin::getClassTypeId());
        if(!app_origin.empty()) {
            ViewProviderOrigin* origin;
            origin = static_cast<ViewProviderOrigin*>(Gui::Application::Instance->activeDocument()->getViewProvider(app_origin[0]));
            origin->setTemporaryVisibilityMode(false);
        }            
    }
    
    
    delete ui;
    if (proxy)
        delete proxy;
}

void TaskMirroredParameters::changeEvent(QEvent *e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(proxy);
    }
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgMirroredParameters::TaskDlgMirroredParameters(ViewProviderMirrored *MirroredView)
    : TaskDlgTransformedParameters(MirroredView)
{
    parameter = new TaskMirroredParameters(MirroredView);

    Content.push_back(parameter);
}
//==== calls from the TaskView ===============================================================

bool TaskDlgMirroredParameters::accept()
{
    std::string name = TransformedView->getObject()->getNameInDocument();

    try {
            // Handle Originals
        if (!TaskDlgTransformedParameters::accept())
            return false;

        TaskMirroredParameters* mirrorParameter = static_cast<TaskMirroredParameters*>(parameter);
        std::vector<std::string> mirrorPlanes;
        App::DocumentObject* obj;
        mirrorParameter->getMirrorPlane(obj, mirrorPlanes);
        std::string mirrorPlane = getPythonStr(obj, mirrorPlanes);
        if (!mirrorPlane.empty() && obj) {
            Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.MirrorPlane = %s", name.c_str(), mirrorPlane.c_str());
        } else {
            Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.MirrorPlane = None", name.c_str());
        }
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.recompute()");
        if (!TransformedView->getObject()->isValid())
            throw Base::Exception(TransformedView->getObject()->getStatusString());
        Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");
        Gui::Command::commitCommand();
    }
    catch (const Base::Exception& e) {
        QMessageBox::warning(parameter, tr("Input error"), QString::fromAscii(e.what()));
        return false;
    }

    return true;
}

#include "moc_TaskMirroredParameters.cpp"
