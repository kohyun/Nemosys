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
#ifndef NEMOSYS_NEMDRIVER_H_
#define NEMOSYS_NEMDRIVER_H_

#include "nemosys_export.h"

#include <jsoncons/json.hpp>
#include <memory>
#include <string>

namespace NEM {
namespace DRV {

// TODO: need to harden json input checks etc.
/**
 * @class NemDriver
 * @brief base class for drivers
 */
class NEMOSYS_EXPORT NemDriver {
 public:
  virtual ~NemDriver() = default;

  /**
   * Factory method for all drivers.
   * @param inputjson json input with "Program Type" key
   * @return pointer to base NemDriver class
   */
  static std::unique_ptr<NemDriver> readJSON(const jsoncons::json &inputjson);
  /**
   * @brief Run the workflow represented by the driver.
   */
  virtual void execute() const = 0;

  JSONCONS_TYPE_TRAITS_FRIEND

 protected:
  NemDriver() = default;
  NemDriver(const NemDriver&) = default;
  NemDriver(NemDriver &&) = default;
  NemDriver &operator=(const NemDriver &) = default;
  NemDriver &operator=(NemDriver &&) = default;

 private:
  virtual jsoncons::string_view getProgramType() const = 0;
};

struct NEMOSYS_EXPORT DriverOutFile {
 public:
  explicit DriverOutFile(std::string output);

  std::string outputFile{};

  JSONCONS_TYPE_TRAITS_FRIEND

 private:
  DriverOutFile() = default;
};

struct NEMOSYS_EXPORT DriverInOutFiles {
 public:
  DriverInOutFiles(std::string input, std::string output);

  std::string inputMeshFile{};
  std::string outputMeshFile{};

  JSONCONS_TYPE_TRAITS_FRIEND

 private:
  DriverInOutFiles() = default;
};

}  // namespace DRV
}  // namespace NEM

#endif  // NEMOSYS_NEMDRIVER_H_
