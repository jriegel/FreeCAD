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
# include <sstream>
# include <QRegExp>
# include <QTextStream>
# include <QMessageBox>
# include <Precision.hxx>
#endif

#include "ui_TaskDatumShapeBinder.h"
#include "TaskDatumShapeBinder.h"

#include "ReferenceSelection.h"
#include "Workbench.h"
#include <Mod/PartDesign/App/DatumShape.h>
#include <Mod/Part/App/PartFeature.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/Command.h>

using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskDatumShapeBinder */


//**************************************************************************
//**************************************************************************
// Task Parameter
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDatumShapeBinder::TaskDatumShapeBinder(ViewProviderDatumShapeBinder *view,bool newObj, QWidget *parent)
    : Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("PartDesign_ShapeBinder"),
                             tr("Datum shape parameters"), true, parent)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskDatumShapeBinder();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    connect(ui->buttonRefAdd, SIGNAL(toggled(bool)),
            this, SLOT(onButtonRefAdd(bool)));
    connect(ui->buttonRefRemove, SIGNAL(toggled(bool)),
            this, SLOT(onButtonRefRemove(bool)));
    connect(ui->buttonBase, SIGNAL(toggled(bool)),
            this, SLOT(onBaseButton(bool)));
    
    this->groupLayout()->addWidget(proxy);
    
    Gui::Document* doc = Gui::Application::Instance->activeDocument(); 
    vp = view;
    
    //add initial values   
    Part::Feature* obj = nullptr;
    std::vector<std::string> subs;
            
    if(vp->getObject()->isDerivedFrom(PartDesign::ShapeBinder::getClassTypeId()))
        PartDesign::ShapeBinder::getFilterdReferences(&static_cast<PartDesign::ShapeBinder*>(vp->getObject())->Support, obj, subs);            
    else 
        PartDesign::ShapeBinder::getFilterdReferences(&static_cast<PartDesign::ShapeBinder2D*>(vp->getObject())->Support, obj, subs); 
    
    if(obj)
        ui->baseEdit->setText(QString::fromUtf8(obj->getNameInDocument()));

    for (auto sub : subs)
        ui->listWidgetReferences->addItem(QString::fromStdString(sub));
 
    //make sure th euser sees al important things: the base feature to select edges and the 
    //spine/auxillery spine he already selected 
    if(obj) {
        auto* svp = doc->getViewProvider(obj);
        if(svp) {
            supportShow = svp->isShow();
            svp->setVisible(true);
        }
    }
    
    updateUI();
}

void TaskDatumShapeBinder::updateUI()
{
    
}

void TaskDatumShapeBinder::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (selectionMode == none)
        return;

    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        if (referenceSelected(msg)) {
            if (selectionMode == refAdd) {
                QString sub = QString::fromStdString(msg.pSubName);
                if(!sub.isEmpty())
                    ui->listWidgetReferences->addItem(QString::fromStdString(msg.pSubName));
                
                ui->baseEdit->setText(QString::fromStdString(msg.pObjectName));
            }
            else if (selectionMode == refRemove) {
                QString sub = QString::fromStdString(msg.pSubName);
                if(!sub.isEmpty())
                    removeFromListWidget(ui->listWidgetReferences, QString::fromUtf8(msg.pSubName));
                else {
                    ui->baseEdit->clear();
                }                
            } else if(selectionMode == refObjAdd) {
                ui->listWidgetReferences->clear();
                ui->baseEdit->setText(QString::fromUtf8(msg.pObjectName));
            }
            clearButtons();
            static_cast<ViewProviderDatumShapeBinder*>(vp)->highlightReferences(false, false);
            vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
        } 
        clearButtons();
        exitSelectionMode();
    }
}

TaskDatumShapeBinder::~TaskDatumShapeBinder()
{/*
    PartDesign::Pipe* pipe = static_cast<PartDesign::Pipe*>(vp->getObject());
    Gui::Document* doc = Gui::Application::Instance->activeDocument(); 
    
    //make sure th euser sees al important things: the base feature to select edges and the 
    //spine/auxillery spine he already selected 
    if(pipe->BaseFeature.getValue())
        doc->getViewProvider(pipe->BaseFeature.getValue())->hide();
    if(pipe->Spine.getValue()) {
        auto* svp = doc->getViewProvider(pipe->Spine.getValue());
        svp->setVisible(supportShow);
        supportShow = false;
    }
    static_cast<ViewProviderPipe*>(vp)->highlightReferences(false, false);
    */
    delete ui;
}

void TaskDatumShapeBinder::changeEvent(QEvent *e)
{
}

void TaskDatumShapeBinder::onButtonRefAdd(bool checked) {
    
    if (checked) {
        //clearButtons(refAdd);
        //hideObject();
        Gui::Selection().clearSelection();
        selectionMode = refAdd;
        vp->highlightReferences(true, false);
    }
}

void TaskDatumShapeBinder::onButtonRefRemove(bool checked) {

    if (checked) {
        //clearButtons(refRemove);
        //hideObject();
        Gui::Selection().clearSelection();        
        selectionMode = refRemove;
        vp->highlightReferences(true, false);
    }
}

void TaskDatumShapeBinder::onBaseButton(bool checked) {

    if (checked) {
        //clearButtons(refRemove);
        //hideObject();
        Gui::Selection().clearSelection();        
        selectionMode = refObjAdd;
        //DressUpView->highlightReferences(true);
    }
}

void TaskDatumShapeBinder::removeFromListWidget(QListWidget* widget, QString itemstr) {

    QList<QListWidgetItem*> items = widget->findItems(itemstr, Qt::MatchExactly);
    if (!items.empty()) {
        for (QList<QListWidgetItem*>::const_iterator i = items.begin(); i != items.end(); i++) {
            QListWidgetItem* it = widget->takeItem(widget->row(*i));
            delete it;
        }
    }
}

bool TaskDatumShapeBinder::referenceSelected(const SelectionChanges& msg) const {
    
    if ((msg.Type == Gui::SelectionChanges::AddSelection) && (
                (selectionMode == refAdd) || (selectionMode == refRemove) 
                || (selectionMode == refObjAdd))) {

        if (strcmp(msg.pDocName, vp->getObject()->getDocument()->getName()) != 0)
            return false;

        // not allowed to reference ourself
        const char* fname = vp->getObject()->getNameInDocument();        
        if (strcmp(msg.pObjectName, fname) == 0)
            return false;
       
        //change the references 
        std::string subName(msg.pSubName);

        Part::Feature* obj = nullptr;
        std::vector<std::string> refs;
                
        if(vp->getObject()->isDerivedFrom(PartDesign::ShapeBinder::getClassTypeId()))
            PartDesign::ShapeBinder::getFilterdReferences(&static_cast<PartDesign::ShapeBinder*>(vp->getObject())->Support, obj, refs);            
        else 
            PartDesign::ShapeBinder::getFilterdReferences(&static_cast<PartDesign::ShapeBinder2D*>(vp->getObject())->Support, obj, refs); 
    
        //if we already have a object we need to ensure th new selected subref belongs to it
        if(obj && strcmp(msg.pObjectName, obj->getNameInDocument()) != 0)
            return false;
        
        std::vector<std::string>::iterator f = std::find(refs.begin(), refs.end(), subName);

        if(selectionMode != refObjAdd) {
            if (selectionMode == refAdd) {
                if (f == refs.end())
                    refs.push_back(subName);
                else
                    return false; // duplicate selection
            } else {
                if (f != refs.end())
                    refs.erase(f);
                else
                    return false;
            }        
        }
        else {
            refs.clear();
        }
        
        if(vp->getObject()->isDerivedFrom(PartDesign::ShapeBinder::getClassTypeId()))
            static_cast<PartDesign::ShapeBinder*>(vp->getObject())->Support.setValue(obj, refs);            
        else 
            static_cast<PartDesign::ShapeBinder2D*>(vp->getObject())->Support.setValue( obj, refs); 
    
        return true;
    }

    return false;
}

void TaskDatumShapeBinder::clearButtons() {

    ui->buttonRefAdd->setChecked(false);
    ui->buttonRefRemove->setChecked(false);
    ui->buttonBase->setChecked(false);
}

void TaskDatumShapeBinder::exitSelectionMode() {

    selectionMode = none;
    Gui::Selection().clearSelection();
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgDatumShapeBinder::TaskDlgDatumShapeBinder(ViewProviderDatumShapeBinder *view, bool newObj)
   : Gui::TaskView::TaskDialog()
{
    assert(view);
    parameter    = new TaskDatumShapeBinder(view, newObj);
    vp = view;

    Content.push_back(parameter);
}

TaskDlgDatumShapeBinder::~TaskDlgDatumShapeBinder()
{

}

//==== calls from the TaskView ===============================================================


bool TaskDlgDatumShapeBinder::accept()
{
    std::string name = vp->getObject()->getNameInDocument();

    try {
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.recompute()");
        if (!vp->getObject()->isValid())
            throw Base::Exception(vp->getObject()->getStatusString());
        Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");
        Gui::Command::commitCommand();
    }
    catch (const Base::Exception& e) {
        QMessageBox::warning(parameter, tr("Input error"), QString::fromUtf8(e.what()));
        return false;
    }

    return true;
}

//bool TaskDlgDatumShapeBinder::reject()
//{
//    // get the support and Sketch
//    PartDesign::Pipe* pcPipe = static_cast<PartDesign::Pipe*>(PipeView->getObject()); 
//    Sketcher::SketchObject *pcSketch = 0;
//    App::DocumentObject    *pcSupport = 0;
//    if (pcPipe->Sketch.getValue()) {
//        pcSketch = static_cast<Sketcher::SketchObject*>(pcPipe->Sketch.getValue()); 
//        pcSupport = pcSketch->Support.getValue();
//    }
//
//    // roll back the done things
//    Gui::Command::abortCommand();
//    Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");
//    
//    // if abort command deleted the object the support is visible again
//    if (!Gui::Application::Instance->getViewProvider(pcPipe)) {
//        if (pcSketch && Gui::Application::Instance->getViewProvider(pcSketch))
//            Gui::Application::Instance->getViewProvider(pcSketch)->show();
//        if (pcSupport && Gui::Application::Instance->getViewProvider(pcSupport))
//            Gui::Application::Instance->getViewProvider(pcSupport)->show();
//    }
//
//    //Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.recompute()");
//    //Gui::Command::commitCommand();
//
//    return true;
//}



#include "moc_TaskDatumShapeBinder.cpp"
