//  Copyright (C) 2007-2008  CEA/DEN, EDF R&D, OPEN CASCADE
//
//  Copyright (C) 2003-2007  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
//  CEDRAT, EDF R&D, LEG, PRINCIPIA R&D, BUREAU VERITAS
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2.1 of the License.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
//
//  See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
//
//  SMESH SMESH : idl implementation based on 'SMESH' unit's calsses
// File      : StdMeshers_ProjectionUtils.cxx
// Created   : Fri Oct 27 10:24:28 2006
// Author    : Edward AGAPOV (eap)
//
#include "StdMeshers_ProjectionUtils.hxx"

#include "StdMeshers_ProjectionSource1D.hxx"
#include "StdMeshers_ProjectionSource2D.hxx"
#include "StdMeshers_ProjectionSource3D.hxx"

#include "SMESH_Algo.hxx"
#include "SMESH_Block.hxx"
#include "SMESH_Gen.hxx"
#include "SMESH_Hypothesis.hxx"
#include "SMESH_IndexedDataMapOfShapeIndexedMapOfShape.hxx"
#include "SMESH_Mesh.hxx"
#include "SMESH_MesherHelper.hxx"
#include "SMESH_subMesh.hxx"
#include "SMESH_subMeshEventListener.hxx"
#include "SMDS_EdgePosition.hxx"

#include "utilities.h"

#include <BRepTools.hxx>
#include <BRepTools_WireExplorer.hxx>
#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <Bnd_Box.hxx>
#include <TopAbs.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_Array1OfShape.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>
#include <TopTools_ListOfShape.hxx>
#include <TopTools_MapOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Shape.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>
#include <TopTools_DataMapIteratorOfDataMapOfShapeShape.hxx>
#include <TopTools_DataMapIteratorOfDataMapOfShapeListOfShape.hxx>

using namespace std;


#define RETURN_BAD_RESULT(msg) { MESSAGE(")-: Error: " << msg); return false; }
#define SHOW_VERTEX(v,msg) // { \
//  if ( v.IsNull() ) cout << msg << " NULL SHAPE" << endl; \
// else if (v.ShapeType() == TopAbs_VERTEX) {\
//   gp_Pnt p = BRep_Tool::Pnt( TopoDS::Vertex( v ));\
//   cout << msg << (v).TShape().operator->()<<" ( " <<p.X()<<", "<<p.Y()<<", "<<p.Z()<<" )"<<endl;}\
// else {\
// cout << msg << " "; TopAbs::Print(v.ShapeType(),cout) <<" "<<(v).TShape().operator->()<<endl;}\
// }
#define SHOW_LIST(msg,l) \
// { \
//     cout << msg << " ";\
//     list< TopoDS_Edge >::const_iterator e = l.begin();\
//     for ( int i = 0; e != l.end(); ++e, ++i ) {\
//       cout << i << "V (" << TopExp::FirstVertex( *e, true ).TShape().operator->() << ") "\
//            << i << "E (" << e->TShape().operator->() << "); "; }\
//     cout << endl;\
//   }

#define HERE StdMeshers_ProjectionUtils

namespace {

  //================================================================================
  /*!
   * \brief Write shape for debug purposes
   */
  //================================================================================

  bool _StoreBadShape(const TopoDS_Shape& shape)
  {
#ifdef _DEBUG_
    const char* type[] ={"COMPOUND","COMPSOLID","SOLID","SHELL","FACE","WIRE","EDGE","VERTEX"};
    BRepTools::Write( shape, SMESH_Comment("/tmp/") << type[shape.ShapeType()] << "_"
                      << shape.TShape().operator->() << ".brep");
#endif
    return false;
  }
  
  //================================================================================
  /*!
   * \brief Reverse order of edges in a list and their orientation
    * \param edges - list of edges to reverse
    * \param nbEdges - number of edges to reverse
   */
  //================================================================================

  void Reverse( list< TopoDS_Edge > & edges, const int nbEdges )
  {
    SHOW_LIST("BEFORE REVERSE", edges);

    list< TopoDS_Edge >::iterator eIt = edges.begin();
    if ( edges.size() == nbEdges )
    {
      edges.reverse();
    }
    else  // reverse only the given nb of edges
    {
      // look for the last edge to be reversed
      list< TopoDS_Edge >::iterator eBackIt = edges.begin();
      for ( int i = 1; i < nbEdges; ++i )
        ++eBackIt;
      // reverse
      while ( eIt != eBackIt ) {
        std::swap( *eIt, *eBackIt );
        SHOW_LIST("# AFTER SWAP", edges)
        if ( (++eIt) != eBackIt )
          --eBackIt;
      }
    }
    for ( eIt = edges.begin(); eIt != edges.end(); ++eIt )
      eIt->Reverse();
    SHOW_LIST("ATFER REVERSE", edges)
  }

  //================================================================================
  /*!
   * \brief Check if propagation is possible
    * \param theMesh1 - source mesh
    * \param theMesh2 - target mesh
    * \retval bool - true if possible
   */
  //================================================================================

  bool IsPropagationPossible( SMESH_Mesh* theMesh1, SMESH_Mesh* theMesh2 )
  {
    if ( theMesh1 != theMesh2 ) {
      TopoDS_Shape mainShape1 = theMesh1->GetMeshDS()->ShapeToMesh();
      TopoDS_Shape mainShape2 = theMesh2->GetMeshDS()->ShapeToMesh();
      return mainShape1.IsSame( mainShape2 );
    }
    return true;
  }

  //================================================================================
  /*!
   * \brief Fix up association of edges in faces by possible propagation
    * \param nbEdges - nb of edges in an outer wire
    * \param edges1 - edges of one face
    * \param edges2 - matching edges of another face
    * \param theMesh1 - mesh 1
    * \param theMesh2 - mesh 2
    * \retval bool - true if association was fixed
   */
  //================================================================================

  bool FixAssocByPropagation( const int             nbEdges,
                              list< TopoDS_Edge > & edges1,
                              list< TopoDS_Edge > & edges2,
                              SMESH_Mesh*           theMesh1,
                              SMESH_Mesh*           theMesh2)
  {
    if ( nbEdges == 2 && IsPropagationPossible( theMesh1, theMesh2 ) )
    {
      list< TopoDS_Edge >::iterator eIt2 = ++edges2.begin(); // 2nd edge of the 2nd face
      TopoDS_Edge edge2 = HERE::GetPropagationEdge( theMesh1, *eIt2, edges1.front() ).second;
      if ( !edge2.IsNull() ) { // propagation found for the second edge
        Reverse( edges2, nbEdges );
        return true;
      }
    }
    return false;
  }

  //================================================================================
  /*!
   * \brief Look for a group containing a target shape and similar to a source group
    * \param tgtShape - target edge or face
    * \param tgtMesh1 - target mesh
    * \param srcGroup - source group
    * \retval TopoDS_Shape - found target group
   */
  //================================================================================

  TopoDS_Shape FindGroupContaining(const TopoDS_Shape& tgtShape,
                                   const SMESH_Mesh*   tgtMesh1,
                                   const TopoDS_Shape& srcGroup)
  {
    list<SMESH_subMesh*> subMeshes = tgtMesh1->GetGroupSubMeshesContaining(tgtShape);
    list<SMESH_subMesh*>::iterator sm = subMeshes.begin();
    int type, last = TopAbs_SHAPE;
    StdMeshers_ProjectionUtils util;
    for ( ; sm != subMeshes.end(); ++sm ) {
      const TopoDS_Shape & group = (*sm)->GetSubShape();
      // check if group is similar to srcGroup
      for ( type = srcGroup.ShapeType(); type < last; ++type)
        if ( util.Count( srcGroup, (TopAbs_ShapeEnum)type, 0) !=
             util.Count( group,    (TopAbs_ShapeEnum)type, 0))
          break;
      if ( type == last )
        return group;
    }
    return TopoDS_Shape();
  }

  //================================================================================
  /*!
   * \brief Find association of groups at top and bottom of prism
   */
  //================================================================================

  bool AssocGroupsByPropagation(const TopoDS_Shape&   theGroup1,
                                const TopoDS_Shape&   theGroup2,
                                SMESH_Mesh&           theMesh,
                                HERE::TShapeShapeMap& theMap)
  {
    // If groups are on top and bottom of prism then we can associate
    // them using "vertical" (or "side") edges and faces of prism since
    // they connect corresponding vertices and edges of groups.

    TopTools_IndexedMapOfShape subshapes1, subshapes2;
    TopExp::MapShapes( theGroup1, subshapes1 );
    TopExp::MapShapes( theGroup2, subshapes2 );
    TopTools_ListIteratorOfListOfShape ancestIt;

    // Iterate on vertices of group1 to find corresponding vertices in group2
    // and associate adjacent edges and faces

    TopTools_MapOfShape verticShapes;
    TopExp_Explorer vExp1( theGroup1, TopAbs_VERTEX );
    for ( ; vExp1.More(); vExp1.Next() )
    {
      const TopoDS_Vertex& v1 = TopoDS::Vertex( vExp1.Current() );
      if ( theMap.IsBound( v1 )) continue; // already processed

      // Find "vertical" edge ending in v1 and whose other vertex belongs to group2
      TopoDS_Shape verticEdge, v2;
      ancestIt.Initialize( theMesh.GetAncestors( v1 ));
      for ( ; verticEdge.IsNull() && ancestIt.More(); ancestIt.Next() )
      {
        if ( ancestIt.Value().ShapeType() != TopAbs_EDGE ) continue;
        v2 = HERE::GetNextVertex( TopoDS::Edge( ancestIt.Value() ), v1 );
        if ( subshapes2.Contains( v2 ))
          verticEdge = ancestIt.Value();
      }
      if ( verticEdge.IsNull() )
        return false;

      HERE::InsertAssociation( v1, v2, theMap);

      // Associate edges by vertical faces sharing the found vertical edge
      ancestIt.Initialize( theMesh.GetAncestors( verticEdge ) );
      for ( ; ancestIt.More(); ancestIt.Next() )
      {
        if ( ancestIt.Value().ShapeType() != TopAbs_FACE ) continue;
        if ( !verticShapes.Add( ancestIt.Value() )) continue;
        const TopoDS_Face& face = TopoDS::Face( ancestIt.Value() );

        // get edges of the face
        TopoDS_Edge edgeGr1, edgeGr2, verticEdge2;
        list< TopoDS_Edge > edges;    list< int > nbEdgesInWire;
        SMESH_Block::GetOrderedEdges( face, v1, edges, nbEdgesInWire);
        if ( nbEdgesInWire.front() != 4 )
          return _StoreBadShape( face );
        list< TopoDS_Edge >::iterator edge = edges.begin();
        if ( verticEdge.IsSame( *edge )) {
          edgeGr2     = *(++edge);
          verticEdge2 = *(++edge);
          edgeGr1     = *(++edge);
        } else {
          edgeGr1     = *(edge++);
          verticEdge2 = *(edge++);
          edgeGr2     = *(edge++);
        }

        HERE::InsertAssociation( edgeGr1, edgeGr2.Reversed(), theMap);
      }
    }

    // Associate faces
    TopoDS_Iterator gr1It( theGroup1 );
    if ( gr1It.Value().ShapeType() == TopAbs_FACE )
    {
      // find a boundary edge of group1 to start from
      TopoDS_Shape bndEdge;
      TopExp_Explorer edgeExp1( theGroup1, TopAbs_EDGE );
      for ( ; bndEdge.IsNull() && edgeExp1.More(); edgeExp1.Next())
        if ( HERE::IsBoundaryEdge( TopoDS::Edge( edgeExp1.Current()), theGroup1, theMesh ))
          bndEdge = edgeExp1.Current();
      if ( bndEdge.IsNull() )
        return false;

      list< TopoDS_Shape > edges(1, bndEdge);
      list< TopoDS_Shape >::iterator edge1 = edges.begin();
      for ( ; edge1 != edges.end(); ++edge1 )
      {
        // there must be one or zero not associated faces between ancestors of edge
        // belonging to theGroup1
        TopoDS_Shape face1;
        ancestIt.Initialize( theMesh.GetAncestors( *edge1 ) );
        for ( ; ancestIt.More() && face1.IsNull(); ancestIt.Next() ) {
          if ( ancestIt.Value().ShapeType() == TopAbs_FACE &&
               !theMap.IsBound( ancestIt.Value() ) &&
               subshapes1.Contains( ancestIt.Value() ))
            face1 = ancestIt.Value();

          // add edges of face1 to start searching for adjacent faces from
          for ( TopExp_Explorer e(face1, TopAbs_EDGE); e.More(); e.Next())
            if ( !edge1->IsSame( e.Current() ))
              edges.push_back( e.Current() );
        }
        if ( !face1.IsNull() ) {
          // find the corresponding face of theGroup2
          TopoDS_Shape edge2 = theMap( *edge1 );
          TopoDS_Shape face2;
          ancestIt.Initialize( theMesh.GetAncestors( edge2 ) );
          for ( ; ancestIt.More() && face2.IsNull(); ancestIt.Next() ) {
            if ( ancestIt.Value().ShapeType() == TopAbs_FACE &&
                 !theMap.IsBound( ancestIt.Value() ) &&
                 subshapes2.Contains( ancestIt.Value() ))
              face2 = ancestIt.Value();
          }
          if ( face2.IsNull() )
            return false;

          HERE::InsertAssociation( face1, face2, theMap);
        }
      }
    }
    return true;
  }

} // namespace

//=======================================================================
/*!
 * \brief Looks for association of all subshapes of two shapes
 * \param theShape1 - shape 1
 * \param theMesh1 - mesh built on shape 1
 * \param theShape2 - shape 2
 * \param theMesh2 - mesh built on shape 2
 * \param theAssociation - association map to be filled that may
 *                         contain association of one or two pairs of vertices
 * \retval bool - true if association found
 */
//=======================================================================

bool StdMeshers_ProjectionUtils::FindSubShapeAssociation(const TopoDS_Shape& theShape1,
                                                         SMESH_Mesh*         theMesh1,
                                                         const TopoDS_Shape& theShape2,
                                                         SMESH_Mesh*         theMesh2,
                                                         TShapeShapeMap &    theMap)
{
  if ( theShape1.ShapeType() != theShape2.ShapeType() ) {
    // is it the case of a group member -> another group? (PAL16202, 16203)
    TopoDS_Shape group1, group2;
    if ( theShape1.ShapeType() == TopAbs_COMPOUND ) {
      group1 = theShape1;
      group2 = FindGroupContaining( theShape2, theMesh2, group1 );
    }
    else if ( theShape2.ShapeType() == TopAbs_COMPOUND ) {
      group2 = theShape2;
      group1 = FindGroupContaining( theShape1, theMesh1, group2 );
    }
    if ( group1.IsNull() || group2.IsNull() )
      RETURN_BAD_RESULT("Different shape types");
    // Associate compounds
    return FindSubShapeAssociation(group1, theMesh1, group2, theMesh2, theMap );
  }

  bool bidirect = ( !theShape1.IsSame( theShape2 ));
  if ( !theMap.IsEmpty() )
  {
    //======================================================================
    // HAS initial vertex association
    //======================================================================
    switch ( theShape1.ShapeType() ) {
      // ----------------------------------------------------------------------
    case TopAbs_EDGE: { // TopAbs_EDGE
      // ----------------------------------------------------------------------
      if ( theMap.Extent() != 2 )
        RETURN_BAD_RESULT("Wrong map extent " << theMap.Extent() );
      TopoDS_Edge edge1 = TopoDS::Edge( theShape1 );
      TopoDS_Edge edge2 = TopoDS::Edge( theShape2 );
      TopoDS_Vertex VV1[2], VV2[2];
      TopExp::Vertices( edge1, VV1[0], VV1[1] );
      TopExp::Vertices( edge2, VV2[0], VV2[1] );
      int i1 = 0, i2 = 0;
      if ( theMap.IsBound( VV1[ i1 ] )) i1 = 1;
      if ( theMap.IsBound( VV2[ i2 ] )) i2 = 1;
      InsertAssociation( VV1[ i1 ], VV2[ i2 ], theMap, bidirect);
      InsertAssociation( theShape1, theShape2, theMap, bidirect );
      return true;
    }
      // ----------------------------------------------------------------------
    case TopAbs_FACE: { // TopAbs_FACE
      // ----------------------------------------------------------------------
      TopoDS_Face face1 = TopoDS::Face( theShape1 );
      TopoDS_Face face2 = TopoDS::Face( theShape2 );

      TopoDS_Vertex VV1[2], VV2[2];
      // find a not closed edge of face1 both vertices of which are associated
      int nbEdges = 0;
      TopExp_Explorer exp ( face1, TopAbs_EDGE );
      for ( ; VV2[ 1 ].IsNull() && exp.More(); exp.Next(), ++nbEdges ) {
        TopExp::Vertices( TopoDS::Edge( exp.Current() ), VV1[0], VV1[1] );
        if ( theMap.IsBound( VV1[0] ) ) {
          VV2[ 0 ] = TopoDS::Vertex( theMap( VV1[0] ));
          if ( theMap.IsBound( VV1[1] ) && !VV1[0].IsSame( VV1[1] ))
            VV2[ 1 ] = TopoDS::Vertex( theMap( VV1[1] ));
        }
      }
      if ( VV2[ 1 ].IsNull() ) { // 2 bound vertices not found
        if ( nbEdges > 1 ) {
          RETURN_BAD_RESULT("2 bound vertices not found" );
        } else {
          VV2[ 1 ] = VV2[ 0 ];
        }
      }
      list< TopoDS_Edge > edges1, edges2;
      int nbE = FindFaceAssociation( face1, VV1, face2, VV2, edges1, edges2 );
      if ( !nbE ) RETURN_BAD_RESULT("FindFaceAssociation() failed");
      FixAssocByPropagation( nbE, edges1, edges2, theMesh1, theMesh2 );

      list< TopoDS_Edge >::iterator eIt1 = edges1.begin();
      list< TopoDS_Edge >::iterator eIt2 = edges2.begin();
      for ( ; eIt1 != edges1.end(); ++eIt1, ++eIt2 )
      {
        InsertAssociation( *eIt1, *eIt2, theMap, bidirect);
        VV1[0] = TopExp::FirstVertex( *eIt1, true );
        VV2[0] = TopExp::FirstVertex( *eIt2, true );
        InsertAssociation( VV1[0], VV2[0], theMap, bidirect);
      }
      InsertAssociation( theShape1, theShape2, theMap, bidirect );
      return true;
    }
      // ----------------------------------------------------------------------
    case TopAbs_SHELL: // TopAbs_SHELL, TopAbs_SOLID
    case TopAbs_SOLID: {
      // ----------------------------------------------------------------------
      TopoDS_Vertex VV1[2], VV2[2];
      // try to find a not closed edge of shape1 both vertices of which are associated
      TopoDS_Edge edge1;
      TopExp_Explorer exp ( theShape1, TopAbs_EDGE );
      for ( ; VV2[ 1 ].IsNull() && exp.More(); exp.Next() ) {
        edge1 = TopoDS::Edge( exp.Current() );
        TopExp::Vertices( edge1 , VV1[0], VV1[1] );
        if ( theMap.IsBound( VV1[0] )) {
          VV2[ 0 ] = TopoDS::Vertex( theMap( VV1[0] ));
          if ( theMap.IsBound( VV1[1] ) && !VV1[0].IsSame( VV1[1] ))
            VV2[ 1 ] = TopoDS::Vertex( theMap( VV1[1] ));
        }
      }
      if ( VV2[ 1 ].IsNull() ) // 2 bound vertices not found
        RETURN_BAD_RESULT("2 bound vertices not found" );
      // get an edge2 of theShape2 corresponding to edge1
      TopoDS_Edge edge2 = GetEdgeByVertices( theMesh2, VV2[ 0 ], VV2[ 1 ]);
      if ( edge2.IsNull() )
        RETURN_BAD_RESULT("GetEdgeByVertices() failed");

      // build map of edge to faces if shapes are not subshapes of main ones
      bool isSubOfMain = false;
      if ( SMESHDS_SubMesh * sm = theMesh1->GetMeshDS()->MeshElements( theShape1 ))
        isSubOfMain = !sm->IsComplexSubmesh();
      else
        isSubOfMain = theMesh1->GetMeshDS()->ShapeToIndex( theShape1 );
      TAncestorMap e2f1, e2f2;
      const TAncestorMap& edgeToFace1 = isSubOfMain ? theMesh1->GetAncestorMap() : e2f1;
      const TAncestorMap& edgeToFace2 = isSubOfMain ? theMesh2->GetAncestorMap() : e2f2;
      if (!isSubOfMain) {
        TopExp::MapShapesAndAncestors( theShape1, TopAbs_EDGE, TopAbs_FACE, e2f1 );
        TopExp::MapShapesAndAncestors( theShape2, TopAbs_EDGE, TopAbs_FACE, e2f2 );
        if ( !edgeToFace1.Contains( edge1 ))
          RETURN_BAD_RESULT("edge1 does not belong to theShape1");
        if ( !edgeToFace2.Contains( edge2 ))
          RETURN_BAD_RESULT("edge2 does not belong to theShape2");
      }
      //
      // Look for 2 corresponing faces:
      //
      TopoDS_Shape F1, F2;

      // get a face sharing edge1 (F1)
      TopoDS_Shape FF2[2];
      TopTools_ListIteratorOfListOfShape ancestIt1( edgeToFace1.FindFromKey( edge1 ));
      for ( ; F1.IsNull() && ancestIt1.More(); ancestIt1.Next() )
        if ( ancestIt1.Value().ShapeType() == TopAbs_FACE )
          F1 = ancestIt1.Value().Oriented( TopAbs_FORWARD );
      if ( F1.IsNull() )
        RETURN_BAD_RESULT(" Face1 not found");

      // get 2 faces sharing edge2 (one of them is F2)
      TopTools_ListIteratorOfListOfShape ancestIt2( edgeToFace2.FindFromKey( edge2 ));
      for ( int i = 0; FF2[1].IsNull() && ancestIt2.More(); ancestIt2.Next() )
        if ( ancestIt2.Value().ShapeType() == TopAbs_FACE )
          FF2[ i++ ] = ancestIt2.Value().Oriented( TopAbs_FORWARD );

      // get oriented edge1 and edge2 from F1 and FF2[0]
      for ( exp.Init( F1, TopAbs_EDGE ); exp.More(); exp.Next() )
        if ( edge1.IsSame( exp.Current() )) {
          edge1 = TopoDS::Edge( exp.Current() );
          break;
        }
      for ( exp.Init( FF2[ 0 ], TopAbs_EDGE ); exp.More(); exp.Next() )
        if ( edge2.IsSame( exp.Current() )) {
          edge2 = TopoDS::Edge( exp.Current() );
          break;
        }

      // compare first vertices of edge1 and edge2
      TopExp::Vertices( edge1, VV1[0], VV1[1], true );
      TopExp::Vertices( edge2, VV2[0], VV2[1], true );
      F2 = FF2[ 0 ]; // (F2 !)
      if ( !VV1[ 0 ].IsSame( theMap( VV2[ 0 ]))) {
        edge2.Reverse();
        if ( FF2[ 1 ].IsNull() )
          F2.Reverse();
        else
          F2 = FF2[ 1 ];
      }

      TopTools_MapOfShape boundEdges;

      // association of face subshapes and neighbour faces
      list< pair < TopoDS_Face, TopoDS_Edge > > FE1, FE2;
      list< pair < TopoDS_Face, TopoDS_Edge > >::iterator fe1, fe2;
      FE1.push_back( make_pair( TopoDS::Face( F1 ), edge1 ));
      FE2.push_back( make_pair( TopoDS::Face( F2 ), edge2 ));
      for ( fe1 = FE1.begin(), fe2 = FE2.begin(); fe1 != FE1.end(); ++fe1, ++fe2 )
      {
        const TopoDS_Face& face1 = fe1->first;
        if ( theMap.IsBound( face1 ) ) continue;
        const TopoDS_Face& face2 = fe2->first;
        edge1 = fe1->second;
        edge2 = fe2->second;
        TopExp::Vertices( edge1, VV1[0], VV1[1], true );
        TopExp::Vertices( edge2, VV2[0], VV2[1], true );
        list< TopoDS_Edge > edges1, edges2;
        int nbE = FindFaceAssociation( face1, VV1, face2, VV2, edges1, edges2 );
        if ( !nbE ) RETURN_BAD_RESULT("FindFaceAssociation() failed");
        InsertAssociation( face1, face2, theMap, bidirect); // assoc faces
        MESSAGE("Assoc FACE " << theMesh1->GetMeshDS()->ShapeToIndex( face1 )<<
                " to "        << theMesh2->GetMeshDS()->ShapeToIndex( face2 ));
        if ( nbE == 2 && (edge1.IsSame( edges1.front())) != (edge2.IsSame( edges2.front())))
        {
          Reverse( edges2, nbE );
        }
        list< TopoDS_Edge >::iterator eIt1 = edges1.begin();
        list< TopoDS_Edge >::iterator eIt2 = edges2.begin();
        for ( ; eIt1 != edges1.end(); ++eIt1, ++eIt2 )
        {
          if ( !boundEdges.Add( *eIt1 )) continue; // already associated
          InsertAssociation( *eIt1, *eIt2, theMap, bidirect);  // assoc edges
          MESSAGE("Assoc edge " << theMesh1->GetMeshDS()->ShapeToIndex( *eIt1 )<<
                  " to "        << theMesh2->GetMeshDS()->ShapeToIndex( *eIt2 ));
          VV1[0] = TopExp::FirstVertex( *eIt1, true );
          VV2[0] = TopExp::FirstVertex( *eIt2, true );
          InsertAssociation( VV1[0], VV2[0], theMap, bidirect); // assoc vertices
          MESSAGE("Assoc vertex " << theMesh1->GetMeshDS()->ShapeToIndex( VV1[0] )<<
                  " to "          << theMesh2->GetMeshDS()->ShapeToIndex( VV2[0] ));

          // add adjacent faces to process
          TopoDS_Face nextFace1 = GetNextFace( edgeToFace1, *eIt1, face1 );
          TopoDS_Face nextFace2 = GetNextFace( edgeToFace2, *eIt2, face2 );
          if ( !nextFace1.IsNull() && !nextFace2.IsNull() ) {
            FE1.push_back( make_pair( nextFace1, *eIt1 ));
            FE2.push_back( make_pair( nextFace2, *eIt2 ));
          }
        }
      }
      InsertAssociation( theShape1, theShape2, theMap, bidirect );
      return true;
    }
      // ----------------------------------------------------------------------
    case TopAbs_COMPOUND: { // GROUP
      // ----------------------------------------------------------------------
      // Maybe groups contain only one member
      TopoDS_Iterator it1( theShape1 ), it2( theShape2 );
      TopAbs_ShapeEnum memberType = it1.Value().ShapeType();
      int nbMembers = Count( theShape1, memberType, true );
      if ( nbMembers == 0 ) return true;
      if ( nbMembers == 1 ) {
        return FindSubShapeAssociation( it1.Value(), theMesh1, it2.Value(), theMesh2, theMap );
      }
      // Try to make shells of faces
      //
      BRep_Builder builder;
      TopoDS_Shell shell1, shell2;
      builder.MakeShell(shell1); builder.MakeShell(shell2);
      if ( memberType == TopAbs_FACE ) {
        // just add faces of groups to shells
        for (; it1.More(); it1.Next(), it2.Next() )
          builder.Add( shell1, it1.Value() ), builder.Add( shell2, it2.Value() );
      }
      else if ( memberType == TopAbs_EDGE ) {
        // Try to add faces sharing more than one edge of a group or
        // sharing all its vertices with the group
        TopTools_IndexedMapOfShape groupVertices[2];
        TopExp::MapShapes( theShape1, TopAbs_VERTEX, groupVertices[0]);
        TopExp::MapShapes( theShape2, TopAbs_VERTEX, groupVertices[1]);
        //
        TopTools_MapOfShape groupEdges[2], addedFaces[2];
        bool hasInitAssoc = (!theMap.IsEmpty()), initAssocOK = !hasInitAssoc;
        for (; it1.More(); it1.Next(), it2.Next() ) {
          groupEdges[0].Add( it1.Value() );
          groupEdges[1].Add( it2.Value() );
          if ( !initAssocOK ) {
            // for shell association there must be an edge with both vertices bound
            TopoDS_Vertex v1, v2;
            TopExp::Vertices( TopoDS::Edge( it1.Value()), v1, v2 );
            initAssocOK = ( theMap.IsBound( v1 ) && theMap.IsBound( v2 ));
          }
        }
        for (int is2ndGroup = 0; initAssocOK && is2ndGroup < 2; ++is2ndGroup) {
          const TopoDS_Shape& group = is2ndGroup ? theShape2: theShape1;
          SMESH_Mesh*         mesh  = is2ndGroup ? theMesh2 : theMesh1;
          TopoDS_Shell&       shell = is2ndGroup ? shell2   : shell1;
          for ( TopoDS_Iterator it( group ); it.More(); it.Next() ) {
            const TopoDS_Edge& edge = TopoDS::Edge( it.Value() );
            TopoDS_Face face;
            for ( int iF = 0; iF < 2; ++iF ) { // loop on 2 faces sharing edge
              face = GetNextFace(mesh->GetAncestorMap(), edge, face);
              if ( !face.IsNull() ) {
                int nbGroupEdges = 0;
                for ( TopExp_Explorer f( face, TopAbs_EDGE ); f.More(); f.Next())
                  if ( groupEdges[ is2ndGroup ].Contains( f.Current() ))
                    if ( ++nbGroupEdges > 1 )
                      break;
                bool add = (nbGroupEdges > 1 || Count( face, TopAbs_EDGE, true ) == 1 );
                if ( !add ) {
                  add = true;
                  for ( TopExp_Explorer v( face, TopAbs_VERTEX ); add && v.More(); v.Next())
                    add = groupVertices[ is2ndGroup ].Contains( v.Current() );
                }
                if ( add && addedFaces[ is2ndGroup ].Add( face ))
                  builder.Add( shell, face );
              }
            }
          }
        }
      } else {
        RETURN_BAD_RESULT("Unexpected group type");
      }
      // Associate shells
      //
      int nbFaces1 = Count( shell1, TopAbs_FACE, 0 );
      int nbFaces2 = Count( shell2, TopAbs_FACE, 0 );
      if ( nbFaces1 != nbFaces2 )
        RETURN_BAD_RESULT("Different nb of faces found for shells");
      if ( nbFaces1 > 0 ) {
        bool ok = false;
        if ( nbFaces1 == 1 ) {
          TopoDS_Shape F1 = TopoDS_Iterator( shell1 ).Value();
          TopoDS_Shape F2 = TopoDS_Iterator( shell2 ).Value();
          ok = FindSubShapeAssociation( F1, theMesh1, F2, theMesh2, theMap );
        }
        else {
          ok = FindSubShapeAssociation(shell1, theMesh1, shell2, theMesh2, theMap );
        }
        // Check if all members are mapped 
        if ( ok ) {
          TopTools_MapOfShape boundMembers[2];
          TopoDS_Iterator mIt;
          for ( mIt.Initialize( theShape1 ); mIt.More(); mIt.Next())
            if ( theMap.IsBound( mIt.Value() )) {
              boundMembers[0].Add( mIt.Value() );
              boundMembers[1].Add( theMap( mIt.Value() ));
            }
          if ( boundMembers[0].Extent() != nbMembers ) {
            // make compounds of not bound members
            TopoDS_Compound comp[2];
            for ( int is2ndGroup = 0; is2ndGroup < 2; ++is2ndGroup ) {
              builder.MakeCompound( comp[is2ndGroup] );
              for ( mIt.Initialize( is2ndGroup ? theShape2:theShape1 ); mIt.More(); mIt.Next())
                if ( ! boundMembers[ is2ndGroup ].Contains( mIt.Value() ))
                  builder.Add( comp[ is2ndGroup ], mIt.Value() );
            }
            // check if theMap contains initial association for the comp's
            bool hasInitialAssoc = false;
            if ( memberType == TopAbs_EDGE ) {
              for ( TopExp_Explorer v( comp[0], TopAbs_VERTEX ); v.More(); v.Next())
                if ( theMap.IsBound( v.Current() )) {
                  hasInitialAssoc = true;
                  break;
                }
            }
            if ( hasInitialAssoc == bool( !theMap.IsEmpty() ))
              ok = FindSubShapeAssociation( comp[0], theMesh1, comp[1], theMesh2, theMap );
            else {
              TShapeShapeMap tmpMap;
              ok = FindSubShapeAssociation( comp[0], theMesh1, comp[1], theMesh2, tmpMap );
              if ( ok ) {
                TopTools_DataMapIteratorOfDataMapOfShapeShape mapIt( tmpMap );
                for ( ; mapIt.More(); mapIt.Next() )
                  theMap.Bind( mapIt.Key(), mapIt.Value());
              }
            }
          }
        }
        return ok;
      }
      // Each edge of an edge group is shared by own faces
      // ------------------------------------------------------------------
      //
      // map vertices to edges sharing them, avoid doubling edges in lists
      TopTools_DataMapOfShapeListOfShape v2e[2];
      for (int isFirst = 0; isFirst < 2; ++isFirst ) {
        const TopoDS_Shape& group = isFirst ? theShape1 : theShape2;
        TopTools_DataMapOfShapeListOfShape& veMap = v2e[ isFirst ? 0 : 1 ];
        TopTools_MapOfShape addedEdges;
        for ( TopExp_Explorer e( group, TopAbs_EDGE ); e.More(); e.Next() ) {
          const TopoDS_Shape& edge = e.Current();
          if ( addedEdges.Add( edge )) {
            for ( TopExp_Explorer v( edge, TopAbs_VERTEX ); v.More(); v.Next()) {
              const TopoDS_Shape& vertex = v.Current();
              if ( !veMap.IsBound( vertex )) {
                TopTools_ListOfShape l;
                veMap.Bind( vertex, l );
              }
              veMap( vertex ).Append( edge );
            }
          }
        }   
      }
      while ( !v2e[0].IsEmpty() )
      {
        // find a bound vertex
        TopoDS_Vertex V[2];
        TopTools_DataMapIteratorOfDataMapOfShapeListOfShape v2eIt( v2e[0] );
        for ( ; v2eIt.More(); v2eIt.Next())
          if ( theMap.IsBound( v2eIt.Key() )) {
            V[0] = TopoDS::Vertex( v2eIt.Key() );
            V[1] = TopoDS::Vertex( theMap( V[0] ));
            break;
          }
        if ( V[0].IsNull() )
          RETURN_BAD_RESULT("No more bound vertices");

        while ( !V[0].IsNull() && v2e[0].IsBound( V[0] )) {
          TopTools_ListOfShape& edges0 = v2e[0]( V[0] );
          TopTools_ListOfShape& edges1 = v2e[1]( V[1] );
          int nbE0 = edges0.Extent(), nbE1 = edges1.Extent();
          if ( nbE0 != nbE1 )
            RETURN_BAD_RESULT("Different nb of edges: "<< nbE0 << " != " << nbE1);

          if ( nbE0 == 1 )
          {
            TopoDS_Edge e0 = TopoDS::Edge( edges0.First() );
            TopoDS_Edge e1 = TopoDS::Edge( edges1.First() );
            v2e[0].UnBind( V[0] );
            v2e[1].UnBind( V[1] );
            InsertAssociation( e0, e1, theMap, bidirect );
            MESSAGE("Assoc edge " << theMesh1->GetMeshDS()->ShapeToIndex( e0 )<<
                    " to "        << theMesh2->GetMeshDS()->ShapeToIndex( e1 ));
            V[0] = GetNextVertex( e0, V[0] );
            V[1] = GetNextVertex( e1, V[1] );
            if ( !V[0].IsNull() ) {
              InsertAssociation( V[0], V[1], theMap, bidirect );
              MESSAGE("Assoc vertex " << theMesh1->GetMeshDS()->ShapeToIndex( V[0] )<<
                      " to "          << theMesh2->GetMeshDS()->ShapeToIndex( V[1] ));
            }
          }
          else if ( nbE0 == 2 )
          {
            // one of edges must have both ends bound
            TopoDS_Vertex v0e0 = GetNextVertex( TopoDS::Edge( edges0.First() ), V[0] );
            TopoDS_Vertex v1e0 = GetNextVertex( TopoDS::Edge( edges0.Last() ),  V[0] );
            TopoDS_Vertex v0e1 = GetNextVertex( TopoDS::Edge( edges1.First() ), V[1] );
            TopoDS_Vertex v1e1 = GetNextVertex( TopoDS::Edge( edges1.Last() ),  V[1] );
            TopoDS_Shape e0b, e1b, e0n, e1n, v1b; // bound and not-bound
            TopoDS_Vertex v0n, v1n;
            if ( theMap.IsBound( v0e0 )) {
              v0n = v1e0; e0b = edges0.First(); e0n = edges0.Last(); v1b = theMap( v0e0 );
            } else if ( theMap.IsBound( v1e0 )) {
              v0n = v0e0; e0n = edges0.First(); e0b = edges0.Last(); v1b = theMap( v1e0 );
            } else {
              RETURN_BAD_RESULT("None of vertices bound");
            }
            if ( v1b.IsSame( v1e1 )) {
              v1n = v0e1; e1n = edges1.First(); e1b = edges1.Last();
            } else {
              v1n = v1e1; e1b = edges1.First(); e1n = edges1.Last();
            }
            InsertAssociation( e0b, e1b, theMap, bidirect );
            InsertAssociation( e0n, e1n, theMap, bidirect );
            InsertAssociation( v0n, v1n, theMap, bidirect );
            MESSAGE("Assoc edge " << theMesh1->GetMeshDS()->ShapeToIndex( e0b )<<
                    " to "        << theMesh2->GetMeshDS()->ShapeToIndex( e1b ));
            MESSAGE("Assoc edge " << theMesh1->GetMeshDS()->ShapeToIndex( e0n )<<
                    " to "        << theMesh2->GetMeshDS()->ShapeToIndex( e1n ));
            MESSAGE("Assoc vertex " << theMesh1->GetMeshDS()->ShapeToIndex( v0n )<<
                    " to "          << theMesh2->GetMeshDS()->ShapeToIndex( v1n ));
            v2e[0].UnBind( V[0] );
            v2e[1].UnBind( V[1] );
            V[0] = v0n;
            V[1] = v1n;
          }
          else {
            RETURN_BAD_RESULT("Not implemented");
          }
        }
      } //while ( !v2e[0].IsEmpty() )
      return true;
    }

    default:
      RETURN_BAD_RESULT("Unexpected shape type");

    } // end switch by shape type
  } // end case of available initial vertex association

  //======================================================================
  // NO INITIAL VERTEX ASSOCIATION
  //======================================================================

  switch ( theShape1.ShapeType() ) {

  case TopAbs_EDGE: {
    // ----------------------------------------------------------------------
    TopoDS_Edge edge1 = TopoDS::Edge( theShape1 );
    TopoDS_Edge edge2 = TopoDS::Edge( theShape2 );
    if ( IsPropagationPossible( theMesh1, theMesh2 ))
    {
      TopoDS_Edge prpEdge = GetPropagationEdge( theMesh1, edge2, edge1 ).second;
      if ( !prpEdge.IsNull() )
      {
        TopoDS_Vertex VV1[2], VV2[2];
        TopExp::Vertices( edge1,   VV1[0], VV1[1], true );
        TopExp::Vertices( prpEdge, VV2[0], VV2[1], true );
        InsertAssociation( VV1[ 0 ], VV2[ 0 ], theMap, bidirect);
        InsertAssociation( VV1[ 1 ], VV2[ 1 ], theMap, bidirect);
        if ( VV1[0].IsSame( VV1[1] ) || // one of edges is closed
             VV2[0].IsSame( VV2[1] ) )
        {
          InsertAssociation( edge1, prpEdge, theMap, bidirect); // insert with a proper orientation
        }
        InsertAssociation( theShape1, theShape2, theMap, bidirect );
        return true; // done
      }
    }
    if ( IsClosedEdge( edge1 ) && IsClosedEdge( edge2 ))
    {
      // TODO: find out a proper orientation (is it possible?)
      InsertAssociation( edge1, edge2, theMap, bidirect); // insert with a proper orientation
      InsertAssociation( TopExp::FirstVertex(edge1), TopExp::FirstVertex(edge2),
                         theMap, bidirect);
      InsertAssociation( theShape1, theShape2, theMap, bidirect );
      return true; // done
    }
    break; // try by vertex closeness
  }

  case TopAbs_FACE: {
    // ----------------------------------------------------------------------
    if ( IsPropagationPossible( theMesh1, theMesh2 )) // try by propagation in one mesh
    {
      TopoDS_Face face1 = TopoDS::Face(theShape1);
      TopoDS_Face face2 = TopoDS::Face(theShape2);
      TopoDS_Edge edge1, edge2;
      // get outer edge of theShape1
      edge1 = TopoDS::Edge( OuterShape( face1, TopAbs_EDGE ));
      // find out if any edge of face2 is a propagation edge of outer edge1
      map<int,TopoDS_Edge> propag_edges; // use map to find the closest propagation edge
      for ( TopExp_Explorer exp( face2, TopAbs_EDGE ); exp.More(); exp.Next() ) {
        edge2 = TopoDS::Edge( exp.Current() );
        pair<int,TopoDS_Edge> step_edge = GetPropagationEdge( theMesh1, edge2, edge1 );
        if ( !step_edge.second.IsNull() ) { // propagation found
          propag_edges.insert( step_edge );
        }
      }
      if ( !propag_edges.empty() ) // propagation found
      {
        edge2 = propag_edges.begin()->second;
        TopoDS_Vertex VV1[2], VV2[2];
        TopExp::Vertices( edge1, VV1[0], VV1[1], true );
        TopExp::Vertices( edge2, VV2[0], VV2[1], true );
        list< TopoDS_Edge > edges1, edges2;
        int nbE = FindFaceAssociation( face1, VV1, face2, VV2, edges1, edges2 );
        if ( !nbE ) RETURN_BAD_RESULT("FindFaceAssociation() failed");
        if ( nbE == 2 ) // only 2 edges
        {
          // take care of proper association of propagated edges
          bool same1 = edge1.IsSame( edges1.front() );
          bool same2 = edge2.IsSame( edges2.front() );
          if ( same1 != same2 )
            Reverse(edges2, nbE);
        }
        // store association
        list< TopoDS_Edge >::iterator eIt1 = edges1.begin();
        list< TopoDS_Edge >::iterator eIt2 = edges2.begin();
        for ( ; eIt1 != edges1.end(); ++eIt1, ++eIt2 )
        {
          InsertAssociation( *eIt1, *eIt2, theMap, bidirect);
          VV1[0] = TopExp::FirstVertex( *eIt1, true );
          VV2[0] = TopExp::FirstVertex( *eIt2, true );
          InsertAssociation( VV1[0], VV2[0], theMap, bidirect);
        }
        InsertAssociation( theShape1, theShape2, theMap, bidirect );
        return true;
      }
    }
    break; // try by vertex closeness
  }
  case TopAbs_COMPOUND: {
    // ----------------------------------------------------------------------
    if ( IsPropagationPossible( theMesh1, theMesh2 )) {

      // try to accosiate all using propagation
      if ( AssocGroupsByPropagation( theShape1, theShape2, *theMesh1, theMap ))
        return true;

      // find a boundary edge for theShape1
      TopoDS_Edge E;
      for(TopExp_Explorer exp(theShape1, TopAbs_EDGE); exp.More(); exp.Next() ) {
        E = TopoDS::Edge( exp.Current() );
        if ( IsBoundaryEdge( E, theShape1, *theMesh1 ))
          break;
        else
          E.Nullify();
      }
      if ( E.IsNull() )
        break; // try by vertex closeness

      // find association for vertices of edge E
      TopoDS_Vertex VV1[2], VV2[2];
      for(TopExp_Explorer eexp(E, TopAbs_VERTEX); eexp.More(); eexp.Next()) {
        TopoDS_Vertex V1 = TopoDS::Vertex( eexp.Current() );
        // look for an edge ending in E whose one vertex is in theShape1
        // and the other, in theShape2
        const TopTools_ListOfShape& Ancestors = theMesh1->GetAncestors(V1);
        TopTools_ListIteratorOfListOfShape ita(Ancestors);
        for(; ita.More(); ita.Next()) {
          if( ita.Value().ShapeType() != TopAbs_EDGE ) continue;
          TopoDS_Edge edge = TopoDS::Edge(ita.Value());
          bool FromShape1 = false;
          for(TopExp_Explorer expe(theShape1, TopAbs_EDGE); expe.More(); expe.Next() ) {
            if(edge.IsSame(expe.Current())) {
              FromShape1 = true;
              break;
            }
          }
          if(!FromShape1) {
            // is it an edge between theShape1 and theShape2?
            TopExp_Explorer expv(edge, TopAbs_VERTEX);
            TopoDS_Vertex V2 = TopoDS::Vertex( expv.Current() );
            if(V2.IsSame(V1)) {
              expv.Next();
              V2 = TopoDS::Vertex( expv.Current() );
            }
            bool FromShape2 = false;
            for ( expv.Init( theShape2, TopAbs_VERTEX ); expv.More(); expv.Next()) {
              if ( V2.IsSame( expv.Current() )) {
                FromShape2 = true;
                break;
              }
            }
            if ( FromShape2 ) {
              if ( VV1[0].IsNull() )
                VV1[0] = V1, VV2[0] = V2;
              else
                VV1[1] = V1, VV2[1] = V2;
              break; // from loop on ancestors of V1
            }
          }
        }
      }
      if ( !VV1[1].IsNull() ) {
        InsertAssociation( VV1[0], VV2[0], theMap, bidirect);
        InsertAssociation( VV1[1], VV2[1], theMap, bidirect);
        return FindSubShapeAssociation( theShape1, theMesh1, theShape2, theMesh2, theMap);
      }
    }
    break; // try by vertex closeness
  }
  default:;
  }

  // Find association by closeness of vertices
  // ------------------------------------------

  TopTools_IndexedMapOfShape vMap1, vMap2;
  TopExp::MapShapes( theShape1, TopAbs_VERTEX, vMap1 );
  TopExp::MapShapes( theShape2, TopAbs_VERTEX, vMap2 );

  if ( vMap1.Extent() != vMap2.Extent() )
    RETURN_BAD_RESULT("Different nb of vertices");

  if ( vMap1.Extent() == 1 ) {
    InsertAssociation( vMap1(1), vMap2(1), theMap, bidirect);
    if ( theShape1.ShapeType() == TopAbs_EDGE ) {
      InsertAssociation( theShape1, theShape2, theMap, bidirect );
      return true;
    }
    return FindSubShapeAssociation( theShape1, theMesh1, theShape2, theMesh2, theMap);
  }

  // Find transformation to make the shapes be of similar size at same location

  Bnd_Box box[2];
  for ( int i = 1; i <= vMap1.Extent(); ++i ) {
    box[ 0 ].Add( BRep_Tool::Pnt ( TopoDS::Vertex( vMap1( i ))));
    box[ 1 ].Add( BRep_Tool::Pnt ( TopoDS::Vertex( vMap2( i ))));
  }

  gp_Pnt gc[2]; // box center
  double x0,y0,z0, x1,y1,z1;
  box[0].Get( x0,y0,z0, x1,y1,z1 );
  gc[0] = 0.5 * ( gp_XYZ( x0,y0,z0 ) + gp_XYZ( x1,y1,z1 ));
  box[1].Get( x0,y0,z0, x1,y1,z1 );
  gc[1] = 0.5 * ( gp_XYZ( x0,y0,z0 ) + gp_XYZ( x1,y1,z1 ));

  // 1 -> 2
  gp_Vec vec01( gc[0], gc[1] );
  double scale = sqrt( box[1].SquareExtent() / box[0].SquareExtent() );

  // Find 2 closest vertices

  TopoDS_Vertex VV1[2], VV2[2];
  // get 2 linked vertices of shape 1 not belonging to an inner wire of a face
  TopoDS_Shape edge = theShape1;
  TopExp_Explorer expF( theShape1, TopAbs_FACE ), expE;
  if ( expF.More() ) {
    for ( ; expF.More(); expF.Next() ) {
      edge.Nullify();
      TopoDS_Shape wire = OuterShape( TopoDS::Face( expF.Current() ), TopAbs_WIRE );
      for ( expE.Init( wire, TopAbs_EDGE ); edge.IsNull() && expE.More(); expE.Next() )
        if ( !IsClosedEdge( TopoDS::Edge( expE.Current() )))
          edge = expE.Current();
      if ( !edge.IsNull() )
        break;
    }
  } else if (edge.ShapeType() != TopAbs_EDGE) { // no faces
    edge.Nullify();
    for ( expE.Init( theShape1, TopAbs_EDGE ); edge.IsNull() && expE.More(); expE.Next() )
      if ( !IsClosedEdge( TopoDS::Edge( expE.Current() )))
        edge = expE.Current();
  }
  if ( edge.IsNull() || edge.ShapeType() != TopAbs_EDGE )
    RETURN_BAD_RESULT("Edge not found");

  TopExp::Vertices( TopoDS::Edge( edge ), VV1[0], VV1[1]);
  if ( VV1[0].IsSame( VV1[1] ))
    RETURN_BAD_RESULT("Only closed edges");

  // find vertices closest to 2 linked vertices of shape 1
  for ( int i1 = 0; i1 < 2; ++i1 )
  {
    double dist2 = DBL_MAX;
    gp_Pnt p1 = BRep_Tool::Pnt( VV1[ i1 ]);
    p1.Translate( vec01 );
    p1.Scale( gc[1], scale );
    for ( int i2 = 1; i2 <= vMap2.Extent(); ++i2 )
    {
      TopoDS_Vertex V2 = TopoDS::Vertex( vMap2( i2 ));
      gp_Pnt p2 = BRep_Tool::Pnt ( V2 );
      double d2 = p1.SquareDistance( p2 );
      if ( d2 < dist2 && !V2.IsSame( VV2[ 0 ])) {
        VV2[ i1 ] = V2; dist2 = d2;
      }
    }
  }

  InsertAssociation( VV1[ 0 ], VV2[ 0 ], theMap, bidirect);
  InsertAssociation( VV1[ 1 ], VV2[ 1 ], theMap, bidirect);
  MESSAGE("Initial assoc VERT " << theMesh1->GetMeshDS()->ShapeToIndex( VV1[ 0 ] )<<
          " to "                << theMesh2->GetMeshDS()->ShapeToIndex( VV2[ 0 ] )<<
          "\nand         VERT " << theMesh1->GetMeshDS()->ShapeToIndex( VV1[ 1 ] )<<
          " to "                << theMesh2->GetMeshDS()->ShapeToIndex( VV2[ 1 ] ));
  if ( theShape1.ShapeType() == TopAbs_EDGE ) {
    InsertAssociation( theShape1, theShape2, theMap, bidirect );
    return true;
  }

  return FindSubShapeAssociation( theShape1, theMesh1, theShape2, theMesh2, theMap );
}

//================================================================================
/*!
 * \brief Find association of edges of faces
 * \param face1 - face 1
 * \param VV1 - vertices of face 1
 * \param face2 - face 2
 * \param VV2 - vertices of face 2 associated with ones of face 1
 * \param edges1 - out list of edges of face 1
 * \param edges2 - out list of edges of face 2
 * \retval int - nb of edges in an outer wire in a success case, else zero
 */
//================================================================================

int StdMeshers_ProjectionUtils::FindFaceAssociation(const TopoDS_Face& face1,
                                                    TopoDS_Vertex      VV1[2],
                                                    const TopoDS_Face& face2,
                                                    TopoDS_Vertex      VV2[2],
                                                    list< TopoDS_Edge > & edges1,
                                                    list< TopoDS_Edge > & edges2)
{
  edges1.clear();
  edges2.clear();

  list< int > nbVInW1, nbVInW2;
  if ( SMESH_Block::GetOrderedEdges( face1, VV1[0], edges1, nbVInW1) !=
       SMESH_Block::GetOrderedEdges( face2, VV2[0], edges2, nbVInW2) )
    RETURN_BAD_RESULT("Different number of wires in faces ");

  if ( nbVInW1.front() != nbVInW2.front() )
    RETURN_BAD_RESULT("Different number of edges in faces: " <<
                      nbVInW1.front() << " != " << nbVInW2.front());

  // Define if we need to reverse one of wires to make edges in lists match each other

  bool reverse = false;

  list< TopoDS_Edge >::iterator eBackIt;
  if ( !VV1[1].IsSame( TopExp::LastVertex( edges1.front(), true ))) {
    reverse = true;
    eBackIt = --edges1.end();
    // check if the second vertex belongs to the first or last edge in the wire
    if ( !VV1[1].IsSame( TopExp::FirstVertex( *eBackIt, true ))) {
      bool KO = true; // belongs to none
      if ( nbVInW1.size() > 1 ) { // several wires
        eBackIt = edges1.begin();
        for ( int i = 1; i < nbVInW1.front(); ++i ) ++eBackIt;
        KO = !VV1[1].IsSame( TopExp::FirstVertex( *eBackIt, true ));
      }
      if ( KO )
        RETURN_BAD_RESULT("GetOrderedEdges() failed");
    }
  }
  eBackIt = --edges2.end();
  if ( !VV2[1].IsSame( TopExp::LastVertex( edges2.front(), true ))) {
    reverse = !reverse;
    // check if the second vertex belongs to the first or last edge in the wire
    if ( !VV2[1].IsSame( TopExp::FirstVertex( *eBackIt, true ))) {
      bool KO = true; // belongs to none
      if ( nbVInW2.size() > 1 ) { // several wires
        eBackIt = edges2.begin();
        for ( int i = 1; i < nbVInW2.front(); ++i ) ++eBackIt;
        KO = !VV2[1].IsSame( TopExp::FirstVertex( *eBackIt, true ));
      }
      if ( KO )
        RETURN_BAD_RESULT("GetOrderedEdges() failed");
    }
  }
  if ( reverse )
  {
    Reverse( edges2 , nbVInW2.front());
    if (( VV1[1].IsSame( TopExp::LastVertex( edges1.front(), true ))) !=
        ( VV2[1].IsSame( TopExp::LastVertex( edges2.front(), true ))))
      RETURN_BAD_RESULT("GetOrderedEdges() failed");
  }
  return nbVInW2.front();
}

//=======================================================================
//function : InitVertexAssociation
//purpose  : 
//=======================================================================

void StdMeshers_ProjectionUtils::InitVertexAssociation( const SMESH_Hypothesis* theHyp,
                                                        TShapeShapeMap &        theAssociationMap,
                                                        const TopoDS_Shape&     theTargetShape)
{
  string hypName = theHyp->GetName();
  if ( hypName == "ProjectionSource1D" ) {
    const StdMeshers_ProjectionSource1D * hyp =
      static_cast<const StdMeshers_ProjectionSource1D*>( theHyp );
    if ( hyp->HasVertexAssociation() )
      InsertAssociation( hyp->GetSourceVertex(),hyp->GetTargetVertex(),theAssociationMap);
  }
  else if ( hypName == "ProjectionSource2D" ) {
    const StdMeshers_ProjectionSource2D * hyp =
      static_cast<const StdMeshers_ProjectionSource2D*>( theHyp );
    if ( hyp->HasVertexAssociation() ) {
      InsertAssociation( hyp->GetSourceVertex(1),hyp->GetTargetVertex(1),theAssociationMap);
      InsertAssociation( hyp->GetSourceVertex(2),hyp->GetTargetVertex(2),theAssociationMap);
    }
  }
  else if ( hypName == "ProjectionSource3D" ) {
    const StdMeshers_ProjectionSource3D * hyp =
      static_cast<const StdMeshers_ProjectionSource3D*>( theHyp );
    if ( hyp->HasVertexAssociation() ) {
      InsertAssociation( hyp->GetSourceVertex(1),hyp->GetTargetVertex(1),theAssociationMap);
      InsertAssociation( hyp->GetSourceVertex(2),hyp->GetTargetVertex(2),theAssociationMap);
    }
  }
}

//=======================================================================
/*!
 * \brief Inserts association theShape1 <-> theShape2 to TShapeShapeMap
 * \param theShape1 - shape 1
 * \param theShape2 - shape 2
 * \param theAssociationMap - association map 
 * \retval bool - true if there was no association for these shapes before
 */
//=======================================================================

bool StdMeshers_ProjectionUtils::InsertAssociation( const TopoDS_Shape& theShape1,
                                                    const TopoDS_Shape& theShape2,
                                                    TShapeShapeMap &    theAssociationMap,
                                                    const bool          theBidirectional)
{
  if ( !theShape1.IsNull() && !theShape2.IsNull() ) {
    SHOW_VERTEX(theShape1,"Assoc ");
    SHOW_VERTEX(theShape2," to ");
    bool isNew = ( theAssociationMap.Bind( theShape1, theShape2 ));
    if ( theBidirectional )
      theAssociationMap.Bind( theShape2, theShape1 );
    return isNew;
  }
  else {
    throw SMESH_Exception("StdMeshers_ProjectionUtils: attempt to associate NULL shape");
  }
  return false;
}

//=======================================================================
//function : IsSubShape
//purpose  : 
//=======================================================================

bool StdMeshers_ProjectionUtils::IsSubShape( const TopoDS_Shape& shape,
                                             SMESH_Mesh*         aMesh )
{
  if ( shape.IsNull() || !aMesh )
    return false;
  return
    aMesh->GetMeshDS()->ShapeToIndex( shape ) ||
    // PAL16202
    shape.ShapeType() == TopAbs_COMPOUND && aMesh->GetMeshDS()->IsGroupOfSubShapes( shape );
}

//=======================================================================
//function : IsSubShape
//purpose  : 
//=======================================================================

bool StdMeshers_ProjectionUtils::IsSubShape( const TopoDS_Shape& shape,
                                             const TopoDS_Shape& mainShape )
{
  if ( !shape.IsNull() && !mainShape.IsNull() )
  {
    for ( TopExp_Explorer exp( mainShape, shape.ShapeType());
          exp.More();
          exp.Next() )
      if ( shape.IsSame( exp.Current() ))
        return true;
  }
  SCRUTE((shape.IsNull()));
  SCRUTE((mainShape.IsNull()));
  return false;
}


//=======================================================================
/*!
 * \brief Finds an edge by its vertices in a main shape of the mesh
 * \param aMesh - the mesh
 * \param V1 - vertex 1
 * \param V2 - vertex 2
 * \retval TopoDS_Edge - found edge
 */
//=======================================================================

TopoDS_Edge StdMeshers_ProjectionUtils::GetEdgeByVertices( SMESH_Mesh*          theMesh,
                                                           const TopoDS_Vertex& theV1,
                                                           const TopoDS_Vertex& theV2)
{
  if ( theMesh && !theV1.IsNull() && !theV2.IsNull() )
  {
    TopTools_ListIteratorOfListOfShape ancestorIt( theMesh->GetAncestors( theV1 ));
    for ( ; ancestorIt.More(); ancestorIt.Next() )
      if ( ancestorIt.Value().ShapeType() == TopAbs_EDGE )
        for ( TopExp_Explorer expV ( ancestorIt.Value(), TopAbs_VERTEX );
              expV.More();
              expV.Next() )
          if ( theV2.IsSame( expV.Current() ))
            return TopoDS::Edge( ancestorIt.Value() );
  }
  return TopoDS_Edge();
}

//================================================================================
/*!
 * \brief Return another face sharing an edge
 * \param edgeToFaces - data map of descendants to ancestors
 * \param edge - edge
 * \param face - face
 * \retval TopoDS_Face - found face
 */
//================================================================================

TopoDS_Face StdMeshers_ProjectionUtils::GetNextFace( const TAncestorMap& edgeToFaces,
                                                     const TopoDS_Edge&  edge,
                                                     const TopoDS_Face&  face)
{
//   if ( !edge.IsNull() && !face.IsNull() && edgeToFaces.Contains( edge ))
  if ( !edge.IsNull() && edgeToFaces.Contains( edge )) // PAL16202
  {
    TopTools_ListIteratorOfListOfShape ancestorIt( edgeToFaces.FindFromKey( edge ));
    for ( ; ancestorIt.More(); ancestorIt.Next() )
      if ( ancestorIt.Value().ShapeType() == TopAbs_FACE &&
           !face.IsSame( ancestorIt.Value() ))
        return TopoDS::Face( ancestorIt.Value() );
  }
  return TopoDS_Face();
}

//================================================================================
/*!
 * \brief Return other vertex of an edge
 */
//================================================================================

TopoDS_Vertex StdMeshers_ProjectionUtils::GetNextVertex(const TopoDS_Edge&   edge,
                                                        const TopoDS_Vertex& vertex)
{
  TopoDS_Vertex vF,vL;
  TopExp::Vertices(edge,vF,vL);
  if ( vF.IsSame( vL ))
    return TopoDS_Vertex();
  return vertex.IsSame( vF ) ? vL : vF; 
}

//================================================================================
/*!
 * \brief Return a propagation edge
 * \param aMesh - mesh
 * \param theEdge - edge to find by propagation
 * \param fromEdge - start edge for propagation
 * \retval pair<int,TopoDS_Edge> - propagation step and found edge
 */
//================================================================================

pair<int,TopoDS_Edge>
StdMeshers_ProjectionUtils::GetPropagationEdge( SMESH_Mesh*        aMesh,
                                                const TopoDS_Edge& theEdge,
                                                const TopoDS_Edge& fromEdge)
{
  SMESH_IndexedMapOfShape aChain;
  int step = 0;

  // List of edges, added to chain on the previous cycle pass
  TopTools_ListOfShape listPrevEdges;
  listPrevEdges.Append(fromEdge);

  // Collect all edges pass by pass
  while (listPrevEdges.Extent() > 0) {
    step++;
    // List of edges, added to chain on this cycle pass
    TopTools_ListOfShape listCurEdges;

    // Find the next portion of edges
    TopTools_ListIteratorOfListOfShape itE (listPrevEdges);
    for (; itE.More(); itE.Next()) {
      TopoDS_Shape anE = itE.Value();

      // Iterate on faces, having edge <anE>
      TopTools_ListIteratorOfListOfShape itA (aMesh->GetAncestors(anE));
      for (; itA.More(); itA.Next()) {
        TopoDS_Shape aW = itA.Value();

        // There are objects of different type among the ancestors of edge
        if (aW.ShapeType() == TopAbs_WIRE) {
          TopoDS_Shape anOppE;

          BRepTools_WireExplorer aWE (TopoDS::Wire(aW));
          Standard_Integer nb = 1, found = 0;
          TopTools_Array1OfShape anEdges (1,4);
          for (; aWE.More(); aWE.Next(), nb++) {
            if (nb > 4) {
              found = 0;
              break;
            }
            anEdges(nb) = aWE.Current();
            if (anEdges(nb).IsSame(anE)) found = nb;
          }

          if (nb == 5 && found > 0) {
            // Quadrangle face found, get an opposite edge
            Standard_Integer opp = found + 2;
            if (opp > 4) opp -= 4;
            anOppE = anEdges(opp);

            // add anOppE to aChain if ...
            if (!aChain.Contains(anOppE)) { // ... anOppE is not in aChain
              // Add found edge to the chain oriented so that to
              // have it co-directed with a forward MainEdge
              TopAbs_Orientation ori = anE.Orientation();
              if ( anEdges(opp).Orientation() == anEdges(found).Orientation() )
                ori = TopAbs::Reverse( ori );
              anOppE.Orientation( ori );
              if ( anOppE.IsSame( theEdge ))
                return make_pair( step, TopoDS::Edge( anOppE ));
              aChain.Add(anOppE);
              listCurEdges.Append(anOppE);
            }
          } // if (nb == 5 && found > 0)
        } // if (aF.ShapeType() == TopAbs_WIRE)
      } // for (; itF.More(); itF.Next())
    } // for (; itE.More(); itE.Next())

    listPrevEdges = listCurEdges;
  } // while (listPrevEdges.Extent() > 0)

  return make_pair( INT_MAX, TopoDS_Edge());
}

//================================================================================
  /*!
   * \brief Find corresponding nodes on two faces
    * \param face1 - the first face
    * \param mesh1 - mesh containing elements on the first face
    * \param face2 - the second face
    * \param mesh2 - mesh containing elements on the second face
    * \param assocMap - map associating subshapes of the faces
    * \param node1To2Map - map containing found matching nodes
    * \retval bool - is a success
   */
//================================================================================

bool StdMeshers_ProjectionUtils::
FindMatchingNodesOnFaces( const TopoDS_Face&     face1,
                          SMESH_Mesh*            mesh1,
                          const TopoDS_Face&     face2,
                          SMESH_Mesh*            mesh2,
                          const TShapeShapeMap & assocMap,
                          TNodeNodeMap &         node1To2Map)
{
  SMESHDS_Mesh* meshDS1 = mesh1->GetMeshDS();
  SMESHDS_Mesh* meshDS2 = mesh2->GetMeshDS();
  
  SMESH_MesherHelper helper1( *mesh1 );
  SMESH_MesherHelper helper2( *mesh2 );

  // Get corresponding submeshes and roughly check match of meshes

  SMESHDS_SubMesh * SM2 = meshDS2->MeshElements( face2 );
  SMESHDS_SubMesh * SM1 = meshDS1->MeshElements( face1 );
  if ( !SM2 || !SM1 )
    RETURN_BAD_RESULT("Empty submeshes");
  if ( SM2->NbNodes()    != SM1->NbNodes() ||
       SM2->NbElements() != SM1->NbElements() )
    RETURN_BAD_RESULT("Different meshes on corresponding faces "
                      << meshDS1->ShapeToIndex( face1 ) << " and "
                      << meshDS2->ShapeToIndex( face2 ));
  if ( SM2->NbElements() == 0 )
    RETURN_BAD_RESULT("Empty submeshes");

  helper1.SetSubShape( face1 );
  helper2.SetSubShape( face2 );
  if ( helper1.HasSeam() != helper2.HasSeam() )
    RETURN_BAD_RESULT("Different faces' geometry");

  // Data to call SMESH_MeshEditor::FindMatchingNodes():

  // 1. Nodes of corresponding links:

  // get 2 matching edges, try to find not seam ones
  TopoDS_Edge edge1, edge2, seam1, seam2, anyEdge1, anyEdge2;
  TopExp_Explorer eE( OuterShape( face2, TopAbs_WIRE ), TopAbs_EDGE );
  do {
    // edge 2
    TopoDS_Edge e2 = TopoDS::Edge( eE.Current() );
    eE.Next();
    // edge 1
    if ( !assocMap.IsBound( e2 ))
      RETURN_BAD_RESULT("Association not found for edge " << meshDS2->ShapeToIndex( e2 ));
    TopoDS_Edge e1 = TopoDS::Edge( assocMap( e2 ));
    if ( !IsSubShape( e1, face1 ))
      RETURN_BAD_RESULT("Wrong association, edge " << meshDS1->ShapeToIndex( e1 ) <<
                        " isn't a subshape of face " << meshDS1->ShapeToIndex( face1 ));
    // check that there are nodes on edges
    SMESHDS_SubMesh * eSM1 = meshDS1->MeshElements( e1 );
    SMESHDS_SubMesh * eSM2 = meshDS2->MeshElements( e2 );
    bool nodesOnEdges = ( eSM1 && eSM2 && eSM1->NbNodes() && eSM2->NbNodes() );
    // check that the nodes on edges belong to faces
    // (as NETGEN ignores nodes on the degenerated geom edge)
    bool nodesOfFaces = false;
    if ( nodesOnEdges ) {
      const SMDS_MeshNode* n1 = eSM1->GetNodes()->next();
      const SMDS_MeshNode* n2 = eSM2->GetNodes()->next();
      nodesOfFaces = ( n1->GetInverseElementIterator(SMDSAbs_Face)->more() &&
                       n2->GetInverseElementIterator(SMDSAbs_Face)->more() );
    }
    if ( nodesOfFaces )
    {
      if ( helper2.IsRealSeam( e2 )) {
        seam1 = e1; seam2 = e2;
      }
      else {
        edge1 = e1; edge2 = e2;
      }
    }
    else {
      anyEdge1 = e1; anyEdge2 = e2;
    }
  } while ( edge2.IsNull() && eE.More() );
  //
  if ( edge2.IsNull() ) {
    edge1 = seam1; edge2 = seam2;
  }
  bool hasNodesOnEdge = (! edge2.IsNull() );
  if ( !hasNodesOnEdge ) {
    // 0020338 - nb segments == 1
    edge1 = anyEdge1; edge2 = anyEdge2;
  }

  // get 2 matching vertices
  TopoDS_Vertex V2 = TopExp::FirstVertex( TopoDS::Edge( edge2 ));
  if ( !assocMap.IsBound( V2 ))
    RETURN_BAD_RESULT("Association not found for vertex " << meshDS2->ShapeToIndex( V2 ));
  TopoDS_Vertex V1 = TopoDS::Vertex( assocMap( V2 ));

  // nodes on vertices
  const SMDS_MeshNode* vNode1 = SMESH_Algo::VertexNode( V1, meshDS1 );
  const SMDS_MeshNode* vNode2 = SMESH_Algo::VertexNode( V2, meshDS2 );
  if ( !vNode1 ) RETURN_BAD_RESULT("No node on vertex #" << meshDS1->ShapeToIndex( V1 ));
  if ( !vNode2 ) RETURN_BAD_RESULT("No node on vertex #" << meshDS2->ShapeToIndex( V2 ));

  // nodes on edges linked with nodes on vertices
  const SMDS_MeshNode* nullNode = 0;
  vector< const SMDS_MeshNode*> eNode1( 2, nullNode );
  vector< const SMDS_MeshNode*> eNode2( 2, nullNode );
  if ( hasNodesOnEdge )
  {
    int nbNodeToGet = 1;
    if ( IsClosedEdge( edge1 ) || IsClosedEdge( edge2 ) )
      nbNodeToGet = 2;
    for ( int is2 = 0; is2 < 2; ++is2 )
    {
      TopoDS_Edge &     edge  = is2 ? edge2 : edge1;
      SMESHDS_Mesh *    smDS  = is2 ? meshDS2 : meshDS1;
      SMESHDS_SubMesh* edgeSM = smDS->MeshElements( edge );
      // nodes linked with ones on vertices
      const SMDS_MeshNode*           vNode = is2 ? vNode2 : vNode1;
      vector< const SMDS_MeshNode*>& eNode = is2 ? eNode2 : eNode1;
      int nbGotNode = 0;
      SMDS_ElemIteratorPtr vElem = vNode->GetInverseElementIterator(SMDSAbs_Edge);
      while ( vElem->more() && nbGotNode != nbNodeToGet ) {
        const SMDS_MeshElement* elem = vElem->next();
        if ( edgeSM->Contains( elem ))
          eNode[ nbGotNode++ ] = 
            ( elem->GetNode(0) == vNode ) ? elem->GetNode(1) : elem->GetNode(0);
      }
      if ( nbGotNode > 1 ) // sort found nodes by param on edge
      {
        SMESH_MesherHelper* helper = is2 ? &helper2 : &helper1;
        double u0 = helper->GetNodeU( edge, eNode[ 0 ]);
        double u1 = helper->GetNodeU( edge, eNode[ 1 ]);
        if ( u0 > u1 ) std::swap( eNode[ 0 ], eNode[ 1 ]);
      }
      if ( nbGotNode == 0 )
        RETURN_BAD_RESULT("Found no nodes on edge " << smDS->ShapeToIndex( edge ) <<
                          " linked to " << vNode );
    }
  }
  else // 0020338 - nb segments == 1
  {
    // get 2 other matching vertices
    V2 = TopExp::LastVertex( TopoDS::Edge( edge2 ));
    if ( !assocMap.IsBound( V2 ))
      RETURN_BAD_RESULT("Association not found for vertex " << meshDS2->ShapeToIndex( V2 ));
    V1 = TopoDS::Vertex( assocMap( V2 ));

    // nodes on vertices
    eNode1[0] = SMESH_Algo::VertexNode( V1, meshDS1 );
    eNode2[0] = SMESH_Algo::VertexNode( V2, meshDS2 );
    if ( !eNode1[0] ) RETURN_BAD_RESULT("No node on vertex #" << meshDS1->ShapeToIndex( V1 ));
    if ( !eNode2[0] ) RETURN_BAD_RESULT("No node on vertex #" << meshDS2->ShapeToIndex( V2 ));
  }

  // 2. face sets

  set<const SMDS_MeshElement*> Elems1, Elems2;
  for ( int is2 = 0; is2 < 2; ++is2 )
  {
    set<const SMDS_MeshElement*> & elems = is2 ? Elems2 : Elems1;
    SMESHDS_SubMesh*                  sm = is2 ? SM2 : SM1;
    SMESH_MesherHelper*           helper = is2 ? &helper2 : &helper1;
    const TopoDS_Face &             face = is2 ? face2 : face1;
    SMDS_ElemIteratorPtr eIt = sm->GetElements();

    if ( !helper->IsRealSeam( is2 ? edge2 : edge1 ))
    {
      while ( eIt->more() ) elems.insert( eIt->next() );
    }
    else
    {
      // the only suitable edge is seam, i.e. it is a sphere.
      // FindMatchingNodes() will not know which way to go from any edge.
      // So we ignore all faces having nodes on edges or vertices except
      // one of faces sharing current start nodes

      // find a face to keep
      const SMDS_MeshElement* faceToKeep = 0;
      const SMDS_MeshNode* vNode = is2 ? vNode2 : vNode1;
      const SMDS_MeshNode* eNode = is2 ? eNode2[0] : eNode1[0];
      TIDSortedElemSet inSet, notInSet;

      const SMDS_MeshElement* f1 =
        SMESH_MeshEditor::FindFaceInSet( vNode, eNode, inSet, notInSet );
      if ( !f1 ) RETURN_BAD_RESULT("The first face on seam not found");
      notInSet.insert( f1 );

      const SMDS_MeshElement* f2 =
        SMESH_MeshEditor::FindFaceInSet( vNode, eNode, inSet, notInSet );
      if ( !f2 ) RETURN_BAD_RESULT("The second face on seam not found");

      // select a face with less UV of vNode
      const SMDS_MeshNode* notSeamNode[2] = {0, 0};
      for ( int iF = 0; iF < 2; ++iF ) {
        const SMDS_MeshElement* f = ( iF ? f2 : f1 );
        for ( int i = 0; !notSeamNode[ iF ] && i < f->NbNodes(); ++i ) {
          const SMDS_MeshNode* node = f->GetNode( i );
          if ( !helper->IsSeamShape( node->GetPosition()->GetShapeId() ))
            notSeamNode[ iF ] = node;
        }
      }
      gp_Pnt2d uv1 = helper->GetNodeUV( face, vNode, notSeamNode[0] );
      gp_Pnt2d uv2 = helper->GetNodeUV( face, vNode, notSeamNode[1] );
      if ( uv1.X() + uv1.Y() > uv2.X() + uv2.Y() )
        faceToKeep = f2;
      else
        faceToKeep = f1;

      // fill elem set
      elems.insert( faceToKeep );
      while ( eIt->more() ) {
        const SMDS_MeshElement* f = eIt->next();
        int nbNodes = f->NbNodes();
        if ( f->IsQuadratic() )
          nbNodes /= 2;
        bool onBnd = false;
        for ( int i = 0; !onBnd && i < nbNodes; ++i ) {
          const SMDS_MeshNode* node = f->GetNode( i );
          onBnd = ( node->GetPosition()->GetTypeOfPosition() != SMDS_TOP_FACE);
        }
        if ( !onBnd )
          elems.insert( f );
      }
      // add also faces adjacent to faceToKeep
      int nbNodes = faceToKeep->NbNodes();
      if ( faceToKeep->IsQuadratic() ) nbNodes /= 2;
      notInSet.insert( f1 );
      notInSet.insert( f2 );
      for ( int i = 0; i < nbNodes; ++i ) {
        const SMDS_MeshNode* n1 = faceToKeep->GetNode( i );
        const SMDS_MeshNode* n2 = faceToKeep->GetNode( i+1 % nbNodes );
        f1 = SMESH_MeshEditor::FindFaceInSet( n1, n2, inSet, notInSet );
        if ( f1 )
          elems.insert( f1 );
      }
    } // case on a sphere
  } // loop on 2 faces

  //  int quadFactor = (*Elems1.begin())->IsQuadratic() ? 2 : 1;

  node1To2Map.clear();
  int res = SMESH_MeshEditor::FindMatchingNodes( Elems1, Elems2,
                                                 vNode1, vNode2,
                                                 eNode1[0], eNode2[0],
                                                 node1To2Map);
  if ( res != SMESH_MeshEditor::SEW_OK )
    RETURN_BAD_RESULT("FindMatchingNodes() result " << res );

  // On a sphere, add matching nodes on the edge

  if ( helper1.IsRealSeam( edge1 ))
  {
    // sort nodes on edges by param on edge
    map< double, const SMDS_MeshNode* > u2nodesMaps[2];
    for ( int is2 = 0; is2 < 2; ++is2 )
    {
      TopoDS_Edge &     edge  = is2 ? edge2 : edge1;
      SMESHDS_Mesh *    smDS  = is2 ? meshDS2 : meshDS1;
      SMESHDS_SubMesh* edgeSM = smDS->MeshElements( edge );
      map< double, const SMDS_MeshNode* > & pos2nodes = u2nodesMaps[ is2 ];

      SMDS_NodeIteratorPtr nIt = edgeSM->GetNodes();
      while ( nIt->more() ) {
        const SMDS_MeshNode* node = nIt->next();
        const SMDS_EdgePosition* pos =
          static_cast<const SMDS_EdgePosition*>(node->GetPosition().get());
        pos2nodes.insert( make_pair( pos->GetUParameter(), node ));
      }
      if ( pos2nodes.size() != edgeSM->NbNodes() )
        RETURN_BAD_RESULT("Equal params of nodes on edge "
                          << smDS->ShapeToIndex( edge ) << " of face " << is2 );
    }
    if ( u2nodesMaps[0].size() != u2nodesMaps[1].size() )
      RETURN_BAD_RESULT("Different nb of new nodes on edges or wrong params");

    // compare edge orientation
    double u1 = helper1.GetNodeU( edge1, vNode1 );
    double u2 = helper2.GetNodeU( edge2, vNode2 );
    bool isFirst1 = ( u1 < u2nodesMaps[0].begin()->first );
    bool isFirst2 = ( u2 < u2nodesMaps[1].begin()->first );
    bool reverse ( isFirst1 != isFirst2 );

    // associate matching nodes
    map< double, const SMDS_MeshNode* >::iterator u_Node1, u_Node2, end1;
    map< double, const SMDS_MeshNode* >::reverse_iterator uR_Node2;
    u_Node1 = u2nodesMaps[0].begin();
    u_Node2 = u2nodesMaps[1].begin();
    uR_Node2 = u2nodesMaps[1].rbegin();
    end1 = u2nodesMaps[0].end();
    for ( ; u_Node1 != end1; ++u_Node1 ) {
      const SMDS_MeshNode* n1 = u_Node1->second;
      const SMDS_MeshNode* n2 = ( reverse ? (uR_Node2++)->second : (u_Node2++)->second );
      node1To2Map.insert( make_pair( n1, n2 ));
    }

    // associate matching nodes on the last vertices
    V2 = TopExp::LastVertex( TopoDS::Edge( edge2 ));
    if ( !assocMap.IsBound( V2 ))
      RETURN_BAD_RESULT("Association not found for vertex " << meshDS2->ShapeToIndex( V2 ));
    V1 = TopoDS::Vertex( assocMap( V2 ));
    vNode1 = SMESH_Algo::VertexNode( V1, meshDS1 );
    vNode2 = SMESH_Algo::VertexNode( V2, meshDS2 );
    if ( !vNode1 ) RETURN_BAD_RESULT("No node on vertex #" << meshDS1->ShapeToIndex( V1 ));
    if ( !vNode2 ) RETURN_BAD_RESULT("No node on vertex #" << meshDS2->ShapeToIndex( V2 ));
    node1To2Map.insert( make_pair( vNode1, vNode2 ));
  }

// don't know why this condition is usually true :(
//   if ( node1To2Map.size() * quadFactor < SM1->NbNodes() )
//     MESSAGE("FindMatchingNodes() found too few node pairs starting from nodes ("
//             << vNode1->GetID() << " - " << eNode1[0]->GetID() << ") ("
//             << vNode2->GetID() << " - " << eNode2[0]->GetID() << "):"
//             << node1To2Map.size() * quadFactor << " < " << SM1->NbNodes());
  
  return true;
}

//================================================================================
/*!
 * \brief Check if the first and last vertices of an edge are the same
 * \param anEdge - the edge to check
 * \retval bool - true if same
 */
//================================================================================

bool StdMeshers_ProjectionUtils::IsClosedEdge( const TopoDS_Edge& anEdge )
{
  return TopExp::FirstVertex( anEdge ).IsSame( TopExp::LastVertex( anEdge ));
}

//================================================================================
  /*!
   * \brief Return any subshape of a face belonging to the outer wire
    * \param face - the face
    * \param type - type of subshape to return
    * \retval TopoDS_Shape - the found subshape
   */
//================================================================================

TopoDS_Shape StdMeshers_ProjectionUtils::OuterShape( const TopoDS_Face& face,
                                                     TopAbs_ShapeEnum   type)
{
  TopExp_Explorer exp( BRepTools::OuterWire( face ), type );
  if ( exp.More() )
    return exp.Current();
  return TopoDS_Shape();
}

//================================================================================
  /*!
   * \brief Check that submesh is computed and try to compute it if is not
    * \param sm - submesh to compute
    * \param iterationNb - int used to stop infinite recursive call
    * \retval bool - true if computed
   */
//================================================================================

bool StdMeshers_ProjectionUtils::MakeComputed(SMESH_subMesh * sm, const int iterationNb)
{
  if ( iterationNb > 10 )
    RETURN_BAD_RESULT("Infinite recursive projection");
  if ( !sm )
    RETURN_BAD_RESULT("NULL submesh");
  if ( sm->IsMeshComputed() )
    return true;

  SMESH_Mesh* mesh = sm->GetFather();
  SMESH_Gen* gen   = mesh->GetGen();
  SMESH_Algo* algo = gen->GetAlgo( *mesh, sm->GetSubShape() );
  if ( !algo )
  {
    if ( sm->GetSubShape().ShapeType() != TopAbs_COMPOUND )
      RETURN_BAD_RESULT("No algo assigned to submesh " << sm->GetId());
    // group
    bool computed = true;
    for ( TopoDS_Iterator grMember( sm->GetSubShape() ); grMember.More(); grMember.Next())
      if ( SMESH_subMesh* grSub = mesh->GetSubMesh( grMember.Value() ))
        if ( !MakeComputed( grSub, iterationNb + 1 ))
          computed = false;
    return computed;
  }

  string algoType = algo->GetName();
  if ( algoType.substr(0, 11) != "Projection_")
    return gen->Compute( *mesh, sm->GetSubShape() );

  // try to compute source mesh

  const list <const SMESHDS_Hypothesis *> & hyps =
    algo->GetUsedHypothesis( *mesh, sm->GetSubShape() );

  TopoDS_Shape srcShape;
  SMESH_Mesh* srcMesh = 0;
  list <const SMESHDS_Hypothesis*>::const_iterator hIt = hyps.begin();
  for ( ; srcShape.IsNull() && hIt != hyps.end(); ++hIt ) {
    string hypName = (*hIt)->GetName();
    if ( hypName == "ProjectionSource1D" ) {
      const StdMeshers_ProjectionSource1D * hyp =
        static_cast<const StdMeshers_ProjectionSource1D*>( *hIt );
      srcShape = hyp->GetSourceEdge();
      srcMesh = hyp->GetSourceMesh();
    }
    else if ( hypName == "ProjectionSource2D" ) {
      const StdMeshers_ProjectionSource2D * hyp =
        static_cast<const StdMeshers_ProjectionSource2D*>( *hIt );
      srcShape = hyp->GetSourceFace();
      srcMesh = hyp->GetSourceMesh();
    }
    else if ( hypName == "ProjectionSource3D" ) {
      const StdMeshers_ProjectionSource3D * hyp =
        static_cast<const StdMeshers_ProjectionSource3D*>( *hIt );
      srcShape = hyp->GetSource3DShape();
      srcMesh = hyp->GetSourceMesh();
    }
  }
  if ( srcShape.IsNull() ) // no projection source defined
    return gen->Compute( *mesh, sm->GetSubShape() );

  if ( srcShape.IsSame( sm->GetSubShape() ))
    RETURN_BAD_RESULT("Projection from self");
    
  if ( !srcMesh )
    srcMesh = mesh;

  if ( MakeComputed( srcMesh->GetSubMesh( srcShape ), iterationNb + 1 ))
    return gen->Compute( *mesh, sm->GetSubShape() );

  return false;
}

//================================================================================
/*!
 * \brief Count nb of subshapes
 * \param shape - the shape
 * \param type - the type of subshapes to count
 * \retval int - the calculated number
 */
//================================================================================

int StdMeshers_ProjectionUtils::Count(const TopoDS_Shape&    shape,
                                      const TopAbs_ShapeEnum type,
                                      const bool             ignoreSame)
{
  if ( ignoreSame ) {
    TopTools_IndexedMapOfShape map;
    TopExp::MapShapes( shape, type, map );
    return map.Extent();
  }
  else {
    int nb = 0;
    for ( TopExp_Explorer exp( shape, type ); exp.More(); exp.Next() )
      ++nb;
    return nb;
  }
}

//================================================================================
/*!
 * \brief Return true if edge is a boundary of edgeContainer
 */
//================================================================================

bool StdMeshers_ProjectionUtils::IsBoundaryEdge(const TopoDS_Edge&  edge,
                                                const TopoDS_Shape& edgeContainer,
                                                SMESH_Mesh&         mesh)
{
  TopTools_IndexedMapOfShape facesOfEdgeContainer, facesNearEdge;
  TopExp::MapShapes( edgeContainer, TopAbs_FACE, facesOfEdgeContainer );

  const TopTools_ListOfShape& EAncestors = mesh.GetAncestors(edge);
  TopTools_ListIteratorOfListOfShape itea(EAncestors);
  for(; itea.More(); itea.Next()) {
    if( itea.Value().ShapeType() == TopAbs_FACE &&
        facesOfEdgeContainer.Contains( itea.Value() ))
    {
      facesNearEdge.Add( itea.Value() );
      if ( facesNearEdge.Extent() > 1 )
        return false;
    }
  }
  return ( facesNearEdge.Extent() == 1 );
}


namespace {

  SMESH_subMeshEventListener* GetSrcSubMeshListener();

  //================================================================================
  /*!
   * \brief Listener that resets an event listener on source submesh when 
   * "ProjectionSource*D" hypothesis is modified
   */
  //================================================================================

  struct HypModifWaiter: SMESH_subMeshEventListener
  {
    HypModifWaiter():SMESH_subMeshEventListener(0){} // won't be deleted by submesh

    void ProcessEvent(const int event, const int eventType, SMESH_subMesh* subMesh,
                      EventListenerData*, const SMESH_Hypothesis*)
    {
      if ( event     == SMESH_subMesh::MODIF_HYP &&
           eventType == SMESH_subMesh::ALGO_EVENT)
      {
        // delete current source listener
        subMesh->DeleteEventListener( GetSrcSubMeshListener() );
        // let algo set a new one
        SMESH_Gen* gen = subMesh->GetFather()->GetGen();
        if ( SMESH_Algo* algo = gen->GetAlgo( *subMesh->GetFather(),
                                              subMesh->GetSubShape() ))
          algo->SetEventListener( subMesh );
      }
    }
  };
  //================================================================================
  /*!
   * \brief return static HypModifWaiter
   */
  //================================================================================

  SMESH_subMeshEventListener* GetHypModifWaiter() {
    static HypModifWaiter aHypModifWaiter;
    return &aHypModifWaiter;
  }
  //================================================================================
  /*!
   * \brief return static listener for source shape submeshes
   */
  //================================================================================

  SMESH_subMeshEventListener* GetSrcSubMeshListener() {
    static SMESH_subMeshEventListener srcListener(0); // won't be deleted by submesh
    return &srcListener;
  }
}

//================================================================================
/*!
 * \brief Set event listeners to submesh with projection algo
 * \param subMesh - submesh with projection algo
 * \param srcShape - source shape
 * \param srcMesh - source mesh
 */
//================================================================================

void StdMeshers_ProjectionUtils::SetEventListener(SMESH_subMesh* subMesh,
                                                  TopoDS_Shape   srcShape,
                                                  SMESH_Mesh*    srcMesh)
{
  // Set listener that resets an event listener on source submesh when
  // "ProjectionSource*D" hypothesis is modified
  subMesh->SetEventListener( GetHypModifWaiter(),0,subMesh);

  // Set an event listener to submesh of the source shape
  if ( !srcShape.IsNull() )
  {
    if ( !srcMesh )
      srcMesh = subMesh->GetFather();

    SMESH_subMesh* srcShapeSM = srcMesh->GetSubMesh( srcShape );

    if ( srcShapeSM != subMesh ) {
      if ( srcShapeSM->GetSubMeshDS() &&
           srcShapeSM->GetSubMeshDS()->IsComplexSubmesh() )
      {  // source shape is a group
        TopExp_Explorer it(srcShapeSM->GetSubShape(), // explore the group into subshapes...
                           subMesh->GetSubShape().ShapeType()); // ...of target shape type
        for (; it.More(); it.Next())
        {
          SMESH_subMesh* srcSM = srcMesh->GetSubMesh( it.Current() );
          SMESH_subMeshEventListenerData* data =
            srcSM->GetEventListenerData(GetSrcSubMeshListener());
          if ( data )
            data->mySubMeshes.push_back( subMesh );
          else
            data = SMESH_subMeshEventListenerData::MakeData( subMesh );
          subMesh->SetEventListener ( GetSrcSubMeshListener(), data, srcSM );
        }
      }
      else
      {
        subMesh->SetEventListener( GetSrcSubMeshListener(),
                                   SMESH_subMeshEventListenerData::MakeData( subMesh ),
                                   srcShapeSM );
      }
    }
  }
}
