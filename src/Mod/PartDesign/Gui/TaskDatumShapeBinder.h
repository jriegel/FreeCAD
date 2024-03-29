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


#ifndef GUI_TASKVIEW_TaskDatumShapeBinder_H
#define GUI_TASKVIEW_TaskDatumShapeBinder_H

#include <Gui/TaskView/TaskView.h>
#include <Gui/Selection.h>
#include <Gui/TaskView/TaskDialog.h>

#include "ViewProviderPipe.h"
#include "ViewProviderDatumShapeBinder.h"
#include <QListWidget>

class Ui_TaskDatumShapeBinder;
class Ui_TaskPipeOrientation;
class Ui_TaskPipeScaling;


namespace App {
class Property;
}

namespace Gui {
class ViewProvider;
}

namespace PartDesignGui { 



class TaskDatumShapeBinder : public Gui::TaskView::TaskBox, Gui::SelectionObserver
{
    Q_OBJECT

public:
    TaskDatumShapeBinder(ViewProviderDatumShapeBinder *PipeView,bool newObj=false,QWidget *parent = 0);
    ~TaskDatumShapeBinder();

 
private Q_SLOTS:
    void onButtonRefAdd(bool checked);
    void onButtonRefRemove(bool checked);
    void onBaseButton(bool checked);
  
protected:
    enum selectionModes { none, refAdd, refRemove, refObjAdd };
    void changeEvent(QEvent *e);
    selectionModes selectionMode = none;
    
    void removeFromListWidget(QListWidget*w, QString name);
    bool referenceSelected(const Gui::SelectionChanges& msg) const;

private:
    void onSelectionChanged(const Gui::SelectionChanges& msg);
    void updateUI();
    void clearButtons();
    void exitSelectionMode();

    bool supportShow = false;
    
private:
    QWidget* proxy;
    Ui_TaskDatumShapeBinder* ui;
    ViewProviderDatumShapeBinder* vp;
};


/// simulation dialog for the TaskView
class TaskDlgDatumShapeBinder : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskDlgDatumShapeBinder(ViewProviderDatumShapeBinder *view,bool newObj=false);
    ~TaskDlgDatumShapeBinder();

public:
    /// is called by the framework if the dialog is accepted (Ok)
    virtual bool accept();
    /// is called by the framework if the dialog is rejected (Cancel)
    //virtual bool reject();

protected:
    TaskDatumShapeBinder  *parameter;
    ViewProviderDatumShapeBinder* vp;
};

} //namespace PartDesignGui

#endif // GUI_TASKVIEW_TASKAPPERANCE_H
