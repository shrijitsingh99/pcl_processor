/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: Shrijit Singh <shrijitsingh99@gmail.com>
 *
 */

#include <algorithm>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "pcl_processor/segmentation/extract_clusters.hpp"

#include <pcl/common/centroid.h>

namespace pcl_processor
{

template<typename PointT>
typename EuclideanClusterExtraction<PointT>::PointCloudPtr EuclideanClusterExtraction<PointT>::
process(
  EuclideanClusterExtraction<PointT>::PointCloudPtr cloud_in)
{
  // TODO(shrijitsingh99): Possibly run this on a seperate thread
  cluster_indices_.clear();
  kdtree_->setInputCloud(cloud_in);
  processor_.setInputCloud(cloud_in);
  processor_.extract(cluster_indices_);

  publish_clusters(cloud_in, cluster_indices_);

  return cloud_in;
}

template<typename PointT>
void EuclideanClusterExtraction<PointT>::declare_parameters(rclcpp::Node::SharedPtr & node)
{
  param_callback_ = node->add_on_set_parameters_callback(
    [&](auto && arg)
    {
      return set_parameters(std::forward<decltype(arg)>(arg));
    });

  node->declare_parameter<double>(plugin_name_ + ".cluster_tolerance", 0.0);
  node->declare_parameter<int64_t>(plugin_name_ + ".min_cluster_size", 0);
  // TODO(shrijitsingh99): Look into max cluster size index limit (i.e. pcl::uindex_t limit)
  node->declare_parameter<int64_t>(
    plugin_name_ + ".max_cluster_size",
    std::numeric_limits<pcl::uindex_t>::max());
  node->declare_parameter<bool>(plugin_name_ + ".publish_markers", false);
}

template<typename PointT>
rcl_interfaces::msg::SetParametersResult
EuclideanClusterExtraction<PointT>::set_parameters(
  const std::vector<rclcpp::Parameter> & parameters)
{
  rcl_interfaces::msg::SetParametersResult result;
  result.successful = true;
  for (const auto & parameter : parameters) {
    // TODO(shrijitsingh99): Add try-catch for catching any parameter exceptions and set parameter
    // result to false
    const std::string & parameter_name = parameter.get_name();
    if (parameter_name == plugin_name_ + ".cluster_tolerance") {
      processor_.setClusterTolerance(parameter.as_double());
    } else if (parameter_name == plugin_name_ + ".min_cluster_size") {
      processor_.setMinClusterSize(static_cast<pcl::uindex_t>(std::max(0l, parameter.as_int())));
    } else if (parameter_name == plugin_name_ + ".max_cluster_size") {
      processor_.setMaxClusterSize(static_cast<pcl::uindex_t>(std::max(0l, parameter.as_int())));
    } else if (parameter_name == plugin_name_ + ".publish_markers") {
      publish_markers_ = parameter.as_bool();
    }
  }
  return result;
}

template<typename PointT>
void EuclideanClusterExtraction<PointT>::publish_clusters(
  EuclideanClusterExtraction<PointT>::PointCloudConstPtr cloud_in,
  const std::vector<pcl::PointIndices> & cluster_indices) const
{
  // TODO(shrijitsingh99): Add publishing poitcloud messsages using Detection3DArray msg
  // https://github.com/ros-perception/vision_msgs/blob/kinetic-devel/msg/Detection3DArray.msg
  if (publish_markers_) {
    visualization_msgs::msg::Marker marker_msg;
    marker_msg.header.frame_id = cloud_in->header.frame_id;
    marker_msg.header.stamp = node_.lock()->now();
    marker_msg.ns = plugin_name_ + "/clusters";
    marker_msg.type = visualization_msgs::msg::Marker::SPHERE_LIST;
    marker_msg.action = visualization_msgs::msg::Marker::ADD;
    marker_msg.pose.orientation.w = 1.0;
    marker_msg.scale.x = 0.25;
    marker_msg.scale.y = 0.25;
    marker_msg.scale.z = 0.25;
    marker_msg.color.a = 1.0;
    marker_msg.color.r = 0.0;
    marker_msg.color.g = 1.0;
    marker_msg.color.b = 0.0;

    pcl::PointCloud<PointT> cloud_cluster;
    for (std::vector<pcl::PointIndices>::const_iterator it = cluster_indices.begin();
      it != cluster_indices.end(); ++it)
    {
      for (const auto & idx : it->indices) {
        cloud_cluster.push_back((*cloud_in)[idx]);
      }
      cloud_cluster.width = cloud_cluster.size();
      cloud_cluster.height = 1;
      cloud_cluster.is_dense = true;
      Eigen::Vector4f centroid;
      pcl::compute3DCentroid(cloud_cluster, centroid);
      geometry_msgs::msg::Point pt;
      pt.x = centroid[0];
      pt.y = centroid[1];
      pt.z = centroid[2];
      marker_msg.points.push_back(pt);
      cloud_cluster.points.clear();
    }
    cluster_marker_pub_->publish(marker_msg);
  }
}
}  // namespace pcl_processor

#include <pluginlib/class_list_macros.hpp>

PLUGINLIB_EXPORT_CLASS(
  pcl_processor::EuclideanClusterExtraction<pcl::PointXYZ>,
  pcl_processor::PointCloudProcessor<pcl::PointXYZ>)
