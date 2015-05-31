/***************************************************************************
*   Copyright (c) Juergen Riegel         (juergen.riegel@web.de) 2014     *
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

#include "JcadLibReader.h"

#include <QFileInfo>
#include <QFileDialog>
#include <QDir>
#include <QUuid>
#include <QDebug>
#include <QTextStream>

#include <App/Application.h>

#include <qprocess.h>




JcadLibReader::JcadLibReader()
{

}

JcadLibReader::~JcadLibReader()
{

}




bool JcadLibReader::read(const char* fileName)
{
    qDebug() <<  "JcadLibReader::run(" << fileName << ")";
    log.clear();

    QFileInfo info(QString::fromUtf8(fileName));
 /*   QDir sysTempDir(QDir::tempPath());
    QString tempDir = QUuid::createUuid();
    if (!sysTempDir.mkdir(tempDir)){
        log = QString::fromAscii("Error: can not create temp dir!\n");
        return false;
    }
    QString tempDirPath = QDir::tempPath() + QString::fromAscii("/") + tempDir;*/

    std::string jarFile = App::GetApplication().getHomePath();
    jarFile += "Mod/JtReader/FcJtPlugin.jar";

    if (!info.exists()){
        log = QString::fromAscii("Error: File not found!\n");
       // sysTempDir.rmdir(tempDir);
       return false;

    }
 
    QProcess proc;
    QStringList args;
    args << QString::fromAscii("-jar") << QString::fromUtf8(jarFile.c_str()) << QString::fromUtf8(fileName);// << tempDirPath;

    qDebug() << args;


    QString exe = QString::fromAscii("java");
    proc.start(exe, args);

    if (!proc.waitForStarted()) {
        log = QString::fromAscii("Error: waitForStarted() failed\n");
        return false;
    }
    if (!proc.waitForFinished(120000)){
        log = QString::fromAscii("Error: waitForFinished(120000) failed\n");
        return false;
    }

    log += QString::fromUtf8(proc.readAllStandardError());
    QByteArray out = proc.readAllStandardOutput();

    QTextStream inputStream(&out,QFile::ReadOnly);

    QByteArray line;
    std::vector<Buffer>::iterator actBuffer = Faces.end();
    
    do {
        line = inputStream.readLine().toUtf8();
        if (line[0] == ':'){
            if (line[1] == 'V'){
                if (line[2] == 'C'){
                    actBuffer->Vertexes.reserve(line.right(line.length() - 4).toInt());
                }
                else{
                    QList<QByteArray> v = line.right(line.length() - 3).split(';');
                    MiniVec vc;
                    vc.vec[0] = v[0].toFloat();
                    vc.vec[1] = v[1].toFloat();
                    vc.vec[2] = v[2].toFloat();
                    actBuffer->Vertexes.push_back(vc);
                }
            }
            else if (line[1] == 'C'){
                if (line[2] == 'C'){
                    actBuffer->Colors.reserve(line.right(line.length() - 4).toInt());
                }
                else{
                    QList<QByteArray> v = line.right(line.length() - 3).split(';');
                    MiniVec vc;
                    vc.vec[0] = v[0].toFloat();
                    vc.vec[1] = v[1].toFloat();
                    vc.vec[2] = v[2].toFloat();
                    actBuffer->Colors.push_back(vc);
                }

            }
            else if (line[1] == 'N'){
                if (line[2] == 'C'){
                    actBuffer->Normals.reserve(line.right(line.length() - 4).toInt());
                }
                else{
                    QList<QByteArray> v = line.right(line.length() - 3).split(';');
                    MiniVec vc;
                    vc.vec[0] = v[0].toFloat();
                    vc.vec[1] = v[1].toFloat();
                    vc.vec[2] = v[2].toFloat();
                    actBuffer->Normals.push_back(vc);
                }
            }
            else if (line[1] == 'I'){
                if (line[2] == 'C'){
                    actBuffer->Indexes.reserve(line.right(line.length() - 4).toInt());
                }
                else{
                    int idx = line.right(line.length() - 3).toInt();
                    actBuffer->Indexes.push_back(idx);
                }
            }
            else if (line[1] == 'F'){
                QString FaceName = QString::fromUtf8(line.right(line.length() - 3));
                Names.push_back(FaceName);  
                Faces.push_back(Buffer());
                actBuffer = Faces.end() - 1;

            }
            else{
                log += QString::fromAscii("Error: Unknown Char in key sequence:") + QString::fromUtf8(line) +QString::fromAscii("\n");
            }
                


        }else
            log += QString::fromUtf8(line) + QString::fromAscii("\n");
    } while (!line.isNull());


    log += QString::fromAscii("Sizes: %1  %2").arg(Faces.size()).arg(Faces[0].Vertexes.size()) ;

    return true;



}
