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

class Handle_JtData_Object;

class Handle_JtNode_Part;
class Handle_JtNode_Partition;
class Handle_JtNode_LOD;
class Handle_JtNode_Instance;


class Handle_JtElement_ShapeLOD_TriStripSet;


#include <App/ComplexGeoData.h>

namespace JtReader
{

class JtReaderExport JtPartHandle
{
public:
    struct JtMat {
        float AmbientColor[4];
        float DiffuseColor[4];
        float SpecularColor[4];
        float EmissionColor[4];
        float Shininess;
        float Reflectivity;
    };

public:
    JtPartHandle();
    ~JtPartHandle();

    /// fill this handle with the information in the JtPartObject
    void init(const Handle_JtNode_Part &jtPartObject, const Handle_JtData_Object &parentObject);

    /// get the actual mesh and material 
    void getFaces(int lodLevel, int fragment, std::vector<Base::Vector3d> &Points, std::vector<Base::Vector3d> &Normals, std::vector<Data::ComplexGeoData::Facet> &Topo,bool &hasMat, JtMat &mat);

    const char* getName();
    // returns the number of available LODs
    int getLodCount();
    // return the number of Meshes (with different material) of a given LOD
    int getLodFragmentCount(int lodLevel);

    /// uses a callback to provide the Meshes 
    /// set to true if the Part was under an instance object
    bool isInstanced;

protected:
    //Handle_JtData_Model model;
    Handle_JtNode_Instance *instanceObj;
    Handle_JtNode_LOD *lodObject;
    std::string partName;
    
};

}



