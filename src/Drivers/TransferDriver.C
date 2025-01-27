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

#include <iostream>
#include <string>
#include <utility>

#include "Transfer/FETransfer.H"
#ifdef HAVE_IMPACT
#  include "Transfer/ConservativeSurfaceTransfer.H"
#endif
#ifdef HAVE_SUPERMESH
#  include "Transfer/ConservativeVolumeTransfer.H"
#endif
#include "AuxiliaryFunctions.H"

namespace NEM {
namespace DRV {

TransferDriver::Files::Files(std::string source, std::string target,
                             std::string output)
    : sourceMeshFile(std::move(source)),
      targetMeshFile(std::move(target)),
      outputMeshFile(std::move(output)) {}

TransferDriver::Opts::Opts(std::string method, bool checkQuality)
    : method(std::move(method)), checkQuality(checkQuality) {}

TransferDriver::TransferDriver(Files files, Opts opts)
    : files_(std::move(files)), opts_(std::move(opts)) {
  std::cout << "TransferDriver created" << std::endl;
}

TransferDriver::TransferDriver() : TransferDriver({{}, {}, {}}, {{}, {}}) {}

void TransferDriver::execute() const {
  std::shared_ptr<meshBase> source{
      meshBase::Create(this->files_.sourceMeshFile)};
  std::shared_ptr<meshBase> target{
      meshBase::Create(this->files_.targetMeshFile)};
  std::cout << "TransferDriver created" << std::endl;

  nemAux::Timer T;
  T.start();
  source->setCheckQuality(this->opts_.checkQuality);
  // source->transfer(target, method, arrayNames);
  auto transfer = TransferDriver::CreateTransferObject(
      source.get(), target.get(), this->opts_.method);
  if (this->opts_.arrayNames) {
    transfer->transferPointData(
        source->getArrayIDs(this->opts_.arrayNames.value()),
        source->getNewArrayNames());
  } else {
    transfer->run(source->getNewArrayNames());
  }
  source->write("new.vtu");
  T.stop();

  std::cout << "Time spent transferring data (ms) " << T.elapsed() << std::endl;

  target->write(this->files_.outputMeshFile);
}

const TransferDriver::Files &TransferDriver::getFiles() const { return files_; }

void TransferDriver::setFiles(Files files) { this->files_ = std::move(files); }

const TransferDriver::Opts &TransferDriver::getOpts() const { return opts_; }

void TransferDriver::setOpts(Opts opts) { this->opts_ = std::move(opts); }

TransferDriver::~TransferDriver() {
  std::cout << "TransferDriver destroyed" << std::endl;
}

jsoncons::string_view TransferDriver::getProgramType() const {
  return programType;
}

std::shared_ptr<TransferBase> TransferDriver::CreateTransferObject(
    meshBase *srcmsh, meshBase *trgmsh, const std::string &method) {
  if (method == "Consistent Interpolation") {
    return FETransfer::CreateShared(srcmsh, trgmsh);
  }
#ifdef HAVE_IMPACT
  else if (method == "Conservative Surface Transfer") {
    return ConservativeSurfaceTransfer::CreateShared(srcmsh, trgmsh);
  }
#endif
#ifdef HAVE_SUPERMESH
  else if (method == "Conservative Volume Transfer") {
    return ConservativeVolumeTransfer::CreateShared(srcmsh, trgmsh);
  }
#endif
  else {
    std::cerr << "Method " << method << " is not supported." << std::endl;
    std::cerr << "Supported methods are : " << std::endl;
    std::cerr << "1) Consistent Interpolation" << std::endl;
#ifdef HAVE_IMPACT
    std::cerr << "2) Conservative Surface Transfer" << std::endl;
#endif
#ifdef HAVE_SUPERMESH
    std::cerr << "3) Conservative Volume Transfer" << std::endl;
#endif
    exit(1);
  }
}

}  // namespace DRV
}  // namespace NEM
