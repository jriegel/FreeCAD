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


#ifndef APP_DocumentGraph_H
#define APP_DocumentGraph_H

#include <boost/graph/topological_sort.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/visitors.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/bind.hpp>
#include <boost/regex.hpp>
#include <boost/unordered_set.hpp>

using namespace boost;

// typedef boost::property<boost::vertex_root_t, DocumentObject* > VertexProperty;
typedef boost::adjacency_list <
    boost::vecS,           // class OutEdgeListS  : a Sequence or an AssociativeContainer
    boost::vecS,           // class VertexListS   : a Sequence or a RandomAccessContainer
    boost::directedS,      // class DirectedS     : This is a directed graph
    boost::no_property,    // class VertexProperty:
    boost::no_property,    // class EdgeProperty:
    boost::no_property,    // class GraphProperty:
    boost::listS           // class EdgeListS:
> DependencyList;
typedef boost::graph_traits<DependencyList> Traits;
typedef Traits::vertex_descriptor Vertex;
typedef Traits::edge_descriptor Edge;


namespace App
{

/** Graph book keeping class for the document
 */
class AppExport DocumentGraph
{
    // adjancy list of the objects in the document
    std::map<DocumentObject*, Vertex> VertexObjectList;
    DependencyList DepList;

public:

 
    /// Constructor
    DocumentGraph(void);
    virtual ~DocumentGraph();


};

} //namespace App


#endif // APP_DocumentGraph_H
