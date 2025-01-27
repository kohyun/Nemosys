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
#include "Drivers/TransferDriver.H"
#include "Mesh/vtkMesh.H"

#include <sstream>

#include <vtkAppendFilter.h>
#include <vtkCellData.h>
#include <vtkCellIterator.h>
#include <vtkCellTypes.h>
#include <vtkDataSetReader.h>
#include <vtkDataSetSurfaceFilter.h>
#include <vtkDoubleArray.h>
#include <vtkExtractEdges.h>
#include <vtkFieldData.h>
#include <vtkGenericCell.h>
#include <vtkGenericDataObjectReader.h>
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkRectilinearGrid.h>
#include <vtkSTLReader.h>
#include <vtkSTLWriter.h>
#include <vtkStructuredGrid.h>
#include <vtkTriangleFilter.h>
#include <vtkUnstructuredGridWriter.h>
#include <vtkXMLImageDataReader.h>
#include <vtkXMLImageDataWriter.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkXMLPolyDataWriter.h>
#include <vtkXMLRectilinearGridReader.h>
#include <vtkXMLRectilinearGridWriter.h>
#include <vtkXMLStructuredGridReader.h>
#include <vtkXMLStructuredGridWriter.h>
#include <vtkXMLUnstructuredGridReader.h>
#include <vtkXMLUnstructuredGridWriter.h>
#include <vtkXMLWriter.h>
#include <vtksys/SystemTools.hxx>

#include "AuxiliaryFunctions.H"

using nemAux::operator*;  // for vector multiplication.
using nemAux::operator+;  // for vector addition.

void vtkMesh::write(const std::string &fname) const {
  if (!dataSet) {
    std::cout << "No dataSet to write!" << std::endl;
    exit(1);
  }

  std::string extension = nemAux::find_ext(fname);

  if (extension == ".vtp")
    writeVTFile<vtkXMLPolyDataWriter>(fname, dataSet);
  else if (extension == ".vts")
    writeVTFile<vtkXMLStructuredGridWriter>(fname, dataSet);
  else if (extension == ".vtr")
    writeVTFile<vtkXMLRectilinearGridWriter>(fname, dataSet);
  else if (extension == ".vti")
    writeVTFile<vtkXMLImageDataWriter>(fname, dataSet);
  else if (extension == ".stl")
    writeVTFile<vtkSTLWriter>(fname, dataSet);  // ascii stl
  else if (extension == ".vtk")
    writeVTFile<vtkUnstructuredGridWriter>(fname,
                                           dataSet);  // legacy vtk writer
  else {
    std::string fname_tmp = nemAux::trim_fname(fname, ".vtu");
    // default is vtu
    writeVTFile<vtkXMLUnstructuredGridWriter>(fname_tmp, dataSet);
  }
}

vtkMesh::vtkMesh(vtkSmartPointer<vtkDataSet> dataSet_tmp,
                 const std::string &fname) {
  if (dataSet_tmp) {
    dataSet = dataSet_tmp;
    filename = fname;
    numCells = dataSet->GetNumberOfCells();
    numPoints = dataSet->GetNumberOfPoints();
    std::cout << "vtkMesh constructed" << std::endl;
  } else {
    std::cerr << "Nothing to copy!" << std::endl;
    exit(1);
  }
}

vtkMesh::vtkMesh(const std::vector<double> &xCrds,
                 const std::vector<double> &yCrds,
                 const std::vector<double> &zCrds,
                 const std::vector<nemId_t> &elemConn, const int cellType,
                 const std::string &newname) {
  if (!(xCrds.size() == yCrds.size() && xCrds.size() == zCrds.size())) {
    std::cerr << "Length of coordinate arrays must match!" << std::endl;
    exit(1);
  }

  // point to be pushed into dataSet
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  // declare vtk dataset
  vtkSmartPointer<vtkUnstructuredGrid> dataSet_tmp =
      vtkSmartPointer<vtkUnstructuredGrid>::New();
  numPoints = xCrds.size();
  // allocate size for vtk point container
  points->SetNumberOfPoints(numPoints);
  for (nemId_t i = 0; i < numPoints; ++i)
    points->SetPoint(i, xCrds[i], yCrds[i], zCrds[i]);

  // add points to vtk mesh data structure
  dataSet_tmp->SetPoints(points);
  switch (cellType) {
    case VTK_TETRA: {
      numCells = elemConn.size() / 4;
      dataSet_tmp->Allocate(numCells);
      for (nemId_t i = 0; i < numCells; ++i) {
        vtkSmartPointer<vtkIdList> elmConn = vtkSmartPointer<vtkIdList>::New();
        elmConn->SetNumberOfIds(4);
        for (nemId_t j = 0; j < 4; ++j) elmConn->SetId(j, elemConn[i * 4 + j]);
        dataSet_tmp->InsertNextCell(VTK_TETRA, elmConn);
      }
      break;
    }
    case VTK_TRIANGLE: {
      numCells = elemConn.size() / 3;
      dataSet_tmp->Allocate(numCells);
      for (nemId_t i = 0; i < numCells; ++i) {
        vtkSmartPointer<vtkIdList> elmConn = vtkSmartPointer<vtkIdList>::New();
        elmConn->SetNumberOfIds(3);
        for (nemId_t j = 0; j < 3; ++j) elmConn->SetId(j, elemConn[i * 3 + j]);
        dataSet_tmp->InsertNextCell(VTK_TRIANGLE, elmConn);
      }
      break;
    }
    default: {
      std::cout << "Unknown element type " << cellType << std::endl;
      exit(1);
    }
  }
  filename = newname;
  dataSet = dataSet_tmp;
  std::cout << "vtkMesh constructed" << std::endl;
}

vtkMesh::vtkMesh(const std::string &fname) {
  bool degenerate(false);
  std::string extension = vtksys::SystemTools::GetFilenameLastExtension(fname);
  // Dispatch based on the file extension
  if (extension == ".vtu")
    dataSet.TakeReference(
        ReadAnXMLOrSTLFile<vtkXMLUnstructuredGridReader>(fname));
  else if (extension == ".vtp")
    dataSet.TakeReference(ReadAnXMLOrSTLFile<vtkXMLPolyDataReader>(fname));
  else if (extension == ".vts")
    dataSet.TakeReference(
        ReadAnXMLOrSTLFile<vtkXMLStructuredGridReader>(fname));
  else if (extension == ".vtr")
    dataSet.TakeReference(
        ReadAnXMLOrSTLFile<vtkXMLRectilinearGridReader>(fname));
  else if (extension == ".vti")
    dataSet.TakeReference(ReadAnXMLOrSTLFile<vtkXMLImageDataReader>(fname));
  else if (extension == ".stl")
    dataSet.TakeReference(ReadAnXMLOrSTLFile<vtkSTLReader>(fname));
  else if (extension == ".vtk") {
    // if vtk is produced by MFEM, it's probably degenerate (i.e. point
    // duplicity) in this case, we use a different reader to correct duplicity
    // and transfer the data attributes read by the regular legacy reader using
    // the transfer methods in meshBase
    std::ifstream meshStream(fname);
    if (!meshStream.good()) {
      std::cout << "Error opening file " << fname << std::endl;
      exit(1);
    }
    std::string line;
    getline(meshStream, line);
    getline(meshStream, line);
    meshStream.close();
    if (line.find("MFEM") != std::string::npos) {
      degenerate = true;
      dataSet = vtkDataSet::SafeDownCast(ReadDegenerateVTKFile(fname));
      setFileName(fname);
      numCells = dataSet->GetNumberOfCells();
      numPoints = dataSet->GetNumberOfPoints();
      vtkMesh *vtkMesh_tmp = new vtkMesh(
          vtkDataSet::SafeDownCast(ReadALegacyVTKFile(fname)), fname);
      // vtkMesh_tmp.transfer(this, "Consistent Interpolation");
      auto transfer = NEM::DRV::TransferDriver::CreateTransferObject(
          vtkMesh_tmp, this, "Consistent Interpolation");
      transfer->run(vtkMesh_tmp->getNewArrayNames());
      std::cout << "vtkMesh constructed" << std::endl;
    } else {
      // reading legacy formated vtk files
      vtkSmartPointer<vtkGenericDataObjectReader> reader =
          vtkSmartPointer<vtkGenericDataObjectReader>::New();
      // vtkSmartPointer<vtkDataSetReader> reader =
      //     vtkSmartPointer<vtkDataSetReader>::New();
      reader->SetFileName(fname.c_str());
      reader->Update();
      reader->GetOutput()->Register(reader);
      // obtaining dataset
      dataSet.TakeReference(vtkDataSet::SafeDownCast(reader->GetOutput()));
    }
  } else {
    std::cerr << "Unknown extension: " << extension << std::endl;
    exit(1);
  }

  if (!dataSet) {
    std::cout << "Error populating dataSet" << std::endl;
    exit(1);
  }

  if (!degenerate) {
    setFileName(fname);
    std::cout << "vtkMesh constructed" << std::endl;
    numCells = dataSet->GetNumberOfCells();
    numPoints = dataSet->GetNumberOfPoints();
  }
}

vtkMesh::vtkMesh(const std::string &fname1, const std::string &fname2) {
  std::string ext_in = vtksys::SystemTools::GetFilenameLastExtension(fname1);
  std::string ext_out = vtksys::SystemTools::GetFilenameLastExtension(fname2);

  // sanity check
  if (!(ext_in == ".vtu" && ext_out == ".stl")) {
    std::cerr
        << "vtkMesh: Currently only support conversion between vtu and stl."
        << std::endl;
    exit(1);
  }

  vtkSmartPointer<vtkXMLUnstructuredGridReader> reader =
      vtkSmartPointer<vtkXMLUnstructuredGridReader>::New();
  reader->SetFileName(fname1.c_str());
  reader->Update();
  reader->GetOutput()->Register(reader);

  // obtaining dataset
  dataSet.TakeReference(vtkDataSet::SafeDownCast(reader->GetOutput()));
  if (!dataSet) {
    std::cout << "Error populating dataSet" << std::endl;
    exit(1);
  }
  setFileName(fname1);
  std::cout << "vtkMesh constructed" << std::endl;
  numCells = dataSet->GetNumberOfCells();
  numPoints = dataSet->GetNumberOfPoints();

  // skinn to the surface
  vtkSmartPointer<vtkDataSetSurfaceFilter> surfFilt =
      vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
  surfFilt->SetInputConnection(reader->GetOutputPort());
  surfFilt->Update();

  // triangulate the surface
  vtkSmartPointer<vtkTriangleFilter> triFilt =
      vtkSmartPointer<vtkTriangleFilter>::New();
  triFilt->SetInputConnection(surfFilt->GetOutputPort());
  triFilt->Update();

  // write to stl file
  vtkSmartPointer<vtkSTLWriter> stlWriter =
      vtkSmartPointer<vtkSTLWriter>::New();
  stlWriter->SetFileName(fname2.c_str());
  stlWriter->SetInputConnection(triFilt->GetOutputPort());
  stlWriter->Write();
}

bool readLegacyVTKHeader(const std::string &line) {
  if (line.find("DATASET") != -1) {
    if (line.find("UNSTRUCTURED_GRID") == std::string::npos) {
      std::cerr << "Reading a " << line << " is not supported" << std::endl;
      exit(1);
    }
    return true;
  }
  return false;
}

bool readLegacyVTKFieldData(std::istream &meshStream, std::string &line,
                            vtkSmartPointer<vtkUnstructuredGrid> dataSet_tmp) {
  if (line.find("FIELD") != std::string::npos) {
    if (getline(meshStream, line)) {
      std::string arrname, type;
      int numComponent;
      nemId_t numTuple;
      std::stringstream ss;
      ss.str(line);
      ss >> arrname >> numComponent >> numTuple >> type;
      vtkSmartPointer<vtkDoubleArray> arr =
          vtkSmartPointer<vtkDoubleArray>::New();
      arr->SetName(arrname.c_str());
      arr->SetNumberOfComponents(numComponent);
      arr->SetNumberOfTuples(numTuple);
      getline(meshStream, line);
      ss.str(line);
      double val;
      for (int i = 0; i < numTuple; ++i) {
        ss >> val;
        arr->InsertTuple(i, &val);
      }
      dataSet_tmp->GetFieldData()->AddArray(arr);
      return true;
    }
  }
  return false;
}

bool readLegacyVTKPoints(std::istream &meshStream, std::string &line,
                         nemId_t &numPoints, vtkSmartPointer<vtkPoints> points,
                         vtkSmartPointer<vtkUnstructuredGrid> dataSet_tmp) {
  if (line.find("POINTS") != std::string::npos) {
    std::istringstream ss(line);
    std::string tmp;
    ss >> tmp >> numPoints;
    points->SetNumberOfPoints(numPoints);
    double point[3];
    nemId_t i = 0;
    while (getline(meshStream, line) && i < numPoints) {
      if (!line.empty()) {
        std::istringstream ss(line);
        ss >> point[0] >> point[1] >> point[2];
        points->SetPoint(i, point);
        ++i;
      }
    }
    dataSet_tmp->SetPoints(points);
    return true;
  }
  return false;
}

bool readLegacyVTKCells(std::istream &meshStream, std::string &line,
                        nemId_t &numCells,
                        std::vector<vtkSmartPointer<vtkIdList>> &vtkCellIds,
                        vtkSmartPointer<vtkUnstructuredGrid> dataSet_tmp) {
  if (line.find("CELLS") != std::string::npos) {
    std::istringstream ss(line);
    std::string tmp;
    ss >> tmp >> numCells;
    vtkCellIds.resize(numCells);
    // get cells
    nemId_t i = 0;
    while (i < numCells && getline(meshStream, line)) {
      if (!line.empty()) {
        std::istringstream ss(line);
        nemId_t numId;
        nemId_t id;
        ss >> numId;
        vtkCellIds[i] = vtkSmartPointer<vtkIdList>::New();
        vtkCellIds[i]->SetNumberOfIds(numId);
        for (nemId_t j = 0; j < numId; ++j) {
          ss >> id;
          vtkCellIds[i]->SetId(j, id);
        }
        ++i;
      }
    }
    // get cell types
    while (getline(meshStream, line)) {
      if (line.find("CELL_TYPES") != std::string::npos) {
        dataSet_tmp->Allocate(numCells);
        i = 0;
        while (i < numCells && getline(meshStream, line)) {
          if (!line.empty()) {
            std::istringstream ss(line);
            int cellType;
            ss >> cellType;
            dataSet_tmp->InsertNextCell(cellType, vtkCellIds[i]);
            ++i;
          }
        }
        break;
      }
    }
    return true;
  }
  return false;
}

bool readLegacyVTKData(std::ifstream &meshStream, std::string &line,
                       const nemId_t numTuple, const bool pointOrCell,
                       bool &hasPointOrCell,
                       vtkSmartPointer<vtkUnstructuredGrid> dataSet_tmp) {
  // determines whether more than one point or cell array in dataSet
  bool inProgress(true);
  // while there is a cell or point array to be read
  while (inProgress) {
    int numComponent(0);
    std::string line, attribute, name, type;
    vtkSmartPointer<vtkDoubleArray> arr =
        vtkSmartPointer<vtkDoubleArray>::New();
    while (getline(meshStream, line)) {
      if (!line.empty()) {
        if (line.find("LOOKUP_TABLE default") != std::string::npos) break;

        size_t hasScalar = line.find("SCALARS");
        size_t hasVector = line.find("VECTORS");
        size_t hasNormal = line.find("NORMALS");
        if (hasScalar == std::string::npos && hasVector == std::string::npos &&
            hasNormal == std::string::npos) {
          std::cerr << "attribute type in " << line << " not supported"
                    << std::endl;
          exit(1);
        } else {
          std::istringstream ss(line);
          ss >> attribute >> name >> type >> numComponent;
          if (hasScalar != std::string::npos)
            numComponent =
                numComponent > 4 || numComponent < 1 ? 1 : numComponent;
          else if (hasVector != std::string::npos ||
                   hasNormal != std::string::npos)
            numComponent = 3;
          arr->SetName(name.c_str());
          arr->SetNumberOfComponents(numComponent);
          arr->SetNumberOfTuples(numTuple);
          // std::cout << arr->GetName() << " " << arr->GetNumberOfComponents()
          //           << " " << arr->GetNumberOfTuples() << std::endl;
          break;
        }
      }
    }

    nemId_t i = 0;
    while (i < numTuple && getline(meshStream, line)) {
      if (!line.empty() && line.find("LOOKUP") == std::string::npos) {
        std::istringstream ss(line);
        // double vals[numComponent];
        std::vector<double> vals;
        vals.resize(numComponent, 0.);

        for (int j = 0; j < numComponent; ++j) ss >> vals[j];
        arr->InsertTuple(i, vals.data());
        ++i;
      }
    }

    if (pointOrCell)
      dataSet_tmp->GetPointData()->AddArray(arr);
    else
      dataSet_tmp->GetCellData()->AddArray(arr);

    bool moreData(false);
    std::streampos curr = meshStream.tellg();
    while (getline(meshStream, line)) {
      // if point data encountered while looking for more cell data, we know we
      // read all cell data
      if (line.find("POINT_DATA") != std::string::npos && !pointOrCell) {
        curr = meshStream.tellg();
        hasPointOrCell = true;
        break;
      }
      // if cell data encountered while looking for more point data, we know we
      // read all point data
      if (line.find("CELL_DATA") != std::string::npos && pointOrCell) {
        hasPointOrCell = true;
        curr = meshStream.tellg();
        break;
      }
      // if the previous two conditions are not met and this one is,
      // we know we have more than one point/cell data array
      if (line.find("SCALARS") != std::string::npos ||
          line.find("VECTORS") != std::string::npos ||
          line.find("NORMALS") != std::string::npos) {
        moreData = true;
        break;
      }
    }

    meshStream.seekg(curr);

    if (!moreData) inProgress = false;
  }
  return true;
}

vtkSmartPointer<vtkUnstructuredGrid> ReadALegacyVTKFile(
    const std::string &fileName) {
  std::ifstream meshStream;
  meshStream.open(fileName, std::ifstream::in);
  if (!meshStream.good()) {
    std::cerr << "Could not open file " << fileName << std::endl;
    exit(1);
  }

  // declare points to be pushed into dataSet_tmp
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  // declare vector of cell ids to be pushed into dataSet_tmp
  std::vector<vtkSmartPointer<vtkIdList>> vtkCellIds;
  // declare dataSet_tmp which will be associated to output vtkMesh
  vtkSmartPointer<vtkUnstructuredGrid> dataSet_tmp =
      vtkSmartPointer<vtkUnstructuredGrid>::New();
  nemId_t numPoints(0), numCells(0);
  bool readHeader(false), readPoints(false), readCells(false),
      readFieldData(false), readCellData(false), readPointData(false),
      cellDataFirst(false), pointDataFirst(false), hasPointData(false),
      hasCellData(false);
  std::string line;

  while (getline(meshStream, line)) {
    if (!readHeader)
      if ((readHeader = readLegacyVTKHeader(line)))
        std::cout << "read header" << std::endl;

    if (!readFieldData)
      if ((readFieldData =
               readLegacyVTKFieldData(meshStream, line, dataSet_tmp)))
        std::cout << "read field data" << std::endl;

    if (!readPoints)
      if ((readPoints = readLegacyVTKPoints(meshStream, line, numPoints, points,
                                            dataSet_tmp)))
        std::cout << "read points" << std::endl;

    if (!readCells)
      if ((readCells = readLegacyVTKCells(meshStream, line, numCells,
                                          vtkCellIds, dataSet_tmp)))
        std::cout << "read cells" << std::endl;

    // call functions based on which data appears first in file
    if (!readCellData && !readPointData) {
      cellDataFirst =
          line.find("CELL_DATA") != std::string::npos && !pointDataFirst;
      pointDataFirst =
          line.find("POINT_DATA") != std::string::npos && !cellDataFirst;
    }

    // read cell data then point data if it exists
    if (cellDataFirst) {
      if (!readCellData)  // boolean telling us if there is point data
        if ((readCellData = readLegacyVTKData(meshStream, line, numCells, false,
                                              hasPointData, dataSet_tmp)))
          std::cout << "read cell data" << std::endl;

      if (!readPointData && hasPointData)
        if ((readPointData = readLegacyVTKData(meshStream, line, numPoints,
                                               true, hasCellData, dataSet_tmp)))
          std::cout << "read point data" << std::endl;
    }
    // read point data and then cell data if it exists
    else if (pointDataFirst) {
      if (!readPointData)
        if ((readPointData = readLegacyVTKData(meshStream, line, numPoints,
                                               true, hasCellData, dataSet_tmp)))
          std::cout << "read point data" << std::endl;

      if (!readCellData && hasCellData)
        if ((readCellData = readLegacyVTKData(meshStream, line, numCells, false,
                                              hasPointData, dataSet_tmp)))
          std::cout << "read cell data" << std::endl;
    }
  }
  return dataSet_tmp;
}

vtkSmartPointer<vtkUnstructuredGrid> ReadDegenerateVTKFile(
    const std::string &fileName) {
  std::ifstream meshStream(fileName);
  if (!meshStream.good()) {
    std::cerr << "Error opening file " << fileName << std::endl;
    exit(1);
  }

  std::string line;
  std::map<std::vector<double>, int> point_map;
  std::map<int, int> duplicate_point_map;
  std::pair<std::map<std::vector<double>, int>::iterator, bool> point_ret;
  int numPoints;
  int numCells;
  int cellListSize;
  std::vector<vtkSmartPointer<vtkIdList>> vtkCellIds;
  vtkSmartPointer<vtkUnstructuredGrid> dataSet_tmp =
      vtkSmartPointer<vtkUnstructuredGrid>::New();
  while (getline(meshStream, line)) {
    if (line.find("POINTS") != std::string::npos) {
      std::istringstream ss(line);
      std::string tmp;
      ss >> tmp >> numPoints;
      std::vector<double> point(3);
      int pntId = 0;
      int realPntId = 0;
      while (getline(meshStream, line) && pntId < numPoints) {
        if (!line.empty()) {
          std::istringstream ss(line);
          ss >> point[0] >> point[1] >> point[2];
          point_ret = point_map.insert(
              std::pair<std::vector<double>, int>(point, realPntId));
          if (point_ret.second) ++realPntId;
          duplicate_point_map.insert(
              std::pair<int, int>(pntId, point_ret.first->second));
          ++pntId;
        }
      }
    }

    if (line.find("CELLS") != std::string::npos) {
      std::istringstream ss(line);
      std::string tmp;
      ss >> tmp >> numCells >> cellListSize;
      int cellId = 0;
      // int realCellId = 0;
      vtkCellIds.resize(numCells);
      while (cellId < numCells && getline(meshStream, line)) {
        if (!line.empty()) {
          std::istringstream ss(line);
          int numId;
          int id;
          ss >> numId;
          vtkCellIds[cellId] = vtkSmartPointer<vtkIdList>::New();
          vtkCellIds[cellId]->SetNumberOfIds(numId);
          // cells[cellId].resize(numId);
          for (int j = 0; j < numId; ++j) {
            ss >> id;
            vtkCellIds[cellId]->SetId(j, duplicate_point_map[id]);
            // cells[cellId][j] = duplicate_point_map[id];
          }
          ++cellId;
        }
      }
      // get cell types
      while (getline(meshStream, line)) {
        if (line.find("CELL_TYPES") != std::string::npos) {
          dataSet_tmp->Allocate(numCells);
          cellId = 0;
          // cellTypes.resize(numCells);
          while (cellId < numCells && getline(meshStream, line)) {
            if (!line.empty()) {
              std::istringstream ss(line);
              int cellType;
              ss >> cellType;
              // cellTypes[cellId] = cellType;
              dataSet_tmp->InsertNextCell(cellType, vtkCellIds[cellId]);
              ++cellId;
            }
          }
          break;
        }
      }
    }
  }

  std::multimap<int, std::vector<double>> flipped = nemAux::flip_map(point_map);
  auto flip_it = flipped.begin();

  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  points->SetNumberOfPoints(point_map.size());
  int pnt = 0;
  while (flip_it != flipped.end()) {
    points->SetPoint(pnt, flip_it->second.data());
    ++flip_it;
    ++pnt;
  }
  dataSet_tmp->SetPoints(points);
  return dataSet_tmp;
}

// get point with id
std::vector<double> vtkMesh::getPoint(nemId_t id) const {
  double coords[3];
  dataSet->GetPoint(id, coords);
  std::vector<double> result(coords, coords + 3);
  return result;
}

// get 3 vectors with x,y and z coords
std::vector<std::vector<double>> vtkMesh::getVertCrds() const {
  std::vector<std::vector<double>> comp_crds(3);
  for (int i = 0; i < 3; ++i) comp_crds[i].resize(numPoints);

  double coords[3];
  for (nemId_t i = 0; i < numPoints; ++i) {
    dataSet->GetPoint(i, coords);
    comp_crds[0][i] = coords[0];
    comp_crds[1][i] = coords[1];
    comp_crds[2][i] = coords[2];
  }
  return comp_crds;
}

// get cell with id : returns point indices and respective coordinates
std::map<nemId_t, std::vector<double>> vtkMesh::getCell(nemId_t id) const {
  if (id < numCells) {
    std::map<nemId_t, std::vector<double>> cell;
    vtkSmartPointer<vtkIdList> point_ids = vtkSmartPointer<vtkIdList>::New();
    point_ids = dataSet->GetCell(id)->GetPointIds();
    nemId_t num_ids = point_ids->GetNumberOfIds();
    for (nemId_t i = 0; i < num_ids; ++i) {
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

std::vector<std::vector<double>> vtkMesh::getCellVec(nemId_t id) const {
  if (id < numCells) {
    std::vector<std::vector<double>> cell;
    vtkSmartPointer<vtkIdList> point_ids = vtkSmartPointer<vtkIdList>::New();
    point_ids = dataSet->GetCell(id)->GetPointIds();
    nemId_t num_ids = point_ids->GetNumberOfIds();
    cell.resize(num_ids);
    for (nemId_t i = 0; i < num_ids; ++i) {
      nemId_t pntId = point_ids->GetId(i);
      cell[i] = getPoint(pntId);
    }
    return cell;
  } else {
    std::cerr << "Cell ID is out of range!" << std::endl;
    exit(1);
  }
}

void vtkMesh::inspectEdges(const std::string &ofname) const {
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
  for (nemId_t i = 0; i < extractEdges->GetOutput()->GetNumberOfCells(); ++i) {
    extractEdges->GetOutput()->GetCell(i, genCell);
    vtkPoints *points = genCell->GetPoints();
    double p1[3], p2[3];
    points->GetPoint(0, p1);
    points->GetPoint(1, p2);
    double len = sqrt(pow(p1[0] - p2[0], 2) + pow(p1[1] - p2[1], 2) +
                      pow(p1[2] - p2[2], 2));
    outputStream << len << std::endl;
  }
}

std::vector<nemId_t> vtkMesh::getConnectivities() const {
  std::vector<nemId_t> connectivities;
  vtkSmartPointer<vtkCellIterator> it =
      vtkSmartPointer<vtkCellIterator>::Take(dataSet->NewCellIterator());
  for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextCell()) {
    vtkSmartPointer<vtkIdList> pointIds = it->GetPointIds();
    for (nemId_t i = 0; i < pointIds->GetNumberOfIds(); ++i)
      connectivities.push_back(pointIds->GetId(i));
  }
  return connectivities;
}

void vtkMesh::report() const {
  if (!dataSet) {
    std::cout << "dataSet has not been populated!" << std::endl;
    exit(1);
  }

  using CellContainer = std::map<int, nemId_t>;
  // Generate a report
  std::cout << "Processing the dataset generated from " << filename << std::endl
            << " dataset contains a " << dataSet->GetClassName() << " that has "
            << numCells << " cells and " << numPoints << " points."
            << std::endl;

  CellContainer cellMap;
  for (nemId_t i = 0; i < numCells; i++) cellMap[dataSet->GetCellType(i)]++;

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
    for (int i = 0; i < pd->GetNumberOfArrays(); ++i) {
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
    for (int i = 0; i < cd->GetNumberOfArrays(); ++i) {
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
    for (int i = 0; i < dataSet->GetFieldData()->GetNumberOfArrays(); ++i) {
      std::cout << "\tArray " << i << " is named "
                << dataSet->GetFieldData()->GetArray(i)->GetName();
      vtkDataArray *da = dataSet->GetFieldData()->GetArray(i);
      std::cout << " with " << da->GetNumberOfTuples() << " values. "
                << std::endl;
    }
  }
}

vtkSmartPointer<vtkDataSet> vtkMesh::extractSurface() {
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

// get diameter of circumsphere of each cell
std::vector<double> vtkMesh::getCellLengths() const {
  std::vector<double> result;
  result.resize(getNumberOfCells());
  for (nemId_t i = 0; i < getNumberOfCells(); ++i)
    result[i] = std::sqrt(dataSet->GetCell(i)->GetLength2());

  return result;
}

// get center of a cell
std::vector<double> vtkMesh::getCellCenter(nemId_t cellID) const {
  std::vector<double> center(3, 0.0);
  std::vector<std::vector<double>> cell = getCellVec(cellID);

  for (const auto &i : cell) center = center + i;
  return 1. / cell.size() * center;
}

// returns the cell type
int vtkMesh::getCellType() const { return dataSet->GetCellType(0); }

// set point data
void vtkMesh::setPointDataArray(const std::string &name,
                                const std::vector<double> &data) {
  vtkSmartPointer<vtkDoubleArray> da = vtkSmartPointer<vtkDoubleArray>::New();
  da->SetName(name.c_str());
  da->SetNumberOfComponents(1);
  for (nemId_t i = 0; i < numPoints; i++) da->InsertNextTuple1(data[i]);
  dataSet->GetPointData()->AddArray(da);
}

// set point data (numComponents per point determined by dim of data[0]
void vtkMesh::setPointDataArray(const std::string &name,
                                const std::vector<std::vector<double>> &data) {
  vtkSmartPointer<vtkDoubleArray> da = vtkSmartPointer<vtkDoubleArray>::New();
  da->SetName(name.c_str());
  da->SetNumberOfComponents(data[0].size());
  for (nemId_t i = 0; i < numPoints; i++) da->InsertNextTuple(data[i].data());
  dataSet->GetPointData()->AddArray(da);
  // dataSet->GetPointData()->SetActiveScalars(name);
  // dataSet->GetPointData()->SetScalars(da);
}

void vtkMesh::getPointDataArray(const std::string &name,
                                std::vector<double> &data) {
  int idx;
  vtkSmartPointer<vtkDataArray> pd =
      dataSet->GetPointData()->GetArray(name.c_str(), idx);
  if (idx != -1) {
    if (pd->GetNumberOfComponents() > 1) {
      std::cerr << __func__
                << " is only suitable for scalar data, i.e. 1 component"
                << std::endl;
      exit(1);
    }
    data.resize(pd->GetNumberOfTuples());
    double x[1];
    for (nemId_t i = 0; i < pd->GetNumberOfTuples(); ++i) {
      pd->GetTuple(i, x);
      data[i] = x[0];
    }
  } else {
    std::cerr << "could not find data with name " << name << std::endl;
    exit(1);
  }
}

void vtkMesh::getPointDataArray(int arrayId, std::vector<double> &data) {
  if (arrayId < dataSet->GetPointData()->GetNumberOfArrays()) {
    vtkSmartPointer<vtkDataArray> pd =
        dataSet->GetPointData()->GetArray(arrayId);
    if (pd->GetNumberOfComponents() > 1) {
      std::cerr << __func__
                << " is only suitable for scalar data, i.e. 1 component"
                << std::endl;
      exit(1);
    }
    data.resize(pd->GetNumberOfTuples());
    double x[1];
    for (nemId_t i = 0; i < pd->GetNumberOfTuples(); ++i) {
      pd->GetTuple(i, x);
      data[i] = x[0];
    }
  } else {
    std::cerr << "arrayId exceeds number of point data arrays " << std::endl;
    exit(1);
  }
}

int vtkMesh::getCellDataIdx(const std::string &name) {
  // check physical group exist and obtain id
  int idx;
  dataSet->GetCellData()->GetArray(name.c_str(), idx);
  return idx;
}

void vtkMesh::getCellDataArray(const std::string &name,
                               std::vector<double> &data) {
  int idx;
  vtkSmartPointer<vtkDataArray> cd =
      dataSet->GetCellData()->GetArray(name.c_str(), idx);
  if (idx != -1) {
    if (cd->GetNumberOfComponents() > 1) {
      std::cerr << __func__
                << " is only suitable for scalar data, i.e. 1 component"
                << std::endl;
      exit(1);
    }
    data.resize(cd->GetNumberOfTuples());
    double x[1];
    for (nemId_t i = 0; i < cd->GetNumberOfTuples(); ++i) {
      cd->GetTuple(i, x);
      data[i] = x[0];
    }
  } else {
    std::cerr << "could not find data with name " << name << std::endl;
    exit(1);
  }
}

void vtkMesh::getCellDataArray(int arrayId, std::vector<double> &data) {
  if (arrayId < dataSet->GetCellData()->GetNumberOfArrays()) {
    vtkSmartPointer<vtkDataArray> cd =
        dataSet->GetCellData()->GetArray(arrayId);
    if (cd->GetNumberOfComponents() > 1) {
      std::cerr << __func__
                << " is only suitable for scalar data, i.e. 1 component"
                << std::endl;
      exit(1);
    }
    data.resize(cd->GetNumberOfTuples());
    double x[1];
    for (nemId_t i = 0; i < cd->GetNumberOfTuples(); ++i) {
      cd->GetTuple(i, x);
      data[i] = x[0];
    }
  } else {
    std::cerr << "arrayId exceeds number of cell data arrays " << std::endl;
    exit(1);
  }
}

// set cell data (numComponents per cell determined by dim of data[0])
void vtkMesh::setCellDataArray(const std::string &name,
                               const std::vector<std::vector<double>> &data) {
  vtkSmartPointer<vtkDoubleArray> da = vtkSmartPointer<vtkDoubleArray>::New();
  da->SetName(name.c_str());
  da->SetNumberOfComponents(data[0].size());
  for (nemId_t i = 0; i < numCells; i++) da->InsertNextTuple(data[i].data());
  dataSet->GetCellData()->AddArray(da);
  // dataSet->GetCellData()->SetActiveScalars(name);
  // dataSet->GetCellData()->SetScalars(da);
}

void vtkMesh::setCellDataArray(const std::string &name,
                               const std::vector<double> &data) {
  vtkSmartPointer<vtkDoubleArray> da = vtkSmartPointer<vtkDoubleArray>::New();
  da->SetName(name.c_str());
  da->SetNumberOfComponents(1);
  for (nemId_t i = 0; i < numCells; i++) da->InsertNextTuple1(data[i]);
  dataSet->GetCellData()->AddArray(da);
  // dataSet->GetCellData()->SetActiveScalars(name);
  // dataSet->GetCellData()->SetScalars(da);
}

// remove point data with given id from dataSet if it exists
void vtkMesh::unsetPointDataArray(int arrayID) {
  dataSet->GetPointData()->RemoveArray(arrayID);
}

void vtkMesh::unsetPointDataArray(const std::string &name) {
  dataSet->GetPointData()->RemoveArray(name.c_str());
}

// remove cell data with given id from dataSet if it exists
void vtkMesh::unsetCellDataArray(int arrayID) {
  dataSet->GetCellData()->RemoveArray(arrayID);
}

void vtkMesh::unsetCellDataArray(const std::string &name) {
  dataSet->GetCellData()->RemoveArray(name.c_str());
}

// remove field data with given id from dataSet
void vtkMesh::unsetFieldDataArray(const std::string &name) {
  dataSet->GetFieldData()->RemoveArray(name.c_str());
}

// Merges new dataset with existing dataset
void vtkMesh::merge(vtkSmartPointer<vtkDataSet> dataSet_new) {
  vtkSmartPointer<vtkAppendFilter> appendFilter =
      vtkSmartPointer<vtkAppendFilter>::New();
  appendFilter->AddInputData(dataSet);
  appendFilter->AddInputData(dataSet_new);
  appendFilter->MergePointsOn();
  appendFilter->Update();
  dataSet = appendFilter->GetOutput();

  numCells = dataSet->GetNumberOfCells();
  numPoints = dataSet->GetNumberOfPoints();
}

/*
void addLegacyVTKData(vtkDataArray *arr, const std::string &type,
                      const bool pointOrCell,
                      vtkSmartPointer<vtkUnstructuredGrid> dataSet_tmp) {
  // casting to appropriate type array
  if (!type.compare("unsigned_int")) {
    vtkUnsignedIntArray *typedArr = vtkUnsignedIntArray::SafeDownCast(arr);
    pointOrCell ? dataSet_tmp->GetPointData()->AddArray(typedArr)
                : dataSet_tmp->GetCellData()->AddArray(typedArr);
  } else if (!type.compare("int")) {
    std::cout << "in type arr" << std::endl;
    vtkIntArray *typedArr = vtkIntArray::SafeDownCast(arr);
    pointOrCell ? dataSet_tmp->GetPointData()->AddArray(typedArr)
                : dataSet_tmp->GetCellData()->AddArray(typedArr);
  } else if (!type.compare("unsigned_long")) {
    vtkUnsignedLongArray *typedArr = vtkUnsignedLongArray::SafeDownCast(arr);
    pointOrCell ? dataSet_tmp->GetPointData()->AddArray(typedArr)
                : dataSet_tmp->GetCellData()->AddArray(typedArr);
  } else if (!type.compare("long")) {
    vtkLongArray *typedArr = vtkLongArray::SafeDownCast(arr);
    pointOrCell ? dataSet_tmp->GetPointData()->AddArray(typedArr)
                : dataSet_tmp->GetCellData()->AddArray(typedArr);
  } else if (!type.compare("float")) {
    vtkFloatArray *typedArr = vtkFloatArray::SafeDownCast(arr);
    pointOrCell ? dataSet_tmp->GetPointData()->AddArray(typedArr)
                : dataSet_tmp->GetCellData()->AddArray(typedArr);
  } else if (!type.compare("double")) {
    vtkDoubleArray *typedArr = vtkDoubleArray::SafeDownCast(arr);
    pointOrCell ? dataSet_tmp->GetPointData()->AddArray(typedArr)
                : dataSet_tmp->GetCellData()->AddArray(typedArr);
  }
}
*/
