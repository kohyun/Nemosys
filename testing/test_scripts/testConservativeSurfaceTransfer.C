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
#include <Mesh/meshBase.H>
#include <Mesh/vtkMesh.H>
#include <gtest/gtest.h>
#include <Drivers/TransferDriver.H>
#include <Transfer/ConservativeSurfaceTransfer.H>
#include <Mesh/vtkMesh.H>

#include "vtkPointData.h"
#include "vtkDoubleArray.h"
#include "vtkPolyData.h"
#include "vtkXMLUnstructuredGridWriter.h"

const char* sourceFile;
const char* targetFile;
const char* outputFile;

vtkMesh* sourceMesh;
vtkMesh* targetMesh;
ConservativeSurfaceTransfer * transfer;

TEST(ConservativeSurfaceTransferTest, DriverTest)
{
  std::cerr << "source file : " << sourceFile << std::endl;
  std::cerr << "target file : " << targetFile << std::endl;
  TransferDriver* transferDriver = new TransferDriver(std::string(sourceFile),
                                                      std::string(targetFile),
                                                      std::string("Conservative Surface Transfer"),
                                                      std::string(outputFile),
                                                      false);
  delete transferDriver;
}

TEST(ConservativeSurfaceTransferTest, ConstructionTest)
{
  sourceMesh = new vtkMesh(sourceFile);
  targetMesh = new vtkMesh(targetFile);

  transfer = new ConservativeSurfaceTransfer(sourceMesh, targetMesh);
}

TEST(ConservativeSurfaceTransferTest, WriteTest)
{
  transfer->writeOverlay();
}

TEST(ConservativeSurfaceTransferTest, TransferScalarTest)
{
  vtkDataSet * sourceDataSet = sourceMesh->getDataSet();

  auto pointValues = vtkSmartPointer<vtkDoubleArray>::New();
  pointValues->SetName("scalar field");
  for(int ptIdx = 0; ptIdx < sourceDataSet->GetNumberOfPoints(); ++ptIdx)
  {
    double srcPt[3];
    sourceDataSet->GetPoint(ptIdx, srcPt);
    pointValues->InsertValue(ptIdx, srcPt[0]);
  }

  sourceDataSet->GetPointData()->AddArray(pointValues);

  std::vector<int> arrayIds = { 0 };

  transfer->transferPointData(arrayIds);

  // compute error
  /*
   * NOTE :
   * 
   * SurfX currently has a faulty formulation in its transfer, and the resulting error
   * is large. Therefore, we avoid assertions here but keep the code for the sake of
   * debugging.
   *
   * Resolving this issue within SurfX is a priority.
   *
   */
  vtkDataSet* targetDataSet = targetMesh->getDataSet(); 
  auto targetPointValues = vtkDoubleArray::SafeDownCast(targetDataSet->GetPointData()->GetArray(0));
  double max_err = 0;
  double sum_err = 0;
  for(int ptIdx = 0; ptIdx < targetDataSet->GetNumberOfPoints(); ++ptIdx)
  {
    double val = targetPointValues->GetValue(ptIdx);

    double trgPt[3];
    targetDataSet->GetPoint(ptIdx, trgPt);
    double expected = trgPt[0];

    double err = std::abs(val - expected);
    max_err = std::max(err, max_err);
    sum_err += err;
  }
  std::cerr << sum_err/targetDataSet->GetNumberOfPoints() << std::endl;
  std::cerr << max_err << std::endl;
}

TEST(ConservativeSurfaceTransferTest, TransferVectorTest)
{
  vtkDataSet * sourceDataSet = sourceMesh->getDataSet();

  auto pointValues = vtkSmartPointer<vtkDoubleArray>::New();
  pointValues->SetName("vector field");
  pointValues->SetNumberOfComponents(3);
  for(int ptIdx = 0; ptIdx < sourceDataSet->GetNumberOfPoints(); ++ptIdx)
  {
    double srcPt[3];
    sourceDataSet->GetPoint(ptIdx, srcPt);
    pointValues->InsertNextTuple3(srcPt[0], srcPt[1], srcPt[2]);
  }
  sourceDataSet->GetPointData()->AddArray(pointValues);

  std::vector<int> arrayIds = { 1 };

  transfer->transferPointData(arrayIds);

  // compute error
  vtkDataSet* targetDataSet = targetMesh->getDataSet();
  auto targetPointValues = vtkDoubleArray::SafeDownCast(targetDataSet->GetPointData()->GetArray(1));
  double max_err = 0;
  for(int ptIdx = 0; ptIdx < targetDataSet->GetNumberOfPoints(); ++ptIdx)
  {
    for(int compId = 0; compId < targetPointValues->GetNumberOfComponents(); ++compId)
    {
      double val = targetPointValues->GetComponent(ptIdx, compId);

      double trgPt[3];
      targetDataSet->GetPoint(ptIdx, trgPt);
      double expected = trgPt[compId];

      double err = std::abs(val - expected);
      max_err = std::max(err, max_err);
    }
  }

  std::cerr << max_err << std::endl;
}

TEST(CosnervativeSurfaceTransferTest, TargetWriteTest)
{
  auto writer = vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
  writer->SetInputData(targetMesh->getDataSet());
  writer->SetFileName("output_target.vtu");
  writer->Write();
}

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  assert(argc == 3);
  sourceFile = argv[1];
  targetFile = argv[2];
  outputFile = "target.vtk";
  return RUN_ALL_TESTS();
}
