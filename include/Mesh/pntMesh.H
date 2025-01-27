/*******************************************************************************
* Promesh                                                                      *
* Copyright (C) 2022, IllinoisRocstar LLC. All rights reserved.                *
*                                                                              *
* Promesh is the property of IllinoisRocstar LLC.                              *
*                                                                              *
* IllinoisRocstar LLC                                                          *
* Champaign, IL                                                                *
* www.illinoisrocstar.com                                                      *
* promesh@illinoisrocstar.com                                                  *
*******************************************************************************/
/*******************************************************************************
* This file is part of Promesh                                                 *
*                                                                              *
* This version of Promesh is free software: you can redistribute it and/or     *
* modify it under the terms of the GNU Lesser General Public License as        *
* published by the Free Software Foundation, either version 3 of the License,  *
* or (at your option) any later version.                                       *
*                                                                              *
* Promesh is distributed in the hope that it will be useful, but WITHOUT ANY   *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS    *
* FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more *
* details.                                                                     *
*                                                                              *
* You should have received a copy of the GNU Lesser General Public License     *
* along with this program. If not, see <https://www.gnu.org/licenses/>.        *
*                                                                              *
*******************************************************************************/
#ifndef NEMOSYS_PNTMESH_H_
#define NEMOSYS_PNTMESH_H_

// Nemosys headers
#include "nemosys_export.h"
#include "Mesh/meshBase.H"

// VTK
#include <vtkCellTypes.h>

// others
#include <set>
#include <map>

namespace PNTMesh {

using idTyp = int;

enum class elementType {
  BAR,
  QUADRILATERAL,
  TRIANGLE,
  HEXAGON,
  SPHERICAL,
  CYLINDRICAL,
  BRICK,
  LAGRANGE_BRICK,
  TETRAHEDRON,
  HEXPRISM,
  PRISMATIC,
  OTHER
};

enum class surfaceBCTag {
  REFLECTIVE,
  VOID
};

struct blockType {
  int numElementsInBlock;
  int numBoundarySurfacesInBlock;
  int nodesPerElement;
  int ordIntrp;
  int ordEquat;
  int numSurfPerEleInBlock;
  elementType eTpe;
  std::string regionName;
  std::vector<int> elmIds;
  std::vector<surfaceBCTag> srfBCTag;
  std::vector<int> srfBCEleRef;
  std::vector<int> glbSrfId;
  std::vector<int> adjBlkId;
  std::vector<int> adjElmId;
  std::vector<int> adjRefId;
  std::vector<std::vector<int>> eConn;
};

using BlockMap = std::vector<blockType>;

VTKCellType p2vEMap(elementType et);
elementType v2pEMap(VTKCellType vt);
surfaceBCTag bcTagNum(const std::string &tag);
std::string bcTagStr(surfaceBCTag tag);
elementType elmTypeNum(const std::string &tag);
std::string elmTypeStr(elementType tag);
int elmNumNde(elementType et, int order);
int elmNumSrf(elementType et);


class  NEMOSYS_DEPRECATED_EXPORT pntMesh
{
  public:
    pntMesh();
    explicit pntMesh(const std::string &ifname);
    pntMesh(const meshBase *imb, int dim, int nBlk, const BlockMap &elmBlkMap);

    ~pntMesh() = default;

  public:
    bool isCompatible() const { return isSupported; }
    int getNumberOfPoints() const { return numVertices; }
    int getNumberOfCells() const { return numElements; }
    int getNumberOfBlocks() const { return numBlocks; }

    std::vector<double> getPointCrd(int id) const { return pntCrds[id]; }
    std::vector<int> getElmConn(int id) const { return elmConn[id]; }

    std::vector<int> getElmConn(int id, VTKCellType vct) const;
    std::vector<int>
    getPntConn(std::vector<int> &ci, elementType et, int eo) const;
    std::string getBlockName(int id) const;
    elementType getBlockElmType(int id) const;

    elementType getElmType(int id) const;
    int getElmOrder(int id) const;
    VTKCellType getVtkCellTag(elementType et, int order) const;

    void write(const std::string &fname) const;

  private:
    // populate quantities needed to write
    // pntmesh file
    void pntPopulate(const meshBase *imb);
    void updElmBlk(int blkId);

  private:
    std::string ifname;
    int numVertices;
    int numElements;
    int numDimensions;
    int numBlocks;
    int numSurfaces;
    int numSurfInternal;
    int numSurfBoundary;
    std::vector<std::vector<double>> pntCrds;
    std::vector<std::vector<int>> elmConn;     // connectivities
    std::vector<elementType> elmTyp;           // type
    std::vector<int> elmOrd;                   // order
    std::vector<int> elmBlkId;                 // element block number
    std::vector<int> elmLocalId;               // elements local id indexed by global ids
    std::vector<std::vector<int>> elmSrfId;    // element surface/edge Ids
    std::vector<blockType> elmBlks;

    bool isSupported; // false is non Tri/Tet elements were found

    // pnt topological information
    std::set<std::set<int>> connSet;
    std::map<std::set<int>, int> surfConnToId;
    std::map<int, std::set<int>> surfIdToConn;
    std::vector<bool> surfOnBndr;
    std::map<int, std::vector<std::pair<int, int>>> surfAdjRefNum;
    std::map<int, std::vector<int> > surfAdjElmNum;
};

} // namespace PNTMesh

#endif  // NEMOSYS_PNTMESH_H_
