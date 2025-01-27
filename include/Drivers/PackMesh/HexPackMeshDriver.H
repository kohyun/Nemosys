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
#ifndef NEMOSYS_HEXPACKMESHDRIVER_H_
#define NEMOSYS_HEXPACKMESHDRIVER_H_

#include "nemosys_export.h"
#include "Drivers/PackMesh/PackMeshDriver.H"

#include "MeshManipulationFoam/MeshManipulationFoamParams.H"
#include "MeshGeneration/blockMeshParams.H"
#include "MeshGeneration/snappymeshParams.H"

namespace NEM {
namespace DRV {

/**
 * @details Hexahedral meshing pipeline creates different objects for different
meshing engines in NEMoSys, as well as objects of mesh manipulation utilities to
  complete the meshing workflow.
  ## Pack Meshing Workflow for Hexahedral Mesh
  > * Geometry generation using RocPack output file.
  > * Generation of STL file for workflow downstream.
  > * Generation of background mesh for snappyHexMesh
  > * Snapping of pack surfaces onto background volume mesh to create conformal
interfaces.
  > * Splitting multiple pack regions into different cellZones (volumes).
  > * Merging all different volumes into two different foam mesh (1.
Packs, 2. Surrounding)
  > * Creating patch for Packs and Surrounding regions.
  > * Conversion of foam mesh to VTK.
  > * Provides mesh quality statistics to users in a text file.
 */
class NEMOSYS_EXPORT HexPackMeshDriver : public PackMeshDriver {
 public:
  struct NEMOSYS_EXPORT Files {
   public:
    /**
     * @brief
     * @param inputFile Input surface file name
     * @param isRocpack Boolean for using rocpack to generate an STL
     * @param outPackMeshFile Pack mesh output filename.extension
     * @param outSurroundingFile Surrounding mesh output filename.extension
     */
    Files(std::string inputFile, bool isRocpack, std::string outPackMeshFile,
          std::string outSurroundingFile);
    void setRocpackFile(std::string rocpackFile);
    void setGeoFile(std::string geoFile);
    const std::string &getInputFile() const;
    bool isInputRocpackFile() const;
    std::string outPackMeshFile{};
    std::string outSurroundingFile{};
    /**
     * @brief Combined mesh file name
     */
    std::string outCombinedFile{"PackMesh.vtu"};
    JSONCONS_TYPE_TRAITS_FRIEND
   private:
    Files() = default;
    std::string rocpackOrGeoFile{};
    bool useRocpack{};
  };

  struct NEMOSYS_EXPORT Opts {
   public:
    snappymeshParams smParams{};
    blockMeshParams bmParams{};
    MeshManipulationFoamParams::MergeMeshes mmfMergeParams{};
    MeshManipulationFoamParams::CreatePatch mmfCreatePatchParams{};
    /**
     * @brief Perturb location in mesh if snappy fails
     * @details Ignored if negative
     */
    double locAdjust{0.};
    JSONCONS_TYPE_TRAITS_FRIEND
   private:
    static constexpr const char *type = "hexahedral";
    static constexpr const char *engine = "packmesh";
  };

  HexPackMeshDriver(Files files, Opts opts);

  const Files &getFiles() const;
  void setFiles(Files files);
  const Opts &getOpts() const;
  void setOpts(Opts opts);
  void execute() const override;

  JSONCONS_TYPE_TRAITS_FRIEND

 private:
  HexPackMeshDriver();

  Files files_;
  Opts opts_;
};

}  // namespace DRV
}  // namespace NEM

#endif  // NEMOSYS_HEXPACKMESHDRIVER_H_
