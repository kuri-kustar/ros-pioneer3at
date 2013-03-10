/*
 * Copyright 2012 Open Source Robotics Foundation
 * Copyright 2013 Dereck Wonnacott
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
*/

#include <ros/ros.h>
#include <geometry_msgs/Twist.h>
#include <tf/transform_broadcaster.h>
#include <nav_msgs/Odometry.h>

#include <gazebo/transport/transport.hh>
#include <gazebo/msgs/msgs.hh>
#include <math/gzmath.hh>

#include <iostream>

gazebo::transport::PublisherPtr gz_vel_cmd_pub;
ros::Publisher ros_odom_pub;
tf::TransformBroadcaster *odom_broadcaster;
int ros_odom_pub_seq;
std::string gName;

/////////////////////////////////////////////////
void ros_cmd_vel_Callback(const geometry_msgs::Twist::ConstPtr& msg_in)
{ 
  // Generate a pose
  gazebo::math::Pose pose(msg_in->linear.x,
                          msg_in->linear.y,
                          msg_in->linear.z,
                          msg_in->angular.x,
                          msg_in->angular.y,
                          msg_in->angular.z);
  
  // Convert to a pose message
  gazebo::msgs::Pose msg_out;
  gazebo::msgs::Set(&msg_out, pose);
  gz_vel_cmd_pub->Publish(msg_out);
}

/////////////////////////////////////////////////
void gz_odom_Callback(ConstPose_VPtr &msg_in)
{ 
  //std::cout << "gz_odom" << msg_in->DebugString() << std::endl;

  float x  = 0;
  float y  = 0;
  geometry_msgs::Quaternion odom_quat = tf::createQuaternionMsgFromYaw(0);
  
  for (int i = 0; i < msg_in->pose_size(); i++) 
    if(msg_in->pose(i).name() == gName)
    {
      //std::cout << msg_in->pose(i).DebugString() << std::endl;
      x = msg_in->pose(i).position().x();
      y = msg_in->pose(i).position().y();
      odom_quat.x = msg_in->pose(i).orientation().x();
      odom_quat.y = msg_in->pose(i).orientation().y();
      odom_quat.z = msg_in->pose(i).orientation().z();
      odom_quat.w = msg_in->pose(i).orientation().w();
      break;
    }    

  geometry_msgs::TransformStamped odom_trans;
  odom_trans.header.stamp = ros::Time::now();
  odom_trans.header.frame_id = "odom";
  odom_trans.child_frame_id = "base_link";
  odom_trans.transform.translation.x = x;
  odom_trans.transform.translation.y = y;
  odom_trans.transform.translation.z = 0.0;
  odom_trans.transform.rotation = odom_quat;
  odom_broadcaster->sendTransform(odom_trans);

  nav_msgs::Odometry odom;
  odom.header                 = odom_trans.header;
  odom.child_frame_id         = odom_trans.child_frame_id;
  odom.pose.pose.position.x   = x;
  odom.pose.pose.position.y   = y;
  odom.pose.pose.position.z   = 0.0;
  odom.pose.pose.orientation  = odom_quat;
  odom.twist.twist.linear.x   = 0.0;  //vx;
  odom.twist.twist.linear.y   = 0.0;  //vy;
  odom.twist.twist.angular.z  = 0.0;  //vth;
  ros_odom_pub.publish(odom);
}

/////////////////////////////////////////////////
int main( int argc, char* argv[] )
{
  // Initialize ROS
  ros::init(argc, argv, "Pioneer3AT");
  gName = ros::this_node::getName();
  ros::NodeHandle n_("~");
  ros::Rate loop_rate(10);
  ros_odom_pub = n_.advertise<nav_msgs::Odometry>("odom", 10);
  odom_broadcaster = new tf::TransformBroadcaster;
  ros_odom_pub_seq = 0;
  
  
  // Initialize Gazebo
  gazebo::transport::init();
  gazebo::transport::NodePtr node(new gazebo::transport::Node());
  node->Init();
  gazebo::transport::run();
  gz_vel_cmd_pub = node->Advertise<gazebo::msgs::Pose>(std::string("~") + gName + 
                                                       std::string("/vel_cmd") );
    
  // Subscribers
  gazebo::transport::SubscriberPtr gz_odom_sub = node->Subscribe("~/pose/info", gz_odom_Callback);
  ros::Subscriber ros_cmd_vel_sub = n_.subscribe("cmd_vel", 10, ros_cmd_vel_Callback);
  
  // Spin
  ros::spin();

  // Shutdown
  gazebo::transport::fini();
}



