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

#pragma once

// forward of the TkJt data model
class Handle_JtData_Model;
class Handle_JtNode_Partition;
class Handle_JtData_Object;
class Handle_JtElement_ShapeLOD_TriStripSet;


class TkJtLibReader 
{
public:
    TkJtLibReader(const char* jtFileName); 

    bool isValid() const;

    void Dump();

    int countParts();

protected:
    Handle_JtData_Model *model;
    Handle_JtNode_Partition *partition;

    void traverseDump(const Handle_JtData_Object& obj, int indent = 0);
    void HandleTriangulation(const Handle_JtElement_ShapeLOD_TriStripSet& ShapeLOD);
    int traverseCount(const Handle_JtData_Object& obj);
};





