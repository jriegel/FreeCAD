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
    Base::Console().Log("JcadLibReader::run(%s)", fileName);
    log.clear();

    QFileInfo info(QString::fromUtf8(fileName));
    QDir sysTempDir(QDir::tempPath());
    QString tempDir = QUuid::createUuid();
    if (!sysTempDir.mkdir(tempDir)){
        log = QString::fromAscii("Error: can not create temp dir!\n");
        return false;
    }
    QString tempDirPath = QDir::tempPath() + QString::fromAscii("/") + tempDir;

    std::string jarFile = App::GetApplication().getHomePath();
    jarFile += "Mod/JtReader/FcJtPlugin.jar";

    if (!info.exists()){
        log = QString::fromAscii("Error: File not found!\n");
        sysTempDir.rmdir(tempDir);
       return false;

    }
 
    QProcess proc;
    QStringList args;
    args << QString::fromAscii("-jar") << QString::fromUtf8(jarFile.c_str()) << QString::fromUtf8(fileName) << tempDirPath;

    qDebug() << args;



#ifdef FC_OS_WIN32
    QString exe = QString::fromAscii("java.exe");
#else
    QString exe = QString::fromAscii("java");
#endif

    proc.start(exe, args);
    if (!proc.waitForStarted()) {
        return false;
    }

    if (!proc.waitForFinished(120000))
        return false;

    log += QString::fromUtf8(proc.readAllStandardError());
    log += QString::fromUtf8(proc.readAllStandardOutput());

    return true;



}
