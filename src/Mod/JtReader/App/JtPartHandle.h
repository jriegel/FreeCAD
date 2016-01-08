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



namespace JtReader
{

class JtReaderExport JtPartHandle
{

public:
    // Jt material data-type
    struct JtMat {
        float AmbientColor[4];
        float DiffuseColor[4];
        float SpecularColor[4];
        float EmissionColor[4];
        float Shininess;
        float Reflectivity;
        // setter from arrays
        void set(const float* ambientColor, const float* diffuseColor, const float* specularColor, const float* emissionColor, float shininess, float reflectivity){
            AmbientColor[0] = ambientColor[0]; AmbientColor[1] = ambientColor[1]; AmbientColor[2] = ambientColor[2]; AmbientColor[3] = ambientColor[3];
            DiffuseColor[0] = diffuseColor[0]; DiffuseColor[1] = diffuseColor[1]; DiffuseColor[2] = diffuseColor[2]; DiffuseColor[3] = diffuseColor[3];
            SpecularColor[0] = specularColor[0]; SpecularColor[1] = specularColor[1]; SpecularColor[2] = specularColor[2]; SpecularColor[3] = specularColor[3];
            EmissionColor[0] = emissionColor[0]; EmissionColor[1] = emissionColor[1]; EmissionColor[2] = emissionColor[2]; EmissionColor[3] = emissionColor[3];
            Shininess = shininess;
            Reflectivity = reflectivity;
        }
    };

    
public:
    JtPartHandle();
    ~JtPartHandle();


    /// get the actual mesh and material 
    void getFaces(int lodLevel, int fragment, std::vector<float> &Points, std::vector<float> &Normals, std::vector<int> &Topo, bool &hasMat, JtMat &mat) const;

    const char* getName() const;
    // returns the number of available LODs
    int getLodCount() const;
    // return the number of Meshes (with different material) of a given LOD
    int getLodFragmentCount(int lodLevel) const;

    /// uses a callback to provide the Meshes 
    /// set to true if the Part was under an instance object
    bool isInstanced() const { return _Instanced; }

    const JtReader::JtPartHandle::JtMat &getPartMaterial() const;
    bool hasPartMatrial() const { return _hasPartMatrial; }

    /// fill this handle with the information in the JtPartObject 
    void init(const Handle_JtNode_Part &jtPartObject, const Handle_JtData_Object &parentObject);

protected:

    //Handle_JtData_Model model;
    Handle_JtNode_Instance *instanceObj;
    Handle_JtNode_LOD *lodObject;
    std::string partName;
    bool _Instanced;
    JtMat _partMaterial;
    bool _hasPartMatrial;

};

}



