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
#ifndef NEMOSYS_OMEGAHREFINEDRIVER_H_
#define NEMOSYS_OMEGAHREFINEDRIVER_H_

#include "Drivers/Refine/RefineDriver.H"

#include <Omega_h_defines.hpp>
#include "Mesh/geoMeshBase.H"
#include "Services/omegahRefineSrv.H"

namespace NEM {
namespace DRV {

class NEMOSYS_EXPORT OmegahRefineDriver : public RefineDriver {
 public:
  struct NEMOSYS_EXPORT Transfer {
   public:
    Transfer(std::string arrayName, Omega_h_Transfer method);
    std::string arrayName{};
    Omega_h_Transfer method{};
    jsoncons::optional<std::string> integralName{};
    JSONCONS_TYPE_TRAITS_FRIEND
   private:
    Transfer() = default;
  };
  struct NEMOSYS_EXPORT VarCompare : public NEM::SRV::omegahRefineVarCompareOpts {
    explicit VarCompare(std::string integralName);
    std::string integralName;
    JSONCONS_TYPE_TRAITS_FRIEND
   private:
    VarCompare() = default;
  };
  struct NEMOSYS_EXPORT Opts {
   public:
    explicit Opts(std::vector<NEM::SRV::omegahRefineMetricSource> sources);
    // geoMeshBase
    bool reconstructGeo{false};
    // Metric Opts
    jsoncons::optional<bool> Verbose{};
    std::vector<NEM::SRV::omegahRefineMetricSource> MetricSources{};
    jsoncons::optional<bool> ShouldLimitLengths{};
    jsoncons::optional<Omega_h::Real> MaxLength{};
    jsoncons::optional<Omega_h::Real> MinLength{};
    jsoncons::optional<bool> ShouldLimitGradation{};
    jsoncons::optional<Omega_h::Real> MaxGradationRate{};
    jsoncons::optional<Omega_h::Real> GradationConvergenceTolerance{};
    jsoncons::optional<bool> ShouldLimitElementCount{};
    jsoncons::optional<Omega_h::Real> MaxElementCount{};
    jsoncons::optional<Omega_h::Real> MinElementCount{};
    jsoncons::optional<Omega_h::Real> ElementCountOverRelaxation{};
    jsoncons::optional<Omega_h::Int> NsmoothingSteps{};
    // Adapt Opts
    jsoncons::optional<Omega_h::Real> MinLengthDesired{};
    jsoncons::optional<Omega_h::Real> MaxLengthDesired{};
    jsoncons::optional<Omega_h::Real> MaxLengthAllowed{};
    jsoncons::optional<Omega_h::Real> MinQualityAllowed{};
    jsoncons::optional<Omega_h::Real> MinQualityDesired{};
    jsoncons::optional<Omega_h::Int> NsliverLayers{};
    jsoncons::optional<std::string> Verbosity{};
    jsoncons::optional<Omega_h::Real> LengthHistogramMin{};
    jsoncons::optional<Omega_h::Real> LengthHistogramMax{};
    jsoncons::optional<Omega_h::Int> NlengthHistogramBins{};
    jsoncons::optional<Omega_h::Int> NqualityHistogramBins{};
    jsoncons::optional<bool> ShouldRefine{};
    jsoncons::optional<bool> ShouldCoarsen{};
    jsoncons::optional<bool> ShouldSwap{};
    jsoncons::optional<bool> ShouldCoarsenSlivers{};
    jsoncons::optional<bool> ShouldPreventCoarsenFlip{};
    // Transfer Opts
    std::vector<Transfer> TransferOpts;
    std::vector<VarCompare> TransferOptsIntegralDiffuse;
    JSONCONS_TYPE_TRAITS_FRIEND
   private:
    Opts() = default;
    static constexpr const char *method = "Omega_h";
  };
  OmegahRefineDriver(Files files, Opts opts);

  const Opts &getOpts() const;
  void setOpts(Opts opts);
  void execute() const override;

  JSONCONS_TYPE_TRAITS_FRIEND

 private:
  OmegahRefineDriver();

  Opts opts_;
};

}  // namespace DRV
}  // namespace NEM

#endif  // NEMOSYS_OMEGAHREFINEDRIVER_H_
