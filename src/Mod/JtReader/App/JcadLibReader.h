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


#include <QString>
#include <vector>

class JtReaderExport JcadLibReader
{
    QString log;

public:

    JcadLibReader();
    ~JcadLibReader();
    bool read(const char* fileName);

    const QString& getLog(){ return log; }

    struct MiniVec {
        float vec[3];
    };
    struct Buffer {
        std::vector<MiniVec> Vertexes;
        std::vector<MiniVec> Normals;
        std::vector<MiniVec> Colors;
        std::vector<int32_t> Indexes;
    };

    int getFaceCount() const{ return Faces.size(); }
    const Buffer& getBuffer(int n)const { return Faces[n]; }
    const QString& getName(int n)const { return Names[n]; }

protected:
    std::vector<Buffer> Faces;
    std::vector<QString> Names;


};








