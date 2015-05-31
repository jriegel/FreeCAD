/***************************************************************************
 *   Copyright (c) Juergen Riegel         <juergen.riegel@web.de>          *
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


#include <Python.h>

#include <Base/Console.h>
#include <Base/FileInfo.h>

#include <App/Application.h>
#include <App/Document.h>

#include <Mod/Mesh/App/Core/MeshKernel.h>
#include <Mod/Mesh/App/Core/Elements.h>
#include <Mod/Mesh/App/MeshPy.h>
#include <Mod/Mesh/App/MeshFeature.h>
#include <App/PartPy.h>


#include "TestJtReader.h"
#include "JcadLibReader.h"

using std::vector;
using namespace MeshCore;


//using namespace JtReader;

/* module functions */
static PyObject * read(PyObject *self, PyObject *args)
{
  char* Name;
  if (!PyArg_ParseTuple(args, "et","utf-8",&Name))
        return NULL;
  std::string EncodedName = std::string(Name);
  PyMem_Free(Name);

  PY_TRY {
    //std::auto_ptr<MeshCore::MeshKernel> apcKernel(new MeshCore::MeshKernel());

    //vector<MeshGeomFacet> facets;
    //facets.resize(0 /* some size*/);

    //const SimpleMeshFacet* It=iterStart();
    //int i=0;
    //while(It=iterGetNext())
    //{
    //    facets[i]._aclPoints[0].x = It->p1[0];
    //    facets[i]._aclPoints[0].y = It->p1[1];
    //    facets[i]._aclPoints[0].z = It->p1[2];
    //    facets[i]._aclPoints[1].x = It->p2[0];
    //    facets[i]._aclPoints[1].y = It->p2[1];
    //    facets[i]._aclPoints[1].z = It->p2[2];
    //    facets[i]._aclPoints[2].x = It->p3[0];
    //    facets[i]._aclPoints[2].y = It->p3[1];
    //    facets[i]._aclPoints[2].z = It->p3[2];
    //}
   
    //(*apcKernel) = facets;  

    //return new Mesh::MeshPy(new Mesh::MeshObject(*(apcKernel.release())));

  } PY_CATCH;

	Py_Return;    
}

static PyObject *
open(PyObject *self, PyObject *args)
{
  char* Name;
  if (!PyArg_ParseTuple(args, "et","utf-8",&Name))
        return NULL;
  std::string EncodedName = std::string(Name);
  PyMem_Free(Name);

  PY_TRY {

    //Base::Console().Log("Open in Mesh with %s",Name);
    Base::FileInfo file(EncodedName.c_str());

    // extract ending
    if(file.extension() == "")
      Py_Error(Base::BaseExceptionFreeCADError,"no file ending");

    if(file.hasExtension("jt"))
    {

		TestJtReader reader;

		reader.setFile(EncodedName.c_str());

		reader.read();


        // create new document and add Import feature
       // App::Document *pcDoc = App::GetApplication().newDocument("Unnamed");
       // Mesh::Feature *pcFeature = (Mesh::Feature*)pcDoc->addObject("Mesh::Feature",file.fileNamePure().c_str());
       //   
       // std::auto_ptr<MeshCore::MeshKernel> apcKernel(new MeshCore::MeshKernel());

       // readFile(EncodedName.c_str(),0);

       // vector<MeshGeomFacet> facets;
       // facets.resize(iterSize());

       // const SimpleMeshFacet* It=iterStart();
       // int i=0;
       // while(It=iterGetNext())
       // {
       //     facets[i]._aclPoints[0].x = It->p1[0];
       //     facets[i]._aclPoints[0].y = It->p1[1];
       //     facets[i]._aclPoints[0].z = It->p1[2];
       //     facets[i]._aclPoints[1].x = It->p2[0];
       //     facets[i]._aclPoints[1].y = It->p2[1];
       //     facets[i]._aclPoints[1].z = It->p2[2];
       //     facets[i]._aclPoints[2].x = It->p3[0];
       //     facets[i]._aclPoints[2].y = It->p3[1];
       //     facets[i]._aclPoints[2].z = It->p3[2];
       //     i++;
       // }
       // clearData();
       // (*apcKernel) = facets; 
       // pcFeature->Mesh.setValue(*(apcKernel.get()));

       ////pcFeature->FileName.setValue( Name );
       // pcDoc->recompute();
    }
    else
    {
      Py_Error(Base::BaseExceptionFreeCADError,"unknown file ending");
    }


  } PY_CATCH;

	Py_Return;    
}


/* module functions */
static PyObject *
insert(PyObject *self, PyObject *args)
{
  char* Name;
  const char* DocName;
  if (!PyArg_ParseTuple(args, "ets","utf-8",&Name,&DocName))
        return NULL;
  std::string EncodedName = std::string(Name);
  PyMem_Free(Name);

  PY_TRY {

    Base::FileInfo file(EncodedName.c_str());

    // extract ending
    if(file.extension() == "")
      Py_Error(Base::BaseExceptionFreeCADError,"no file ending");

    if(file.hasExtension("jt") )
    {
        // add Import feature
        App::Document *pcDoc = App::GetApplication().getDocument(DocName);
        if (!pcDoc)
        {
            char szBuf[200];
            snprintf(szBuf, 200, "Import called to the non-existing document '%s'", DocName);
            Py_Error(Base::BaseExceptionFreeCADError,szBuf);
        }

        //readFile(EncodedName.c_str(),0);

        //vector<MeshGeomFacet> facets;

        //if(iterSize()>0){
        //    facets.resize(iterSize());

        //    const SimpleMeshFacet* It=iterStart();
        //    int i=0;
        //    while(It=iterGetNext())
        //    {
        //        facets[i]._aclPoints[0].x = It->p1[0];
        //        facets[i]._aclPoints[0].y = It->p1[1];
        //        facets[i]._aclPoints[0].z = It->p1[2];
        //        facets[i]._aclPoints[1].x = It->p2[0];
        //        facets[i]._aclPoints[1].y = It->p2[1];
        //        facets[i]._aclPoints[1].z = It->p2[2];
        //        facets[i]._aclPoints[2].x = It->p3[0];
        //        facets[i]._aclPoints[2].y = It->p3[1];
        //        facets[i]._aclPoints[2].z = It->p3[2];
        //        i++;
        //    }
        //    clearData();
        //    Mesh::Feature *pcFeature = (Mesh::Feature*)pcDoc->addObject("Mesh::Feature",file.fileNamePure().c_str());
        //  
        //    std::auto_ptr<MeshCore::MeshKernel> apcKernel(new MeshCore::MeshKernel());
        //   (*apcKernel) = facets; 
        //    pcFeature->Mesh.setValue(*(apcKernel.get()));

        //    //pcDoc->recompute();

        //}else{
        //    clearData();
        //    //Py_Error(Base::BaseExceptionFreeCADError,"No Mesh in file");
        //    Base::Console().Warning("No Mesh in file: %s\n",EncodedName.c_str());
        //}
     }
    else
    {
      Py_Error(Base::BaseExceptionFreeCADError,"unknown file ending");
    }

  } PY_CATCH;

	Py_Return;    
}

static PyObject * readJtPart(PyObject *self, PyObject *args)
{
    char* Name;
    PyObject* PartObject = 0;
    if (!PyArg_ParseTuple(args, "etO!", "utf-8", &Name, &(App::PartPy::Type), &PartObject))
        return 0;
    std::string Utf8Name = std::string(Name);
    PyMem_Free(Name);
    App::Part* Part = static_cast<App::PartPy*>(PartObject)->getPartPtr();

    PY_TRY{

        JcadLibReader reader;

        reader.read(Utf8Name.c_str());

        if (reader.getFaceCount() == 1){

            const JcadLibReader::Buffer &buf = reader.getBuffer(0);


            MeshPointArray meshPoints;// (buf.Vertexes.size());
            MeshFacetArray meshFacets;// (buf.Indexes.size() / 3);

            for (auto verts : buf.Vertexes){
                meshPoints.push_back(MeshPoint(Base::Vector3f(verts.vec[0], verts.vec[1], verts.vec[2])));
            }
            for (std::vector<int32_t>::const_iterator it = buf.Indexes.begin(); it != buf.Indexes.end(); it += 3){
                meshFacets.push_back(MeshFacet(*it, *(it + 1), *(it + 2)));
            }
 
            MeshKernel tmp;
            tmp.Adopt(meshPoints, meshFacets,true);

            App::Document *doc = Part->getDocument();
            App::DocumentObject *obj = doc->addObject("Mesh::Feature", reader.getName(0).toStdString().c_str());

            Part->addObject(obj);

            Mesh::Feature *meshObj = dynamic_cast<Mesh::Feature*>(obj);

            meshObj->Mesh.setValue(tmp);

        }

 
        Base::Console().Message(reader.getLog().toStdString().c_str());



    }
        PY_CATCH

        Py_Return;
}

/* registration table  */
struct PyMethodDef JtReader_methods[] = {
    {"open"       ,open ,       Py_NEWARGS, "open a jt file in a new Document"},				
    {"insert"     ,insert,      Py_NEWARGS, "isert a jt file in a existing document"},
    {"read"       ,read,        Py_NEWARGS, "Read a Mesh from a jt file and returns a Mesh object."},
    { "readJtPart", readJtPart, METH_VARARGS,
    "readJtPart(JtFilePath,PartObject) -- Read a Jt into a PartObject" },
    { NULL, NULL }
};


