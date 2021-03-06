/*****************************************************************************************
* Software License Agreement (BSD License)
*
* Copyright (c) 2014, Christian Gehring, Péter Fankhauser, C. Dario Bellicoso, Stelian Coros
* All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*   * Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above
*     copyright notice, this list of conditions and the following
*     disclaimer in the documentation and/or other materials provided
*     with the distribution.
*   * Neither the name of Autonomous Systems Lab nor ETH Zurich
*     nor the names of its contributors may be used to endorse or
*     promote products derived from this software without specific
*     prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
*  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
*  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
*  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
*  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
*  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
*  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
*  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*/
/*
 * GaitSwitcherDynamicGaitDefault.cpp
 *
 *  Created on: Aug 22, 2014
 *      Author: gech
 */

#include "loco/gait_switcher/GaitSwitcherDynamicGaitDefault.hpp"
#include "loco/temp_helpers/math.hpp"

namespace loco {

GaitSwitcherDynamicGaitDefault::GaitSwitcherDynamicGaitDefault(robotModel::RobotModel* robotModel,
                                                               robotTerrain::TerrainBase* terrain,
                                                               double dt) :
    robotModel_(robotModel),
    terrain_(terrain),
    time_step_(dt),
    isRealRobot_(false),
    isTransiting_(false),
    initTransit_(false),
    startTimeOfTransition_(0.0),
    endTimeOfTransition_(0.0),
    currentGaitTransition(nullptr),
    pathToConfigFile_(),
    pathToParameterFiles_(),
    isAutoTransitionOn_(false),
    time_(0.0),
    interpolationParameter_(0.0)
{

}



GaitSwitcherDynamicGaitDefault::~GaitSwitcherDynamicGaitDefault() {
  gaitTransitions_.clear();
  locomotionControllers_.clear();
}


bool GaitSwitcherDynamicGaitDefault::initialize(double dt) {
  time_ = 0.0;
  interpolationParameter_ = 0.0;

  if (!config_.loadXmlDocument(pathToConfigFile_)) {
     std::cout << "Could not load config file: " << pathToConfigFile_  << std::endl;
     return false;
   }

  if(!loadParameterSet(0)) {
    printf("Error: could not load initial parameter set!\n");
    return false;
  }
  return true;

}

bool GaitSwitcherDynamicGaitDefault::advance(double dt) {

  bool isSuccessful = updateTransition(time_);

  if (!locomotionController_->advanceMeasurements(dt)) {
    return false;
  }

  if (!locomotionController_->advanceSetPoints(dt)) {
    return false;
  }

  time_ += dt;
  return isSuccessful;
}


void GaitSwitcherDynamicGaitDefault::setIsRealRobot(bool isRealRobot) {
  isRealRobot_ = isRealRobot;
}


bool GaitSwitcherDynamicGaitDefault::updateTransition(double simulatedTime) {
  if (isAutoTransitionOn_ && !isTransiting_) {
      /* check automatic transition depending on speed */
      const double currentSagittalVelocity = fabs(0.0); // TODO read current speed instead of setting to 0

      const std::string currentGaitName = locomotionController_->getGaitName();
      for (unsigned int i=0;i<gaitTransitions_.size();i++){
        if (gaitTransitions_[i].startName == currentGaitName) {
          bool isGood = false;
          if (currentSagittalVelocity > gaitTransitions_[i].largerSpeedTrigger && gaitTransitions_[i].largerSpeedTrigger >= 0) {
            isGood = true;
          } else if (currentSagittalVelocity < gaitTransitions_[i].smallerSpeedTrigger) {
            isGood = true;
          }
          if (isGood) {
            /* start transition */
            isTransiting_ = true;
            initTransit_ = true;
            locomotionController_->setGaitName(gaitTransitions_[i].endName);
            currentGaitTransition = &gaitTransitions_[i];
            break;
          }
        }
      }

  }


  if (!isTransiting_) {
    /* is not transiting -> terminate */
    return true;
  }


  if (initTransit_) {
      /* intialize the transition */
      if (currentGaitTransition->timeInterval <= 0) {
        /* immediately transit */
        if (currentGaitTransition->stridePhaseTrigger < 0 || currentGaitTransition->stridePhaseTrigger > 1 || (fabs(locomotionController_->getStridePhase()-currentGaitTransition->stridePhaseTrigger) < time_step_/locomotionController_->getStrideDuration())) {
          interpolateParameters(1);
          printf("Finished transition!\n");
          initTransit_ = false;
          isTransiting_ = false;
        }
        return true;
  //      printf("waiting for phase: %f\n",starlETHCon->getCurrentLocomotionSettings()->stridePhase);
      } else {
        /* transit over time interval */
        if (currentGaitTransition->stridePhaseTrigger < 0 || currentGaitTransition->stridePhaseTrigger > 1 || (fabs(locomotionController_->getStridePhase()-currentGaitTransition->stridePhaseTrigger) < time_step_/locomotionController_->getStrideDuration() )) {
          /* transition is now triggered by stride phase */
          printf("Started transition!\n");
          time_ = 0.0;
          initTransit_ = false;
        }
  //      printf("waiting for phase: %f\n",starlETHCon->getCurrentLocomotionSettings()->stridePhase);
        return true;
      }

    }



    if (time_ > currentGaitTransition->timeInterval) {
      /* finished the transition */
      printf("Finished transition!\n");
      isTransiting_ = false;
      return true;
    }


  /* interpolate between the parameter sets */
//  const double t = (currentGaitPattern->getNextAPS()->interpolate_-currentGaitPattern->getCurrentAPS()->interpolate_)*currentGaitPattern->getCurrentAPS()->phase_+currentGaitPattern->getCurrentAPS()->interpolate_;
  double t = time_/currentGaitTransition->timeInterval;
  if(!interpolateParameters(t)) {
    return false;
  }
  return true;
}

bool GaitSwitcherDynamicGaitDefault::interpolateParameters(double t) {
  boundToRange(&t, 0, 1);
  if(!locomotionController_->setToInterpolated(*currentGaitTransition->startLocomotionSettings, *currentGaitTransition->endLocomotionSettings, t)) {
    printf("GaitSwitcherDynamicGaitDefault: Error while interpolating!\n");
    return false;
  }
  return true;
}


bool GaitSwitcherDynamicGaitDefault::getLocomotionControllerByName(const std::string& name, LocomotionControllerDynamicGaitDefault* loco) {
  for (unsigned int i=0; i<locomotionControllers_.size();i++){
    if(locomotionControllers_[i].getGaitName() == name) {
      /* is found */

      loco = &locomotionControllers_[i];
      return true;
    }
  }
  return false;
}


void GaitSwitcherDynamicGaitDefault::setPathToConfigFile(const std::string& pathToConfigFile) {
  pathToConfigFile_ = pathToConfigFile;
}

void GaitSwitcherDynamicGaitDefault::setPathToParameterFiles(const std::string& pathToParameterFiles) {
  pathToParameterFiles_ = pathToParameterFiles;
}

void GaitSwitcherDynamicGaitDefault::setAutoTransition(bool isActive)
{
  isAutoTransitionOn_ = isActive;
}

bool GaitSwitcherDynamicGaitDefault::isTransiting()
{
  return isTransiting_;
}


bool GaitSwitcherDynamicGaitDefault::transitToGait(const std::string& targetGaitName) {
  if (isTransiting_) {
    printf("Is already transiting!\n");
    return false;
  }

  std::string currentGaitName = locomotionController_->getGaitName();
  for (unsigned int i=0;i<gaitTransitions_.size();i++){
    if (gaitTransitions_[i].startName == currentGaitName && gaitTransitions_[i].endName == targetGaitName) {
      /* found transition */
      isTransiting_ = true;
      initTransit_ = true;
      locomotionController_->setGaitName(targetGaitName);
      currentGaitTransition = &gaitTransitions_[i];
      printf("Init transition: %s -> %s\n",currentGaitTransition->startName.c_str(), currentGaitTransition->endName.c_str());
      return true;
    }

  }

  printf("Transition not found!\n");
  return false;
}


bool GaitSwitcherDynamicGaitDefault::loadParameterSet(int parameterSetIdx)
{

  if (isTransiting_) {
    printf("Cannot load parameter set because transition is active!\n");
    return false;
  }

  TiXmlHandle hTask(config_.getHandle().FirstChild("GaitSwitcherDynamicGaitDefault"));
  TiXmlElement* pElem;
  int idx = 0;
  int counter = 0;
  double interpolate = 0;
  std::string name;

  pElem = hTask.FirstChild("ParameterSets").Element();
  if (!pElem) {
    printf("Could not find ParameterSets!\n");
    return false;
  }

  if (parameterSetIdx == 0) {
    /* load initial index */
    if (pElem->QueryIntAttribute("initialIdx", &parameterSetIdx)!=TIXML_SUCCESS) {
      printf("Could not find ParameterSets:initialIdx!\n");
      return false;
    }
  }

  TiXmlHandle hParameterSets = hTask.FirstChild("ParameterSets");
  counter = 0;
  while (counter >= 0) {
    pElem = hParameterSets.Child(counter).Element();
    if(!pElem) {
      if (counter == 0) {
        printf("Could not find any parameter set!\n");
        return false;
      } else {
        /* end of list */
        if (parameterSetIdx == 0) {
          printf("Could not find initial parameter set!\n");
        } else {
          printf("Could not find parameter set with index=%d!\n", parameterSetIdx);
        }

        return false;
        break;
      }
    }
    if (pElem->QueryIntAttribute("idx", &idx)!=TIXML_SUCCESS) {
      printf("Could not find idx of a parameter set!\n");
      return false;
    }
    if (idx == parameterSetIdx) {
      /* found gait */



      /* check if parameter set should be interpolated */
      if (pElem->QueryDoubleAttribute("interpolate", &interpolate)==TIXML_SUCCESS) {
        /* yes */


        /* load initial parameter set */
        std::string init;
        if( pElem->QueryStringAttribute("init", &init) !=TIXML_SUCCESS) {
          printf("Could not find initial parameter set!\n");
          return false;
        }
        std::string parameterFileSuffix = isRealRobot_ ? "" : "Sim";
        std::string parameterFilePath = pathToParameterFiles_ +"/" + init + parameterFileSuffix + ".xml";

        locomotionController_.reset(new LocomotionControllerDynamicGaitDefault(parameterFilePath, robotModel_, terrain_, time_step_));
        locomotionController_->setGaitName(init);
        if(!locomotionController_->initialize(time_step_)) {
          printf("Could not load parameter sets for initial parameter set! (%s)\n",parameterFilePath.c_str());
          return false;
        }

        /* load the transitions */
        gaitTransitions_.clear();

        int isAllowed = 0;
        TiXmlElement* child = hParameterSets.Child(counter).FirstChild( "Transition" ).ToElement();
        for( child; child; child=child->NextSiblingElement() )
        {
          ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
          // Gait Transition
          ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
          isAllowed = 0;
          child->QueryIntAttribute("isAllowed", &isAllowed);
          if (isAllowed == 0) {
            // transition is not allowed, skip it
            continue;
          }

          gaitTransitions_.push_back(new GaitTransition());
          GaitTransition*  gaitTransition = &gaitTransitions_.back();

          // check start gait
          child->QueryStringAttribute("start", &gaitTransition->startName);
          if(!getLocomotionControllerByName(gaitTransition->startName, gaitTransition->startLocomotionSettings)) {
            // gait hasn't been been loaded yet
            std::string myParameterFilePath = pathToParameterFiles_ +"/" + gaitTransition->startName + parameterFileSuffix + ".xml";
            locomotionControllers_.push_back(new LocomotionControllerDynamicGaitDefault(myParameterFilePath, robotModel_, terrain_, time_step_));
            gaitTransition->startLocomotionSettings =  &locomotionControllers_.back();
            gaitTransition->startLocomotionSettings->setGaitName(gaitTransition->startName);
            if(!gaitTransition->startLocomotionSettings->initialize(time_step_)) {
              printf("Error: Could not initialize controller for gait %s\n", gaitTransition->startName.c_str());
              return false;
            }
          }
          else {
            // gait has already been loaded
            for (unsigned int i=0; i<locomotionControllers_.size();i++){
              if(locomotionControllers_[i].getGaitName() == gaitTransition->startName) {
                gaitTransition->startLocomotionSettings = &locomotionControllers_[i];
                break;
              }
            }
          }

          // check end gait
          child->QueryStringAttribute("end", &gaitTransition->endName);
          if(!getLocomotionControllerByName(gaitTransition->endName, gaitTransition->endLocomotionSettings)) {
            // gait hasn't been been loaded yet
            std::string myParameterFilePath = pathToParameterFiles_ +"/" + gaitTransition->endName + parameterFileSuffix + ".xml";
            locomotionControllers_.push_back(new LocomotionControllerDynamicGaitDefault(myParameterFilePath, robotModel_, terrain_, time_step_));
            gaitTransition->endLocomotionSettings = &locomotionControllers_.back();
            gaitTransition->endLocomotionSettings->setGaitName(gaitTransition->endName);

            if(!gaitTransition->endLocomotionSettings->initialize(time_step_)) {
              printf("Error: Could not initialize controller for gait %s\n", gaitTransition->endName.c_str());
              return false;
            }

          }
          else {
            // gait has already been loaded
              for (unsigned int i=0; i<locomotionControllers_.size();i++){
                if(locomotionControllers_[i].getGaitName() == gaitTransition->endName) {
                  gaitTransition->endLocomotionSettings = &locomotionControllers_[i];
                  break;
                }
              }
          }

          /* check speed triggers */
          double value = 0.0;
          value = -1.0;
          child->QueryDoubleAttribute("smallerSpeedTrigger", &value);
          gaitTransition->smallerSpeedTrigger = value;
          value = -1.0;
          child->QueryDoubleAttribute("largerSpeedTrigger", &value);
          gaitTransition->largerSpeedTrigger = value;
          value = 0.0;
          child->QueryDoubleAttribute("phaseTrigger", &value);
          gaitTransition->stridePhaseTrigger = value;
          value = 0.0;
          child->QueryDoubleAttribute("transitTime", &value);
          gaitTransition->timeInterval = value;

          /* load all APS */
//          TiXmlElement* pElem = child->ToElement();
////          if(!pElem) {
////            printf("Could not find APS!\n");
////            return false;
////          }
//
//          GaitPatternAPS* startGaitPattern = ((GaitPatternAPS*)gaitTransition->startLocomotionSettings->getGaitPattern());
//          GaitPatternAPS* endGaitPattern = ((GaitPatternAPS*)gaitTransition->endLocomotionSettings->getGaitPattern());
//
//          APS* startAPS = startGaitPattern->getLastAPS();
//          APS* endAPS = endGaitPattern->getFirstAPS();
//
//          gaitTransition->listAPS_.clear();
//
//          APS newAPS;
//          newAPS.interpolate_ = 0.0;
//
//
//          double cycleCounter = 0;
//
//          /* check transition duration */
//          double time = 0.0;
//          TiXmlElement* childCycle = child->FirstChild("Cycle")->ToElement();
//          time = startAPS->foreCycleDuration_;
//          for( childCycle; childCycle; childCycle=childCycle->NextSiblingElement() )
//          {
//            TiXmlElement* pElem = childCycle->FirstChild("APS")->ToElement();
//            if(pElem->QueryDoubleAttribute("foreCycleDuration", &value) != TIXML_SUCCESS) {
//              printf("Could not find foreCycleDuration!\n");
//              return false;
//            }
//
//            const double cycleDuration = interpolateAPSParameter(startAPS->foreCycleDuration_, endAPS->foreCycleDuration_, value);
////            printf("cycleDuration: %lf val=%lf start=%lf end=%lf\n", cycleDuration, value, startAPS->cycleDuration_, endAPS->cycleDuration_);
//            time += cycleDuration;
//
//          }
//          time += endAPS->foreCycleDuration_;
//          gaitTransition->timeInterval = time;
//          printf("duration of transition %s -> %s : %lfs\n",gaitTransition->startName.c_str(), gaitTransition->endName.c_str(), gaitTransition->timeInterval);
//
//          bool interpolateAPSParams = false;
//
//          childCycle = child->FirstChild("Cycle")->ToElement();
//          for( childCycle; childCycle; childCycle=childCycle->NextSiblingElement() )
//          {
//            TiXmlElement* pElem = childCycle->FirstChild("APS")->ToElement();
//
//            if(pElem->QueryDoubleAttribute("foreCycleDuration", &value) != TIXML_SUCCESS) {
//              printf("Could not find foreCycleDuration!\n");
//              return false;
//            }
//            if (interpolateAPSParams) {
//              newAPS.foreCycleDuration_ = interpolateAPSParameter(startAPS->foreCycleDuration_, endAPS->foreCycleDuration_, value);
//            } else {
//              newAPS.foreCycleDuration_ = value;
//            }
//
//            if(pElem->QueryDoubleAttribute("hindCycleDuration", &value) != TIXML_SUCCESS) {
//              printf("Could not find hindCycleDuration!\n");
//              return false;
//            }
//            if (interpolateAPSParams) {
//              newAPS.hindCycleDuration_ = interpolateAPSParameter(startAPS->hindCycleDuration_, endAPS->hindCycleDuration_, value);
//            } else {
//              newAPS.hindCycleDuration_ = value;
//            }
//
////            printf("cycleDuration: %lf val=%lf start=%lf end=%lf\n", newAPS.cycleDuration_, value, startAPS->cycleDuration_, endAPS->cycleDuration_);
//
//            if(pElem->QueryDoubleAttribute("foreLag", &value) != TIXML_SUCCESS) {
//              printf("Could not find foreLag!\n");
//              return false;
//            }
//            if (interpolateAPSParams) {
//              newAPS.foreLag_ = interpolateAPSParameter(startAPS->foreLag_, endAPS->foreLag_, value);
//            } else {
//              newAPS.foreLag_ = value;
//            }
////            printf("foreLag: %lf val=%lf start=%lf end=%lf\n", newAPS.foreLag_, value, startAPS->foreLag_, endAPS->foreLag_);
//
//            if(pElem->QueryDoubleAttribute("hindLag", &value) != TIXML_SUCCESS) {
//              printf("Could not find hindLag!\n");
//              return false;
//            }
//            if (interpolateAPSParams) {
//              newAPS.hindLag_ = interpolateAPSParameter(startAPS->hindLag_, endAPS->hindLag_, value);
//            } else {
//              newAPS.hindLag_ = value;
//            }
////            printf("hindLag: %lf val=%lf start=%lf end=%lf\n", newAPS.hindLag_, value, startAPS->hindLag_, endAPS->hindLag_);
//
//            if(pElem->QueryDoubleAttribute("pairLag", &value) != TIXML_SUCCESS) {
//              printf("Could not find pairLag!\n");
//              return false;
//            }
//            if (interpolateAPSParams) {
//              newAPS.pairLag_ = interpolateAPSParameter(startAPS->pairLag_, endAPS->pairLag_, value);
//            } else {
//              newAPS.pairLag_ =  value;
//            }
////            printf("pairLag: %lf val=%lf start=%lf end=%lf\n", newAPS.pairLag_, value, startAPS->pairLag_, endAPS->pairLag_);
//
//            if(pElem->QueryDoubleAttribute("foreDutyFactor", &value) != TIXML_SUCCESS) {
//              printf("Could not find foreDutyFactor!\n");
//              return false;
//            }
//            if (interpolateAPSParams) {
//              newAPS.foreDutyFactor_ = interpolateAPSParameter(startAPS->foreDutyFactor_, endAPS->foreDutyFactor_, value);
//            } else {
//              newAPS.foreDutyFactor_ = value;
//            }
////            printf("foreDutyFactor: %lf val=%lf start=%lf end=%lf\n", newAPS.foreDutyFactor_, value, startAPS->foreDutyFactor_, endAPS->foreDutyFactor_);
//
//            if(pElem->QueryDoubleAttribute("hindDutyFactor", &value) != TIXML_SUCCESS) {
//              printf("Could not find hindDutyFactor!\n");
//              return false;
//            }
//            if (interpolateAPSParams) {
//              newAPS.hindDutyFactor_ = interpolateAPSParameter(startAPS->hindDutyFactor_, endAPS->hindDutyFactor_, value);
//            } else {
//              newAPS.hindDutyFactor_ = value;
//            }
////            printf("hindDutyFactor: %lf val=%lf start=%lf end=%lf\n", newAPS.hindDutyFactor_, value, startAPS->hindDutyFactor_, endAPS->hindDutyFactor_);
//
//
//            pElem = childCycle->FirstChild("Interpolation")->ToElement();
//            if(!pElem) {
//              printf("Could not find Interpolation!\n");
//              return false;
//            }
//            if(pElem->QueryDoubleAttribute("value", &newAPS.interpolate_) != TIXML_SUCCESS) {
//              printf("Could not find Interpolation::value!\n");
//              return false;
//            }
//
//            cycleCounter++;
//            gaitTransition->listAPS_.push_back(newAPS);
//          }
//          // add end gait
//          newAPS = *endAPS;
//          newAPS.interpolate_= 1.0;
//          gaitTransition->listAPS_.push_back(newAPS);



        }


      } else {
        if (pElem->QueryStringAttribute("name", &name)!=TIXML_SUCCESS) {
          printf("Could not find name of a parameter set!\n");
          return false;
        }

        std::string parameterFileSuffix = isRealRobot_ ? "" : "Sim";
        std::string parameterFilePath = pathToParameterFiles_ +"/" + name + parameterFileSuffix + ".xml";
        locomotionController_.reset(new LocomotionControllerDynamicGaitDefault(parameterFilePath, robotModel_, terrain_, time_step_));
        if(!locomotionController_->initialize(time_step_)) {
          printf("Could not load parameter file %s\n", parameterFilePath.c_str());
          return false;
        }


      }
      /* we are done */
//      for (int i=0; i<locomotionControllers_.size(); i++) {
//        std::cout << "gait in list: " << locomotionControllers_[i].getGaitName() << std::endl;
//      }
      return true;
    }

    counter++;
  }





  return true;
}

LocomotionControllerDynamicGaitDefault* GaitSwitcherDynamicGaitDefault::getLocomotionController() {
  return locomotionController_.get();
}


} /* namespace loco */
