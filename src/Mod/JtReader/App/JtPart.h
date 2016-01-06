/***************************************************************************
 *   Copyright (c) Juergen Riegel          (juergen.riegel@web.de) 2016    *
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




#ifndef _AppPlane_h_
#define _AppPlane_h_


#include <App/GeoFeature.h>
#include <App/PropertyStandard.h>



namespace JtReader
{

 class JtPartHandle;


/** Plane Object
 *  Used to define planar support for all kind of operations in the document space
 */
class JtReaderExport JtPart : public App::GeoFeature
{
    PROPERTY_HEADER(App::JtPart);

public:

    /// Constructor
    JtPart(void);
    virtual ~JtPart();
    /// additional information about the plane usage (e.g. "BasePlane-xy" in a Part)
    App::PropertyString JtFile;

    /// get called by the container when a property was changed
    virtual void onChanged(const App::Property* prop);

    /// returns the type name of the ViewProvider
    virtual const char* getViewProviderName(void) const {
        return "Gui::ViewProviderJtPart";
    }

    /// Return the bounding box of the plane (this is always a fixed size)
    static Base::BoundBox3d getBoundBox();
    void initJtHandler(JtPartHandle *partPtr);
 };


} //namespace JtReader



#endif
