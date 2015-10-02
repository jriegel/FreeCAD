/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de)          *
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

#include <Base/Writer.h>
#include <iostream>

#include "Document.h"
#include "DocumentObject.h"
#include "DocumentObjectPy.h"
#include "DocumentObjectGroup.h"
#include "PropertyLinks.h"

using namespace App;
using namespace std;


PROPERTY_SOURCE(App::DocumentObject, App::PropertyContainer)



DocumentObjectExecReturn *DocumentObject::StdReturn = 0;

//===========================================================================
// DocumentObject
//===========================================================================

DocumentObject::DocumentObject(void)
  : _pDoc(0),pcNameInDocument(0)
{
    // define Label of type 'Output' to avoid being marked as touched after relabeling
    ADD_PROPERTY_TYPE(Label,("Unnamed"),"Base",Prop_Output,"User name of the object (UTF8)");
}

DocumentObject::~DocumentObject(void)
{
    if (!PythonObject.is(Py::_None())){
        // Remark: The API of Py::Object has been changed to set whether the wrapper owns the passed 
        // Python object or not. In the constructor we forced the wrapper to own the object so we need
        // not to dec'ref the Python object any more.
        // But we must still invalidate the Python object because it need not to be
        // destructed right now because the interpreter can own several references to it.
        Base::PyObjectBase* obj = (Base::PyObjectBase*)PythonObject.ptr();
        // Call before decrementing the reference counter, otherwise a heap error can occur
        obj->setInvalid();
    }
}

namespace App {
class ObjectExecution
{
public:
    ObjectExecution(DocumentObject* o) : obj(o)
    { obj->StatusBits.set(3); }
    ~ObjectExecution()
    { obj->StatusBits.reset(3); }
private:
    DocumentObject* obj;
};
}

App::DocumentObjectExecReturn *DocumentObject::recompute(void)
{
    // set/unset the execution bit
    ObjectExecution exe(this);
    return this->execute();
}

DocumentObjectExecReturn *DocumentObject::execute(void)
{
    return DocumentObject::StdReturn;
}

short DocumentObject::mustExecute(void) const
{
    return (isTouched() ? 1 : 0);
}

const char* DocumentObject::getStatusString(void) const
{
    if (isError()) {
        const char* text = getDocument()->getErrorDescription(this);
        return text ? text : "Error";
    }
    else if (isTouched())
        return "Touched";
    else
        return "Valid";
}

const char *DocumentObject::getNameInDocument(void) const
{
    // Note: It can happen that we query the internal name of an object even if it is not
    // part of a document (anymore). This is the case e.g. if we have a reference in Python 
    // to an object that has been removed from the document. In this case we should rather
    // return 0.
    //assert(pcNameInDocument);
    if (!pcNameInDocument) return 0;
    return pcNameInDocument->c_str();
}

vector<DocumentObject*> DocumentObject::getOutList(void) const
{
    vector<Property*> List;
    List.reserve(15);

    // get all Properties
    getPropertyList(List);

    // count out list size to avoid resize of the vector while collecting
    size_t outListSize = 0;
    for (vector<Property*>::const_iterator It = List.begin(); It != List.end(); ++It) {
        if ((*It)->isDerivedFrom(PropertyLinkList::getClassTypeId())) {
            outListSize += static_cast<PropertyLinkList*>(*It)->getSize();
        }
        else if ((*It)->isDerivedFrom(PropertyLinkSubList::getClassTypeId())) {
            outListSize += static_cast<PropertyLinkSubList*>(*It)->getSize();
        }
        else if ((*It)->isDerivedFrom(PropertyLink::getClassTypeId())) {
            if (static_cast<PropertyLink*>(*It)->getValue())
                outListSize += 1;
        }
        else if ((*It)->isDerivedFrom(PropertyLinkSub::getClassTypeId())) {
            if (static_cast<PropertyLinkSub*>(*It)->getValue())
                outListSize += 1;
        }
    }

    // create the vector in the right size and fill it up
    vector<DocumentObject*> returnVector;
    returnVector.reserve(outListSize);

    for (vector<Property*>::const_iterator It = List.begin();It != List.end(); ++It) {
        if ((*It)->isDerivedFrom(PropertyLinkList::getClassTypeId())) {
            const vector<DocumentObject*> &OutList = static_cast<PropertyLinkList*>(*It)->getValues();
            for (vector<DocumentObject*>::const_iterator It2 = OutList.begin();It2 != OutList.end(); ++It2) {
                if (*It2)
                    returnVector.push_back(*It2);
            }
        }
        else if ((*It)->isDerivedFrom(PropertyLinkSubList::getClassTypeId())) {
            const vector<DocumentObject*> &OutList = static_cast<PropertyLinkSubList*>(*It)->getValues();
            for (vector<DocumentObject*>::const_iterator It2 = OutList.begin();It2 != OutList.end(); ++It2) {
                if (*It2)
                    returnVector.push_back(*It2);
            }
        }
        else if ((*It)->isDerivedFrom(PropertyLink::getClassTypeId())) {
            if (static_cast<PropertyLink*>(*It)->getValue())
                returnVector.push_back(static_cast<PropertyLink*>(*It)->getValue());
        }
        else if ((*It)->isDerivedFrom(PropertyLinkSub::getClassTypeId())) {
            if (static_cast<PropertyLinkSub*>(*It)->getValue())
                returnVector.push_back(static_cast<PropertyLinkSub*>(*It)->getValue());
        }
    }
    return returnVector;
}

// The new DAG handling uses pre-computed vectors for the InList...
#if USE_OLD_DAG
 
vector<App::DocumentObject*> DocumentObject::getInList(void) const
{
	vector<App::DocumentObject*> ret;
    if (_pDoc)
		ret = _pDoc->getInList(this);

	// in case of debug and the old DAG handling we check if the new mimic and the old one delivers exactly the same values!
	// But not necessary in the same order....
#	if _DEBUG 
	{
		if (ret.size() != _inList.size()){
			cerr << "DocumentObject::getInList(): old and new differ in size";
		}
		else{
			vector<App::DocumentObject*> oldInList(ret);
			vector<App::DocumentObject*> newInList(_inList);
			sort(oldInList.begin(), oldInList.end());
			sort(newInList.begin(), newInList.end());
            if (oldInList != newInList){
                cerr << "DocumentObject::getInList(): old and new differ in values";
                assert(0);
            }
		}
    }
#	endif
	return ret;
}

#else // if USE_OLD_DAG

vector<App::DocumentObject*> DocumentObject::getInList(void) const
{
    return _inList;
}


#endif // if USE_OLD_DAG

void _getInListRecursive(vector<DocumentObject*>& objSet, const DocumentObject* obj, const DocumentObject* checkObj, int depth)
{
    for (const auto objIt : obj->getInList()){
        // if the check object is in the recursive inList we have a cycle!
        if (objIt == checkObj || depth <= 0){
            cerr << "DocumentObject::getInListRecursive(): cyclic dependency detected!"<<endl;
            throw Base::Exception("DocumentObject::getInListRecursive(): cyclic dependency detected!");
        }

        objSet.push_back(objIt);
        _getInListRecursive(objSet, objIt, checkObj,depth-1);
    }
}

vector<App::DocumentObject*> DocumentObject::getInListRecursive(void) const
{
    // number of objects in document is a good estimate in result size
    int maxDepth = getDocument()->countObjects() +2;
    vector<App::DocumentObject*> result;
    result.reserve(maxDepth);

    // using a rcursie helper to collect all InLists
    _getInListRecursive(result, this, this, maxDepth);

    // remove duplicate entries and resize the vector
    sort(result.begin(), result.end());
    auto newEnd = unique(result.begin(), result.end());
    result.resize(distance(result.begin(), newEnd));

    return result;
}

void _getOutListRecursive(vector<DocumentObject*>& objSet, const DocumentObject* obj, const DocumentObject* checkObj, int depth)
{
    for (const auto objIt : obj->getOutList()){
        // if the check object is in the recursive inList we have a cycle!
        if (objIt == checkObj || depth <= 0){
            cerr << "DocumentObject::getOutListRecursive(): cyclic dependency detected!" << endl;
            throw Base::Exception("DocumentObject::getOutListRecursive(): cyclic dependency detected!");
        }
        objSet.push_back(objIt);
        _getOutListRecursive(objSet, objIt, checkObj,depth-1);
    }
}

vector<App::DocumentObject*> DocumentObject::getOutListRecursive(void) const
{
    // number of objects in document is a good estimate in result size
    int maxDepth = getDocument()->countObjects() + 2;
    vector<App::DocumentObject*> result;
    result.reserve(maxDepth);

    // using a recursive helper to collect all OutLists
    _getOutListRecursive(result, this, this, maxDepth);

    // remove duplicate entries and resize the vector
    sort(result.begin(), result.end());
    auto newEnd = unique(result.begin(), result.end());
    result.resize(distance(result.begin(), newEnd));

    return result;
}

DocumentObjectGroup* DocumentObject::getGroup() const
{
    return DocumentObjectGroup::getGroupOfObject(this);
}

bool DocumentObject::_isInInListRecursive(const DocumentObject *act, const DocumentObject* test) const
{
    if (find(_inList.begin(), _inList.end(), test) != _inList.end())
        return true;

    for (auto obj : _inList){
        if (_isInInListRecursive(obj, test))
            return true;
    }

    return false;
}

bool DocumentObject::isInInListRecursive(DocumentObject *linkTo) const
{
    return _isInInListRecursive(this,linkTo);
}

bool DocumentObject::isInInList(DocumentObject *linkTo) const
{
    if (find(_inList.begin(), _inList.end(), linkTo) != _inList.end())
        return true;
    else
        return false;
}

#if USE_OLD_DAG
bool DocumentObject::testIfLinkDAGCompatible(const vector<DocumentObject *> &linksTo) const
{
    Document* doc = this->getDocument();
    if (!doc)
        throw Base::Exception("DocumentObject::testIfLinkIsDAG: object is not in any document.");
    vector<App::DocumentObject*> deplist = doc->getDependencyList(linksTo);
    if( find(deplist.begin(),deplist.end(),this) != deplist.end() )
        //found this in dependency list
        return false;
    else
        return true;
}

bool DocumentObject::testIfLinkDAGCompatible(PropertyLinkSubList &linksTo) const
{
    const vector<App::DocumentObject*> &linksTo_in_vector = linksTo.getValues();
    return this->testIfLinkDAGCompatible(linksTo_in_vector);
}

bool DocumentObject::testIfLinkDAGCompatible(PropertyLinkSub &linkTo) const
{
    vector<App::DocumentObject*> linkTo_in_vector;
    linkTo_in_vector.reserve(1);
    linkTo_in_vector.push_back(linkTo.getValue());
    return this->testIfLinkDAGCompatible(linkTo_in_vector);
}
#endif USE_OLD_DAG

void DocumentObject::onLostLinkToObject(DocumentObject*)
{

}

App::Document *DocumentObject::getDocument(void) const
{
    return _pDoc;
}

void DocumentObject::setDocument(App::Document* doc)
{
    _pDoc=doc;
    onSettingDocument();
}

void DocumentObject::onBeforeChange(const Property* prop)
{
    if (_pDoc)
        _pDoc->onBeforeChangeProperty(this,prop);
}

/// get called by the container when a Property was changed
void DocumentObject::onChanged(const Property* prop)
{
    if (_pDoc)
        _pDoc->onChangedProperty(this,prop);
    if (prop->getType() & Prop_Output)
        return;
    // set object touched
    StatusBits.set(0);
}

PyObject *DocumentObject::getPyObject(void)
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new DocumentObjectPy(this),true);
    }
    return Py::new_reference_to(PythonObject); 
}

vector<PyObject *> DocumentObject::getPySubObjects(const vector<string>&) const
{
    // default implementation returns nothing
    return vector<PyObject *>();
}

void DocumentObject::touch(void)
{
    StatusBits.set(0);
}

void DocumentObject::Save (Base::Writer &writer) const
{
    writer.ObjectName = this->getNameInDocument();
    App::PropertyContainer::Save(writer);
}

void App::DocumentObject::_removeBackLink(DocumentObject* rmfObj)
{
	_inList.erase(remove(_inList.begin(), _inList.end(), rmfObj), _inList.end());
}

void App::DocumentObject::_addBackLink(DocumentObject* newObje)
{
	_inList.push_back(newObje);
}