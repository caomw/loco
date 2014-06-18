/*!
* @file     BaseControlDynamicGait.hpp
* @author   Christian Gehring
* @date     Feb, 2014
* @version  1.0
* @ingroup
* @brief
*/
#ifndef LOCO_TorsoControlDynamicGaitPerceptedTerrain_HPP_
#define LOCO_TorsoControlDynamicGaitPerceptedTerrain_HPP_

#include "loco/torso_control/TorsoControlBase.hpp"
#include "loco/torso_control/CoMOverSupportPolygonControl.hpp"

#include "loco/common/LegGroup.hpp"
#include "loco/common/TorsoBase.hpp"

#include "kindr/rotations/RotationEigen.hpp"

#include "loco/common/TerrainModelBase.hpp"

#include "PeriodicRBF1DC1.hpp"

namespace loco {

class TorsoControlDynamicGaitPerceptedTerrain: public TorsoControlBase {
 public:
  TorsoControlDynamicGaitPerceptedTerrain(LegGroup* legs, TorsoBase* torso, loco::TerrainModelBase* terrain);
  virtual ~TorsoControlDynamicGaitPerceptedTerrain();

  virtual bool initialize(double dt);
  virtual void advance(double dt);

  virtual RotationQuaternion computeHeading(const RotationQuaternion& rquat, const Vector& axis);
  RotationQuaternion decomposeRotation(const RotationQuaternion& q_BA, const Vector& vB);
  CoMOverSupportPolygonControl* getCoMControl();
  virtual bool loadParameters(const TiXmlHandle& handle);

 private:
  LegGroup* legs_;
  TorsoBase* torso_;
  loco::TerrainModelBase* terrain_;
  CoMOverSupportPolygonControl comControl_;

  double terrainSlopeAngle_;
  double terrainRollAngle_;

  double headingDistanceFromForeToHindInBaseFrame_;
  double desiredTorsoForeHeightAboveGroundInWorldFrameOffset_;
  double desiredTorsoHindHeightAboveGroundInWorldFrameOffset_;
  rbf::PeriodicRBF1DC1 desiredTorsoForeHeightAboveGroundInWorldFrame_;
  rbf::PeriodicRBF1DC1 desiredTorsoHindHeightAboveGroundInWorldFrame_;
  virtual bool loadParametersHipConfiguration(const TiXmlHandle &hParameterSet);
  virtual bool loadHeightTrajectory(const TiXmlHandle &hTrajectory,  rbf::PeriodicRBF1DC1& trajectory);



};

} /* namespace loco */

#endif /* LOCO_BASECONTROLDYNAMICGAIT_HPP_ */