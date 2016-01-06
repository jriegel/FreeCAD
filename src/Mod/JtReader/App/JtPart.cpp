/***************************************************************************
 *   Copyright (c) Juergen Riegel          (juergen.riegel@web.de) 2012    *
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


#include "JtPart.h"

#include "TkJtLibReader.h"

using namespace JtReader;


PROPERTY_SOURCE(JtReader::JtPart, App::GeoFeature)


//===========================================================================
// Feature
//===========================================================================

JtPart::JtPart(void)
{
    ADD_PROPERTY(JtFile, (""));
    //placement can't be changed
    Placement.StatusBits.set(3, true);

}

JtPart::~JtPart(void)
{
}

Base::BoundBox3d JtPart::getBoundBox()
{
    return Base::BoundBox3d(-10, -10, -10, 10, 10, 10);
}

void JtReader::JtPart::onChanged(const App::Property* prop)
{
    if (prop == &JtFile) {
        TkJtLibReader reader(JtFile.getValue());

        if (! reader.isValid())
            return;

        reader.Dump(std::cout);

        if (reader.countParts() != 1)
            return;


        
    }

}






