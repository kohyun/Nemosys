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
#include "Mesh/foamMesh.H"

#include <iostream>
#include <string>

#include <boost/filesystem.hpp>

// openfoam headers
#include <cellModeller.H>
#include <fileName.H>
#include <foamVtkVtuAdaptor.H>
#include <fvMesh.H>

// vtk
#include <vtkCellData.h>
#include <vtkCellIterator.h>
#include <vtkCellTypes.h>
#include <vtkDataSetSurfaceFilter.h>
#include <vtkExtractEdges.h>
#include <vtkGenericCell.h>
#include <vtkIdList.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkTriangleFilter.h>
#include <vtkUnstructuredGrid.h>

#include "AuxiliaryFunctions.H"

namespace FOAM {

//////////////////////////////////
// foamMesh class
//////////////////////////////////

foamMesh::foamMesh(bool readDB) {
  // initialize openfoam
  int argc = 1;
  char **argv = new char *[2];
  argv[0] = new char[100];
  strcpy(argv[0], "NONE");
  _args = new Foam::argList(argc, argv);
  Foam::Info << "Create time\n" << Foam::endl;
  _runTime = new Foam::Time(Foam::Time::controlDictName, *_args);
  Foam::argList::noParallel();

  // reading mesh database from current location
  if (readDB) read("");
}

foamMesh::foamMesh(std::shared_ptr<meshBase> fullMesh) {
  _volMB = fullMesh;
  createFoamDicts();
}

foamMesh::~foamMesh() {
  if (_args) {
    delete _args;
    _args = nullptr;
  }
  if (_runTime) {
    delete _runTime;
    _runTime = nullptr;
  }
  if (_fmesh) {
    delete _fmesh;
    _fmesh = nullptr;
  }
  std::cout << "foamMesh destroyed" << std::endl;
}

void foamMesh::read(const std::string &fname) {
  // openFOAM's mesh information is distributed through
  // multiple files therefore fname is meaningless
  // this implementation reads current working directory
  // and finds mesh data files automatically if they exist
  // TODO: Error out when information are not existing

  _fmesh = nullptr;

  // reading mesh database and converting
  Foam::word regionName;
  if (_args->readIfPresent("region", regionName)) {
    Foam::Info << "Create mesh " << regionName
               << " for time = " << _runTime->timeName() << Foam::nl
               << Foam::endl;
  } else {
    if (fname == "domain1")
      regionName = "domain1";
    else if (fname == "domain0")
      regionName = "domain0";
    else if (fname == "domain2")
      regionName = "domain2";
    else if (fname == "domain100")
      regionName = "domain100";
    else {
      regionName = Foam::fvMesh::defaultRegion;
      Foam::Info << "Create mesh for time = " << _runTime->timeName()
                 << Foam::nl << Foam::endl;
    }
  }

  _fmesh = new Foam::fvMesh(Foam::IOobject(
      regionName, _runTime->timeName(), *_runTime, Foam::IOobject::MUST_READ));

  genMshDB();
}

void foamMesh::genMshDB() {
  // creating equivalent vtk topology from fvMesh
  // by default polyhedral cells will be decomposed to
  // tets and pyramids. Additional points will be added
  // to underlying fvMesh.
  std::cout << "Performing topological decomposition.\n";
  auto objVfoam = Foam::vtk::vtuAdaptor();
  dataSet = objVfoam.internal(*_fmesh);

  // Obtaining the mesh data and generating requested output file
  numPoints = dataSet->GetNumberOfPoints();
  numCells = dataSet->GetNumberOfCells();
  std::cout << "Number of points " << numPoints << std::endl;
  std::cout << "Number of cells " << numCells << std::endl;
}

// get point with id
std::vector<double> foamMesh::getPoint(nemId_t id) const {
  double coords[3];
  dataSet->GetPoint(id, coords);
  std::vector<double> result(coords, coords + 3);
  return result;
}

// get 3 vectors with x,y and z coords
std::vector<std::vector<double>> foamMesh::getVertCrds() const {
  std::vector<std::vector<double>> comp_crds(3);
  for (int i = 0; i < 3; ++i) { comp_crds[i].resize(numPoints); }
  double coords[3];
  for (int i = 0; i < numPoints; ++i) {
    dataSet->GetPoint(i, coords);
    comp_crds[0][i] = coords[0];
    comp_crds[1][i] = coords[1];
    comp_crds[2][i] = coords[2];
  }
  return comp_crds;
}

// get cell with id : returns point indices and respective coordinates
std::map<nemId_t, std::vector<double>> foamMesh::getCell(nemId_t id) const {
  if (id < numCells) {
    std::map<nemId_t, std::vector<double>> cell;
    vtkSmartPointer<vtkIdList> point_ids = vtkSmartPointer<vtkIdList>::New();
    point_ids = dataSet->GetCell(id)->GetPointIds();
    vtkIdType num_ids = point_ids->GetNumberOfIds();
    for (vtkIdType i = 0; i < num_ids; ++i) {
      nemId_t pntId = point_ids->GetId(i);
      std::vector<double> coord = getPoint(pntId);
      cell.insert(std::pair<nemId_t, std::vector<double>>(pntId, coord));
    }

    return cell;
  } else {
    std::cerr << "Cell ID is out of range!" << std::endl;
    exit(1);
  }
}

std::vector<std::vector<double>> foamMesh::getCellVec(nemId_t id) const {
  if (id < numCells) {
    std::vector<std::vector<double>> cell;
    vtkSmartPointer<vtkIdList> point_ids = vtkSmartPointer<vtkIdList>::New();
    point_ids = dataSet->GetCell(id)->GetPointIds();
    vtkIdType num_ids = point_ids->GetNumberOfIds();
    cell.resize(num_ids);
    for (vtkIdType i = 0; i < num_ids; ++i) {
      nemId_t pntId = point_ids->GetId(i);
      cell[i] = getPoint(pntId);
    }
    return cell;
  } else {
    std::cerr << "Cell ID is out of range!" << std::endl;
    exit(1);
  }
}

void foamMesh::inspectEdges(const std::string &ofname) const {
  std::ofstream outputStream(ofname);
  if (!outputStream.good()) {
    std::cerr << "error opening " << ofname << std::endl;
    exit(1);
  }

  vtkSmartPointer<vtkExtractEdges> extractEdges =
      vtkSmartPointer<vtkExtractEdges>::New();
  extractEdges->SetInputData(dataSet);
  extractEdges->Update();

  vtkSmartPointer<vtkGenericCell> genCell =
      vtkSmartPointer<vtkGenericCell>::New();
  for (int i = 0; i < extractEdges->GetOutput()->GetNumberOfCells(); ++i) {
    extractEdges->GetOutput()->GetCell(i, genCell);
    vtkPoints *points = genCell->GetPoints();
    double p1[3], p2[3];
    points->GetPoint(0, p1);
    points->GetPoint(1, p2);
    double len = std::sqrt(pow(p1[0] - p2[0], 2) + pow(p1[1] - p2[1], 2) +
                           pow(p1[2] - p2[2], 2));
    outputStream << len << std::endl;
  }
}

std::vector<nemId_t> foamMesh::getConnectivities() const {
  std::vector<nemId_t> connectivities;
  vtkSmartPointer<vtkCellIterator> it =
      vtkSmartPointer<vtkCellIterator>::Take(dataSet->NewCellIterator());
  for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextCell()) {
    vtkSmartPointer<vtkIdList> pointIds = it->GetPointIds();
    for (vtkIdType i = 0; i < pointIds->GetNumberOfIds(); ++i) {
      connectivities.push_back(pointIds->GetId(i));
    }
  }
  return connectivities;
}

void foamMesh::report() const {
  if (!dataSet) {
    std::cout << "dataSet has not been populated!" << std::endl;
    exit(1);
  }

  using CellContainer = std::map<int, int>;
  // Generate a report
  std::cout << "Processing the dataset generated from " << filename << std::endl
            << " dataSet contains a " << dataSet->GetClassName() << " that has "
            << numCells << " cells"
            << " and " << numPoints << " points." << std::endl;

  CellContainer cellMap;
  for (int i = 0; i < numCells; i++) { cellMap[dataSet->GetCellType(i)]++; }

  CellContainer::const_iterator it = cellMap.begin();
  while (it != cellMap.end()) {
    std::cout << "\tCell type "
              << vtkCellTypes::GetClassNameFromTypeId(it->first) << " occurs "
              << it->second << " times." << std::endl;
    ++it;
  }

  // Now check for point data
  vtkPointData *pd = dataSet->GetPointData();
  if (pd) {
    std::cout << " contains point data with " << pd->GetNumberOfArrays()
              << " arrays." << std::endl;
    for (int i = 0; i < pd->GetNumberOfArrays(); i++) {
      std::cout << "\tArray " << i << " is named "
                << (pd->GetArrayName(i) ? pd->GetArrayName(i) : "NULL");
      vtkDataArray *da = pd->GetArray(i);
      std::cout << " with " << da->GetNumberOfTuples() << " values. "
                << std::endl;
    }
  }
  // Now check for cell data
  vtkCellData *cd = dataSet->GetCellData();
  if (cd) {
    std::cout << " contains cell data with " << cd->GetNumberOfArrays()
              << " arrays." << std::endl;
    for (int i = 0; i < cd->GetNumberOfArrays(); i++) {
      std::cout << "\tArray " << i << " is named "
                << (cd->GetArrayName(i) ? cd->GetArrayName(i) : "NULL");
      vtkDataArray *da = cd->GetArray(i);
      std::cout << " with " << da->GetNumberOfTuples() << " values. "
                << std::endl;
    }
  }
  // Now check for field data
  if (dataSet->GetFieldData()) {
    std::cout << " contains field data with "
              << dataSet->GetFieldData()->GetNumberOfArrays() << " arrays."
              << std::endl;
    for (int i = 0; i < dataSet->GetFieldData()->GetNumberOfArrays(); i++) {
      std::cout << "\tArray " << i << " is named "
                << dataSet->GetFieldData()->GetArray(i)->GetName();
      vtkDataArray *da = dataSet->GetFieldData()->GetArray(i);
      std::cout << " with " << da->GetNumberOfTuples() << " values. "
                << std::endl;
    }
  }
}

// get diameter of circumsphere of each cell
std::vector<double> foamMesh::getCellLengths() const {
  std::vector<double> result;
  result.resize(getNumberOfCells());
  for (nemId_t i = 0; i < getNumberOfCells(); ++i) {
    result[i] = std::sqrt(dataSet->GetCell(i)->GetLength2());
  }
  return result;
}

// get center of a cell
std::vector<double> foamMesh::getCellCenter(nemId_t cellID) const {
  std::vector<double> center(3, 0.0);
  std::vector<std::vector<double>> cell = getCellVec(cellID);

  using nemAux::operator*;  // for vector multiplication.
  using nemAux::operator+;  // for vector addition.
  for (int i = 0; i < cell.size(); ++i) { center = center + cell[i]; }
  return (1. / cell.size()) * center;
}

// returns the cell type
int foamMesh::getCellType() const { return dataSet->GetCellType(0); }

vtkSmartPointer<vtkDataSet> foamMesh::extractSurface() {
  // extract surface polygons
  vtkSmartPointer<vtkDataSetSurfaceFilter> surfFilt =
      vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
  surfFilt->SetInputData(dataSet);
  surfFilt->Update();

  // triangulate the surface
  vtkSmartPointer<vtkTriangleFilter> triFilt =
      vtkSmartPointer<vtkTriangleFilter>::New();
  triFilt->SetInputData(surfFilt->GetOutput());
  triFilt->Update();

  return triFilt->GetOutput();
}

// Created foam dictionaries for VTK->FOAM conversion
void foamMesh::createFoamDicts() {
  // fvSchemesDict

  // creating a base system directory
  const char dir_path[] = "./system";
  boost::filesystem::path dir(dir_path);
  try {
    boost::filesystem::create_directory(dir);
  } catch (boost::filesystem::filesystem_error &e) {
    std::cerr << "Problem in creating system directory for the cfMesh"
              << "\n";
    std::cerr << e.what() << std::endl;
    throw;
  }

  std::ofstream contDict;
  contDict.open(std::string(dir_path) + "/fvSchemes");
  std::string contText =
      "\
/*--------------------------------*- C++ -*----------------------------------*\n\
| =========                 |                                                |\n\
| \\\\      /  F ield         | NEMoSys: Mesh Conversion interface             |\n\
|  \\\\    /   O peration     |                                                |\n\
|   \\\\  /    A nd           |                                                |\n\
|    \\\\/     M anipulation  |                                                |\n\
\\*---------------------------------------------------------------------------*/\n\
\n\
FoamFile\n\
{\n\
    version   2.0;\n\
    format    ascii;\n\
    class     dictionary;\n\
    object    fvSchemes;\n\
}\n\n\
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //\n\n\
gradSchemes\n\
{\n\
    default         Gauss linear;\n\
    grad(p)         Gauss linear;\n\
}\n\
\n\
divSchemes\n\
{\n\
    default         none;\n\
    div(phi,U)      Gauss linear;\n\
}\n\
\n\
laplacianSchemes\n\
{\n\
    default         none;\n\
    laplacian(nu,U) Gauss linear corrected;\n\
    laplacian((1|A(U)),p) Gauss linear corrected;\n\
}\n\
// ************************************************************************* //";
  contDict << contText;
  contDict.close();

  // fvSolutionDict

  std::ofstream contDict2;
  contDict2.open(std::string(dir_path) + "/fvSolution");
  std::string contText2 =
      "\
/*--------------------------------*- C++ -*----------------------------------*\n\
| =========                 |                                                |\n\
| \\\\      /  F ield         | NEMoSys: Mesh Conversion interface             |\n\
|  \\\\    /   O peration     |                                                |\n\
|   \\\\  /    A nd           |                                                |\n\
|    \\\\/     M anipulation  |                                                |\n\
\\*---------------------------------------------------------------------------*/\n\
\n\
FoamFile\n\
{\n\
    version   2.0;\n\
    format    ascii;\n\
    class     dictionary;\n\
    object    fvSolution;\n\
}\n\n\
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //\n\n\
\n\
// ************************************************************************* //";
  contDict2 << contText2;
  contDict2.close();

  // ControlDict

  std::ofstream contDict3;
  contDict3.open(std::string(dir_path) + "/controlDict");
  std::string contText3 =
      "\
/*--------------------------------*- C++ -*----------------------------------*\n\
| =========                 |                                                |\n\
| \\\\      /  F ield         | NEMoSys: Mesh Conversion interface             |\n\
|  \\\\    /   O peration     |                                                |\n\
|   \\\\  /    A nd           |                                                |\n\
|    \\\\/     M anipulation  |                                                |\n\
\\*---------------------------------------------------------------------------*/\n\
\n\
FoamFile\n\
{\n\
    version   2.0;\n\
    format    ascii;\n\
    class     dictionary;\n\
    location  \"system\";\n\
    object    controlDict;\n\
}\n\n\
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //\n\n\
deltaT  1;\n\n\
startTime 0;\n\n\
writeInterval 1;\n\n\
// ************************************************************************* //";
  contDict3 << contText3;
  contDict3.close();
}

// write the mesh to file named fname
void foamMesh::write(const std::string &fname) const {
  int argc = 1;
  char **argv = new char *[2];
  argv[0] = new char[100];
  strcpy(argv[0], "NONE");
  Foam::argList args(argc, argv);
  Foam::Info << "Create time\n" << Foam::endl;
  Foam::argList::noParallel();

  Foam::Time runTime(Foam::Time::controlDictName, Foam::fileName("."),
                     Foam::fileName("."));

  // Fetches VTK dataset from VTU/VTK files
  vtkSmartPointer<vtkDataSet> vtkDS = _volMB->getDataSet();

  int numPoints = vtkDS->GetNumberOfPoints();
  int numCells = vtkDS->GetNumberOfCells();

  // Gets all point numbers and coordinates
  std::vector<std::vector<double>> verticeZZ;
  verticeZZ.resize(numPoints);
  for (int ipt = 0; ipt < numPoints; ipt++) {
    std::vector<double> getPt = std::vector<double>(3);
    vtkDS->GetPoint(ipt, &getPt[0]);
    verticeZZ[ipt].resize(3);
    verticeZZ[ipt][0] = getPt[0];
    verticeZZ[ipt][1] = getPt[1];
    verticeZZ[ipt][2] = getPt[2];
  }

  // Gets Ids for cells
  std::vector<std::vector<int>> cellIdZZ;
  cellIdZZ.resize(numCells);

  // Foam point data container
  Foam::pointField pointData(numPoints);

  // Gets celltypes for all cells in mesh
  std::vector<int> typeCell;
  typeCell.resize(numCells);

  // Foam cell data container
  Foam::cellShapeList cellShapeData(numCells);

  for (int i = 0; i < numPoints; i++) {
    pointData[i] =
        Foam::vector(verticeZZ[i][0], verticeZZ[i][1], verticeZZ[i][2]);
  }

  for (int i = 0; i < numCells; i++) {
    vtkIdList *ptIds = vtkIdList::New();
    vtkDS->GetCellPoints(i, ptIds);
    int numIds = ptIds->GetNumberOfIds();
    cellIdZZ[i].resize(numIds);
    for (int j = 0; j < numIds; j++) { cellIdZZ[i][j] = ptIds->GetId(j); }

    Foam::labelList meshPoints(numIds);

    for (int k = 0; k < numIds; k++) { meshPoints[k] = cellIdZZ[i][k]; }

    typeCell[i] = vtkDS->GetCellType(i);

    if (typeCell[i] == 12) {
      cellShapeData[i] = Foam::cellShape("hex", meshPoints, true);
    } else if (typeCell[i] == 14) {
      cellShapeData[i] = Foam::cellShape("pyr", meshPoints, true);
    } else if (typeCell[i] == 10) {
      cellShapeData[i] = Foam::cellShape("tet", meshPoints, true);
    } else {
      std::cerr << "Only Hexahedral, Tetrahedral,"
                << " and Pyramid cells are supported for VTK->FOAM!"
                << std::endl;
      throw;
    }
  }

  Foam::polyMesh mesh(Foam::IOobject(Foam::polyMesh::defaultRegion,
                                     runTime.constant(), runTime),
                      std::move(pointData),       // Vertices
                      cellShapeData,              // Cell shape and points
                      Foam::faceListList(),       // Boundary faces
                      Foam::wordList(),           // Boundary Patch Names
                      Foam::wordList(),           // Boundary Patch Type
                      "defaultPatch",             // Default Patch Name
                      Foam::polyPatch::typeName,  // Default Patch Type
                      Foam::wordList()  // Boundary Patch Physical Type
  );
  // ****************************************************************** //

  std::cout << "Writing mesh for time 0" << std::endl;

  mesh.write();
}

void foamMesh::readAMR(const Foam::Time &runTime) {
  _fmesh = new Foam::fvMesh(Foam::IOobject(Foam::fvMesh::defaultRegion,
                                           runTime.timeName(), runTime,
                                           Foam::IOobject::MUST_READ));

  genMshDB();
}

}  // namespace FOAM
