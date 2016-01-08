/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2012     *
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


#pragma once

#include <Gui/ViewProviderGeometryObject.h>
#include <QObject>

class SoFontStyle;
class SoText2;
class SoBaseColor;
class SoTranslation;
class SoCoordinate3;
class SoIndexedLineSet;
class SoEventCallback;
class SoMaterial;
class SoAsciiText;
class SoFont;


namespace JtReader {
    class JtPartHandle;
}


/// this method fill up the inventor nodes from the jt-part-handler class
std::vector<SoNode*> createSoNodesFromHandle(const JtReader::JtPartHandle& partHandle);

namespace JtReaderGui 
{




class JtReaderGuiExport ViewProviderJtPart : public Gui::ViewProviderGeometryObject
{
    PROPERTY_HEADER(Gui::ViewProviderJtPart);

public:
    /// Constructor
    ViewProviderJtPart(void);
    virtual ~ViewProviderJtPart();

    void attach(App::DocumentObject *);
    void updateData(const App::Property*);
    std::vector<std::string> getDisplayModes(void) const;
    void setDisplayMode(const char* ModeName);

   /// indicates if the ViewProvider use the new Selection model
    virtual bool useNewSelectionModel(void) const {return true;}
    /// indicates if the ViewProvider can be selected
    virtual bool isSelectable(void) const ;
    /// return a hit element to the selection path or 0
    virtual std::string getElement(const SoDetail*) const;
    virtual SoDetail* getDetail(const char*) const;

protected:
    void onChanged(const App::Property* prop);
    void updateJtPartGeometry(const JtReader::JtPartHandle* newPartHandle);
private:
    SoGroup* pcJtRootGroup;
    

    const JtReader::JtPartHandle* partHandle;
};

} //namespace Gui
