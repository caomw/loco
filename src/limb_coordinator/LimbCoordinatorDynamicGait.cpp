/*!
* @file     LimbCoordinatorDynamicGait.cpp
* @author   Christian Gehring
* @date     Feb, 2014
* @version  1.0
* @ingroup
* @brief
*/
#include "loco/limb_coordinator/LimbCoordinatorDynamicGait.hpp"

namespace loco {

LimbCoordinatorDynamicGait::LimbCoordinatorDynamicGait(LegGroup* legs, TorsoBase* torso, GaitPatternBase* gaitPattern, bool isUpdatingStridePhase) :
    LimbCoordinatorBase(),
    isUpdatingStridePhase_(isUpdatingStridePhase),
    legs_(legs),
    torso_(torso),
    gaitPattern_(gaitPattern),
    isLegGrounded_{false, false, false, false}
{


}

LimbCoordinatorDynamicGait::~LimbCoordinatorDynamicGait() {

}

void LimbCoordinatorDynamicGait::setIsUpdatingStridePhase(bool isUpdatingStridePhase) {
  isUpdatingStridePhase_ = isUpdatingStridePhase;
}
bool LimbCoordinatorDynamicGait::isUpdatingStridePhase() const {
  return isUpdatingStridePhase_;
}

bool LimbCoordinatorDynamicGait::isLegGrounded(int iLeg) {
    return isLegGrounded_[iLeg];
}

bool LimbCoordinatorDynamicGait::shouldBeLegGrounded(int iLeg) {
  return shouldBeLegGrounded_[iLeg];
}
bool LimbCoordinatorDynamicGait::isAndShouldBeLegGrounded(int iLeg) {
  return (isLegGrounded_[iLeg] && shouldBeLegGrounded_[iLeg]);
}


void LimbCoordinatorDynamicGait::setIsLegGrounded(int iLeg, bool isLegGrounded)
{
  isLegGrounded_[iLeg] = isLegGrounded;
}

void LimbCoordinatorDynamicGait::setShouldBeLegGrounded(int iLeg, bool shouldBeLegGrounded)
{
  shouldBeLegGrounded_[iLeg] = shouldBeLegGrounded;
}



bool LimbCoordinatorDynamicGait::initialize(double dt) {
  if(!gaitPattern_->initialize(dt)) {
    return false;
  }

  if(!advance(0.0)) {
    return false;
  }
  return true;
}

bool LimbCoordinatorDynamicGait::advance(double dt) {
  if (isUpdatingStridePhase_) {
    gaitPattern_->advance(dt);
  }
//  for (int iLeg=0; iLeg<4; iLeg++) {
  int iLeg =0;
  for (auto leg : *legs_) {
    shouldBeLegGrounded_[iLeg] = gaitPattern_->shouldBeLegGrounded(iLeg);
    leg->setShouldBeGrounded(shouldBeLegGrounded_[iLeg]);
    leg->setStanceDuration(gaitPattern_->getStanceDuration(iLeg));
    leg->setSwingDuration(gaitPattern_->getStrideDuration()-gaitPattern_->getStanceDuration(iLeg));
    leg->setWasInStanceMode(leg->isInStanceMode());
    leg->setIsInStanceMode(isLegInStanceMode(iLeg));
    leg->setWasInSwingMode(leg->isInSwingMode());
    leg->setIsInSwingMode(isLegInSwingMode(iLeg));
    leg->setSwingPhase(gaitPattern_->getSwingPhaseForLeg(iLeg));
    leg->setStancePhase(gaitPattern_->getStancePhaseForLeg(iLeg));
    iLeg++;
  }

  torso_->setStridePhase(gaitPattern_->getStridePhase());
  return true;
}

void LimbCoordinatorDynamicGait::setStridePhase(double stridePhase)  {
  gaitPattern_->setStridePhase(stridePhase);
}

bool LimbCoordinatorDynamicGait::isLegInStanceMode(int iLeg) {
  const double swingPhase = gaitPattern_->getSwingPhaseForLeg(iLeg);
  if (swingPhase > 0.9 && this->isLegGrounded(iLeg)) return true; // early touch-down -> switch to stance mode
  return (swingPhase < 0 || swingPhase > 1); // stance mode
}


bool LimbCoordinatorDynamicGait::isLegInSwingMode(int iLeg) {
  const double swingPhase = gaitPattern_->getSwingPhaseForLeg(iLeg);
  if (swingPhase > 0.9 && this->isLegGrounded(iLeg)) return false; // early touch-down -> switch to stance mode
  return (swingPhase >= 0 && swingPhase <= 1);
}

GaitPatternBase* LimbCoordinatorDynamicGait::getGaitPattern() {
  return gaitPattern_;
}

const GaitPatternBase& LimbCoordinatorDynamicGait::getGaitPattern() const {
  return *gaitPattern_;
}

bool LimbCoordinatorDynamicGait::loadParameters(const TiXmlHandle& handle)
{
  if (!gaitPattern_->loadParameters(TiXmlHandle(handle.FirstChild("LimbCoordination")))) {
    return false;
  }
  return true;
}

bool LimbCoordinatorDynamicGait::setToInterpolated(const LimbCoordinatorBase& limbCoordinator1, const LimbCoordinatorBase& limbCoordinator2, double t) {
  const LimbCoordinatorDynamicGait& coordinator1 = static_cast<const LimbCoordinatorDynamicGait&>(limbCoordinator1);
  const LimbCoordinatorDynamicGait& coordinator2 = static_cast<const LimbCoordinatorDynamicGait&>(limbCoordinator2);
  if (!gaitPattern_->setToInterpolated(coordinator1.getGaitPattern(), coordinator2.getGaitPattern(), t)) {
    return false;
  }
  return true;
}

} /* namespace loco */
