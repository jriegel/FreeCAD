/***************************************************************************
*   Copyright (c) 2015 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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



#ifndef COIN_SoObjectSeparator_H
#define COIN_SoObjectSeparator_H


#include <Inventor/nodes/SoSeparator.h>


namespace Gui {

class ViewProvider;

/** special seperator for the head of ViewProvider
 * 
 * @author Juergen Riegel
 */
class GuiExport SoObjectSeparator : public SoSeparator {
    typedef SoSeparator inherited;

    SO_NODE_HEADER(Gui::SoObjectSeparator);

public:
    static void initClass(void);
    static void finish(void);
    SoObjectSeparator(Gui::ViewProvider *vp = NULL);

    Gui::ViewProvider *getViewProvider(){ return viewProvider; }
 
protected:
    virtual ~SoObjectSeparator();
    Gui::ViewProvider * viewProvider;
};

} // namespace Gui


#endif //COIN_SoObjectSeparator_H
