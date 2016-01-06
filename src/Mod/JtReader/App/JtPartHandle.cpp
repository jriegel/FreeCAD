/***************************************************************************
*   Copyright (c) Juergen Riegel         (juergen.riegel@web.de) 2015     *
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

#include <Base/Console.h>

#include "JtPartHandle.h"


#include <JtData_Model.hxx>
#include <JtNode_Partition.hxx>
#include <JtData_Object.hxx>
#include <JtNode_Part.hxx>
#include <JtNode_RangeLOD.hxx>
#include <JtElement_ShapeLOD_TriStripSet.hxx>
#include <JtNode_Shape_TriStripSet.hxx>




using namespace std;
using namespace JtReader;



JtPartHandle::JtPartHandle(const char* jtFileName)
{
    model = new Handle_JtData_Model();
    partition = new Handle(JtNode_Partition);

    *model = new JtData_Model(TCollection_ExtendedString(jtFileName));
	//Base::Console().Log("FcLodHandler::startLod()");

    (*partition) = (*model)->Init(); // inti reads the TOC

}

void JtReader::JtPartHandle::getFaces(int lodLevel, std::vector<Base::Vector3d> &Points, std::vector<Data::ComplexGeoData::Facet> &Topo)
{
    Points.reserve(10);
    Topo.reserve(10);

}


