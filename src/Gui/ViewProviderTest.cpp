/***************************************************************************
 *   Copyright (c) 2015 Jürgen Riegel <juergen.riegel@web.de>              *
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
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoSwitch.h>
#endif

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/Stream.h>
#include "SoFCSelection.h"
#include "ViewProviderTest.h"

#include <cstring>
#include <algorithm>

using std::vector;
using std::string;


using namespace Gui;

PROPERTY_SOURCE(Gui::ViewProviderTest, Gui::ViewProvider)


ViewProviderTest::ViewProviderTest()
{

}

ViewProviderTest::~ViewProviderTest()
{

}

const char* ViewProviderTest::getDefaultDisplayMode() const
{
    // returns the first item of the available modes
    return "Main";
}

std::vector<std::string> ViewProviderTest::getDisplayModes(void) const
{
    std::vector<std::string> ret;
    ret.reserve(1);
    ret.push_back("Main");
    return ret;
}
