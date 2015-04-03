//=================================================================================================
// Copyright (c) 2012, Stefan Kohlbrecher, TU Darmstadt
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of the Simulation, Systems Optimization and Robotics
//       group, TU Darmstadt nor the names of its contributors may be used to
//       endorse or promote products derived from this software without
//       specific prior written permission.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//=================================================================================================


#include "ros/ros.h"
#include <hector_exploration_planner/hector_exploration_planner.h>
#include <costmap_2d/costmap_2d_ros.h>
#include <hector_nav_msgs/GetRobotTrajectory.h>
#include <geometry_msgs/PoseStamped.h>
#include <nav_msgs/GetPlan.h>


class SimpleExplorationPlanner
{
public:
  SimpleExplorationPlanner()
  {
    ros::NodeHandle nh;

    costmap_2d_ros_ = new costmap_2d::Costmap2DROS("global_costmap", tfl_);

    planner_ = new hector_exploration_planner::HectorExplorationPlanner();
    planner_->initialize("hector_exploration_planner",costmap_2d_ros_);

    exploration_plan_service_server_ = nh.advertiseService("get_exploration_path", &SimpleExplorationPlanner::explorationServiceCallback, this);
    make_plan_service_server_ = nh.advertiseService("get_plan", &SimpleExplorationPlanner::makePlanServiceCallback, this);
    frontiers_service_server_ = nh.advertiseService("get_frontiers", &SimpleExplorationPlanner::frontiersServiceCallback, this);

    exploration_plan_pub_ = nh.advertise<nav_msgs::Path>("exploration_path",2);
  }

  bool explorationServiceCallback(hector_nav_msgs::GetRobotTrajectory::Request  &req,
                                  hector_nav_msgs::GetRobotTrajectory::Response &res )
    {
      ROS_INFO("Exploration Service called");

      tf::Stamped<tf::Pose> robot_pose_tf;
      costmap_2d_ros_->getRobotPose(robot_pose_tf);

      geometry_msgs::PoseStamped pose;
      tf::poseStampedTFToMsg(robot_pose_tf, pose);
      planner_->doExploration(pose, res.trajectory.poses);
      res.trajectory.header.frame_id = "map";
      res.trajectory.header.stamp = ros::Time::now();

      if (exploration_plan_pub_.getNumSubscribers() > 0)
      {
        exploration_plan_pub_.publish(res.trajectory);
      }

      return true;
    }

  bool makePlanServiceCallback(nav_msgs::GetPlan::Request  &req,
                               nav_msgs::GetPlan::Response &res )
    {
      ROS_INFO("Exploration Service called");

//       tf::Stamped<tf::Pose> robot_pose_tf;
//       costmap_2d_ros_->getRobotPose(robot_pose_tf);
// 
//       geometry_msgs::PoseStamped pose;
//       tf::poseStampedTFToMsg(robot_pose_tf, pose);
      planner_->makePlan(req.start, req.goal, res.plan.poses);
      res.plan.header.frame_id = "map";
      res.plan.header.stamp = ros::Time::now();

      if (exploration_plan_pub_.getNumSubscribers() > 0)
      {
        exploration_plan_pub_.publish(res.plan);
      }

      return true;
    }

  bool frontiersServiceCallback(hector_nav_msgs::GetRobotTrajectory::Request  &req,
                                  hector_nav_msgs::GetRobotTrajectory::Response &res )
    {
      ROS_INFO("Frontier Service called");

      std::vector<geometry_msgs::PoseStamped> frontiers;
      if(!planner_->findFrontiers(frontiers)) {
        ROS_ERROR("findFrontiers returned 0 frontiers.");
        return false;
      }
      res.trajectory.header.frame_id = "map";
      res.trajectory.header.stamp = ros::Time::now();
      res.trajectory.poses = frontiers;

//       if (exploration_plan_pub_.getNumSubscribers() > 0)
//       {
//         exploration_plan_pub_.publish(res.trajectory);
//       }

      return true;
    }

protected:
  hector_exploration_planner::HectorExplorationPlanner* planner_;
  ros::ServiceServer exploration_plan_service_server_;
  ros::ServiceServer frontiers_service_server_;
  ros::ServiceServer make_plan_service_server_;
  ros::Publisher exploration_plan_pub_;
  costmap_2d::Costmap2DROS* costmap_2d_ros_;
  tf::TransformListener tfl_;

};

int main(int argc, char **argv) {
  ros::init(argc, argv, ROS_PACKAGE_NAME);

  SimpleExplorationPlanner ep;

  ros::spin();

  return 0;
}
