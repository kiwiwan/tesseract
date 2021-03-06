/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2011, Willow Garage, Inc.
 *  All rights reserved.
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
 *   * Neither the name of Willow Garage nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
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
 *********************************************************************/

/* Author: Ioan Sucan */

#ifndef TESSERACT_MONITORING_CURRENT_STATE_H
#define TESSERACT_MONITORING_CURRENT_STATE_H

#include <ros/ros.h>
#include <sensor_msgs/JointState.h>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <tesseract_ros/ros_basic_env.h>
#include <memory>
#include <functional>
#include <unordered_map>

namespace tesseract
{
namespace tesseract_monitoring
{
typedef std::function<void(const sensor_msgs::JointStateConstPtr& joint_state)> JointStateUpdateCallback;

/** @class CurrentStateMonitor
    @brief Monitors the joint_states topic and tf to maintain the current state of the robot. */
class CurrentStateMonitor
{
public:
  /**
   * @brief Constructor.
   * @param robot_model The current kinematic model to build on
   * @param tf A pointer to the tf transformer to use
   */
  CurrentStateMonitor(const tesseract_ros::ROSBasicEnvConstPtr& env);

  /** @brief Constructor.
   *  @param robot_model The current kinematic model to build on
   *  @param tf A pointer to the tf transformer to use
   *  @param nh A ros::NodeHandle to pass node specific options
   */
  CurrentStateMonitor(const tesseract_ros::ROSBasicEnvConstPtr& env, ros::NodeHandle nh);

  ~CurrentStateMonitor();

  /** @brief Start monitoring joint states on a particular topic
   *  @param joint_states_topic The topic name for joint states (defaults to "joint_states")
   */
  void startStateMonitor(const std::string& joint_states_topic = "joint_states");

  /** @brief Stop monitoring the "joint_states" topic
   */
  void stopStateMonitor();

  /** @brief Check if the state monitor is started */
  bool isActive() const;

  /** @brief Get the RobotModel for which we are monitoring state */
  const tesseract_ros::ROSBasicEnvConstPtr& getEnvironment() const { return env_; }
  /** @brief Get the name of the topic being monitored. Returns an empty string if the monitor is inactive. */
  std::string getMonitoredTopic() const;

  /** @brief Query whether we have joint state information for all DOFs in the kinematic model
   *  @return False if we have no joint state information for one or more of the joints
   */
  bool haveCompleteState() const;

  /** @brief Query whether we have joint state information for all DOFs in the kinematic model
   *  @return False if we have no joint state information for one of the joints or if our state
   *  information is more than \e age old
   */
  bool haveCompleteState(const ros::Duration& age) const;

  /** @brief Query whether we have joint state information for all DOFs in the kinematic model
   *  @param missing_joints Returns the list of joints that are missing
   *  @return False if we have no joint state information for one or more of the joints
   */
  bool haveCompleteState(std::vector<std::string>& missing_joints) const;

  /** @brief Query whether we have joint state information for all DOFs in the kinematic model
   *  @param age The max allowed age of the joint state information
   *  @param missing_states Returns the list of joints that are missing
   *  @return False if we have no joint state information for one of the joints or if our state
   *  information is more than \e age old*/
  bool haveCompleteState(const ros::Duration& age, std::vector<std::string>& missing_states) const;

  /** @brief Get the current state
   *  @return Returns the current state */
  EnvStatePtr getCurrentState() const;

  /** @brief Set the state \e upd to the current state maintained by this class. */
  void setToCurrentState(EnvState& upd) const;

  /** @brief Get the time stamp for the current state */
  ros::Time getCurrentStateTime() const;

  /** @brief Get the current state and its time stamp
   *  @return Returns a pair of the current state and its time stamp */
  std::pair<EnvStatePtr, ros::Time> getCurrentStateAndTime() const;

  /** @brief Get the current state values as a map from joint names to joint state values
   *  @return Returns the map from joint names to joint state values*/
  std::unordered_map<std::string, double> getCurrentStateValues() const;

  /** @brief Wait for at most \e wait_time seconds (default 1s) for a robot state more recent than t
   *  @return true on success, false if up-to-date robot state wasn't received within \e wait_time
  */
  bool waitForCurrentState(const ros::Time t = ros::Time::now(), double wait_time = 1.0) const;

  /** @brief Wait for at most \e wait_time seconds until the complete robot state is known.
      @return true if the full state is known */
  bool waitForCompleteState(double wait_time) const;

  /** @brief Wait for at most \e wait_time seconds until the joint values from the group \e group are known. Return true
   * if values for all joints in \e group are known */
  bool waitForCompleteState(const std::string& manip, double wait_time) const;

  /** @brief Get the time point when the monitor was started */
  const ros::Time& getMonitorStartTime() const { return monitor_start_time_; }
  /** @brief Add a function that will be called whenever the joint state is updated*/
  void addUpdateCallback(const JointStateUpdateCallback& fn);

  /** @brief Clear the functions to be called when an update to the joint state is received */
  void clearUpdateCallbacks();

  /** @brief When a joint value is received to be out of bounds, it is changed slightly to fit within bounds,
   *  if the difference is less than a specified value (labeled the "allowed bounds error").
   *  This value can be set using this function.
   *  @param error The specified value for the "allowed bounds error". The default is machine precision. */
  void setBoundsError(double error) { error_ = (error > 0) ? error : -error; }
  /** @brief When a joint value is received to be out of bounds, it is changed slightly to fit within bounds,
   *  if the difference is less than a specified value (labeled the "allowed bounds error").
   *  @return The stored value for the "allowed bounds error"
   */
  double getBoundsError() const { return error_; }
  /** @brief Allow the joint_state arrrays velocity and effort to be copied into the robot state
   *  this is useful in some but not all applications
   */
  void enableCopyDynamics(bool enabled) { copy_dynamics_ = enabled; }
private:
  void jointStateCallback(const sensor_msgs::JointStateConstPtr& joint_state);
  bool isPassiveOrMimicDOF(const std::string& dof) const;

  ros::NodeHandle nh_;
  tesseract_ros::ROSBasicEnvConstPtr env_;
  EnvState state_;
  std::map<std::string, ros::Time> joint_time_;
  bool state_monitor_started_;
  bool copy_dynamics_;  // Copy velocity and effort from joint_state
  ros::Time monitor_start_time_;
  double error_;
  ros::Subscriber joint_state_subscriber_;
  ros::Time current_state_time_;
  ros::Time last_tf_update_;

  mutable boost::mutex state_update_lock_;
  mutable boost::condition_variable state_update_condition_;
  std::vector<JointStateUpdateCallback> update_callbacks_;
};
typedef std::shared_ptr<CurrentStateMonitor> CurrentStateMonitorPtr;
typedef std::shared_ptr<const CurrentStateMonitor> CurrentStateMonitorConstPtr;
}
}
#endif
