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
#ifndef NEMOSYS_VTKTOPATRANCONVERSIONDRIVER_H_
#define NEMOSYS_VTKTOPATRANCONVERSIONDRIVER_H_

#include "nemosys_export.h"
#include "Drivers/Conversion/ConversionDriver.H"

namespace NEM {
namespace DRV {

class NEMOSYS_EXPORT VtkToPatranConversionDriver : public ConversionDriver {
 public:
  using Files = DriverInOutFiles;

  /**
   * @brief PATRAN specific BC information
   */
  struct NEMOSYS_EXPORT BoundaryCond {
   public:
    explicit BoundaryCond(int patchNum);
    virtual ~BoundaryCond() = default;
    /**
     * @brief patch numbers as specified in RocfracControl.txt
     */
    int patchNum{};
    JSONCONS_TYPE_TRAITS_FRIEND
   protected:
    BoundaryCond() = default;
    BoundaryCond(const BoundaryCond &) = default;
    BoundaryCond(BoundaryCond &&) = default;
    BoundaryCond &operator=(const BoundaryCond &) = default;
    BoundaryCond &operator=(BoundaryCond &&) = default;
   private:
    virtual jsoncons::string_view getBCType() const = 0;
  };

  struct NEMOSYS_EXPORT FaceBC : public BoundaryCond {
   public:
    FaceBC(int patchNum, int rocFracFSIType);
    /**
     * @brief Rocfrac FSI Type; 0 = no FSI, 1 = FSI w/ burn, 2 = FSI w/o burn,
     * etc. see Rocfrac manual for details
     */
    int rocFracFSIType{};
    JSONCONS_TYPE_TRAITS_FRIEND
   private:
    FaceBC() = default;
    static constexpr const char *bcType = "Face";
    jsoncons::string_view getBCType() const override;
  };

  struct NEMOSYS_EXPORT NodeBC : public BoundaryCond {
   public:
    NodeBC(int patchNum, int rocfracControlType, bool structural,
           bool meshMotion, bool thermal);
    int rocfracControlType{};
    /**
     * @brief boolean indicating structural BC
     */
    bool structural{};
    /**
     * @brief boolean indicating mesh motion BC
     */
    bool meshMotion{};
    /**
     * @brief boolean indicating heat transfer BC
     */
    bool thermal{};
    JSONCONS_TYPE_TRAITS_FRIEND
   private:
    NodeBC() = default;
    static constexpr const char *bcType = "Node";
    jsoncons::string_view getBCType() const override;
  };

  struct NEMOSYS_EXPORT Opts {
   public:
    Opts(std::vector<std::shared_ptr<BoundaryCond>> bcInfo,
         std::vector<int> nodePatchPreference);
    std::vector<std::shared_ptr<BoundaryCond>> bcInfo{};
    /**
     * @brief // Accumulate node patch preference (determines which patch a node
     * belongs to if it borders two or more patches)
     */
    std::vector<int> nodePatchPreference{};
    JSONCONS_TYPE_TRAITS_FRIEND
   private:
    Opts() = default;
    static constexpr const char *methodName = "VTK->PATRAN";
  };

  VtkToPatranConversionDriver(Files files, Opts opts);

  const Files &getFiles() const;
  void setFiles(Files files);
  const Opts &getOpts() const;
  void setOpts(Opts opts);
  void execute() const override;

  JSONCONS_TYPE_TRAITS_FRIEND

 private:
  VtkToPatranConversionDriver();

  Files files_;
  Opts opts_;
};

}  // namespace DRV
}  // namespace NEM

#endif  // NEMOSYS_VTKTOPATRANCONVERSIONDRIVER_H_
