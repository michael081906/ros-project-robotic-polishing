#include "ros/ros.h"
#include "std_msgs/String.h"
#include "s_hull_pro.h"
#include "pclIo.h"
#include "pclVoxel.h"
#include "pclMlsSmoothing.h"
#include "pclPassThrough.h"
#include "pclStatisticalOutlierRemoval.h"
#include "pclFastTriangular.h"
#include "pclCloudViewer.h"
#include "findNearestPoint.h"
#include "delaunay3.h"
#include "dijkstraPQ.h"
#include <pcl/io/io.h>
#include <pcl/io/pcd_io.h>
#include <pcl/point_types.h>
#include <pcl/filters/statistical_outlier_removal.h>
#include <pcl/visualization/cloud_viewer.h>
#include <pcl/filters/radius_outlier_removal.h>
#include <pcl/filters/conditional_removal.h>
#include <iostream>
#include <queue>
#include <stack>
#include <limits>
#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <hash_set>
#include <ctype.h>
#include <string.h>
#include <string>
#include <sys/types.h>
#include <hash_set>
#include <set>
#include <vector>
#include <fstream>
#include <math.h>
#include <time.h>
#include <sstream>
#include <pcl_ros/point_cloud.h>
#include <sensor_msgs/PointCloud2.h>
#include "robotic_polishing/Trajectory.h" // This header name name from the project name in CmakeList.txt, not physical folder name
#include <pcl_conversions/pcl_conversions.h>
#include <pcl_ros/transforms.h>

typedef pcl::PointCloud<pcl::PointXYZ> PointCloud;

PointCloud pointcloud_in;
PointCloud pointcloud_out;

void callback(const sensor_msgs::PointCloud2Ptr& cloud) {

  pcl::fromROSMsg(*cloud, pointcloud_in);  // copy sensor_msg::Pointcloud message into pcl::PointCloud

  // tf::Transform transformKinect;
  tf::Pose transformKinect;
  transformKinect.setOrigin(tf::Vector3(0.3, 0.0, 0.0));
  tf::Matrix3x3 rotationMatrix(0, 0, 1, -1, 0, 0, 0, -1, 0);
 // tf::Matrix3x3 rotationMatrix(1, 0, 0, 0, 1, 0, 0, 0, 1);
  transformKinect.setBasis(rotationMatrix);
  // tf::Quaternion q;
  // TODO: (Michael) What is Quaternion;
  // transformKinect.setRotation(tf::Quaternion(0, 0, 0, 1));
  pcl_ros::transformPointCloud(pointcloud_in, pointcloud_out, transformKinect);
}

bool find(robotic_polishing::Trajectory::Request &req,
          robotic_polishing::Trajectory::Response &res) {

  // 0. Initialization
  pclIo pclLoad;
  pclCloudViewer pclView;
  pclPassThrough pt;
  pclVoxel vx;
  pclStatistOutRev sor;
  pclMlsSmoothing mls;
  pclFastTriangular ft;
  pcl::PolygonMesh triangles;
  pcl::PointCloud<pcl::PointNormal>::Ptr cloud_with_normal(
      new pcl::PointCloud<pcl::PointNormal>);
  // pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_out(new pcl::PointCloud<pcl::PointXYZ>);
  pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_out;
  cloud_out = pointcloud_out.makeShared();
  pt.setInputCloud(*cloud_out);
  pt.setFilterZlimit(0, 10);
  pt.filterProcess(*cloud_out);
  //  4. Down sample the point cloud
  std::cout << " 4. Down sampling the point cloud, please wait..." << std::endl;
   vx.setInputCloud(*cloud_out);
   vx.setLeafSize(0.01, 0.01, 0.01);
   vx.filterProcess(*cloud_out);
   std::cout << " Down sampling completed" << std::endl;
  /****************************************************************************
  pcl::visualization::CloudViewer viewer("Cloud of Raw data");
  viewer.showCloud(cloud_out);
  int user_data = 0;
  do {
    user_data++;
  } while (!viewer.wasStopped());
  // ****************************************************************************/
  // pub_pcl.publish(cloud_out);

   //  2. Remove the noise of point cloud
   std::cout << " 2. Removing the noise of point cloud, please wait..."
   << std::endl;
   sor.setInputCloud(*cloud_out);
   sor.setMeanK(50);
   sor.setStddevMulThresh(1);
   sor.filterProcess(*cloud_out);
   //*******************************************************************
   // 2017.11.9 Added Michael
   pcl::RadiusOutlierRemoval<pcl::PointXYZ> outrem;
   // build the filter
   outrem.setInputCloud(cloud_out);
   outrem.setRadiusSearch(0.5);
   outrem.setMinNeighborsInRadius(30);
   // apply filter
   outrem.filter(*cloud_out);
   //*******************************************************************
   std::cout << " Removing completed" << std::endl;
   /*****************************************************************/

   //  3. Extract certain region of point cloud
   //  5. Smooth the point cloud
   std::cout << " 5. Smoothing the point cloud, please wait..." << std::endl;
   mls.setInputCloud(*cloud_out);
   mls.setSearchRadius(0.05);
   mls.mlsProcess(*cloud_with_normal);
   std::cout << " Smoothing completed" << std::endl;
   //  7. Mesh the obstacle
   std::cout << " 7. Meshing the obstacle, please wait..." << std::endl;
   ft.setInputCloud(*cloud_with_normal);
   ft.setSearchRadius(0.05);
   ft.reconctruct(triangles);
   std::vector<int> gp33 = ft.getSegID();
   int sizegp3 = gp33.size();
   int sizePointCloud = cloud_with_normal->size();
   std::cout << " Meshing completed..." << std::endl;
   std::cout << " size of gp3: " << sizegp3 << std::endl;
   //  8. Show the result
   // pclView.display(triangles);

   // 9.mergeHanhsPoint
   std::cout << " 9. Merging point of selected point, please wait..."
   << std::endl;
   findNearestPoint mg;
   std::vector<int> selectedPoint;

   //mg.readtext("./src/Robotic-polishing/kidney3dots3_pointCluster.txt");
   mg.setPosition(req.start);
   mg.setPosition(req.end);

   mg.setInputCloud(*cloud_with_normal);
   mg.findNearestProcess(selectedPoint);
   std::cout << " 9. Merging completed" << std::endl;
   std::vector<int> size3;
   size3 = ft.getSegID();
   std::cout << selectedPoint[0] << " " << selectedPoint[1] << " "
   << size3.size() << std::endl;
   std::cout << " 10. Delaunay3 function..." << std::endl;
   std::vector<Triad> triads;
   std::vector<Shx> ptsOut;
   delaunay3 dy3;
   dy3.setInputCloud(*cloud_with_normal);
   dy3.putPointCloudIntoShx();
   dy3.processDelaunay(triads);
   dy3.getShx(ptsOut);
   // write_Triads(triads, "triangles.txt");
   // write_Shx(ptsOut, "pts.txt");
   std::cout << " 10. Delaunay3 function completed..." << std::endl;
   int start = selectedPoint[0];
   int end = selectedPoint[1];
   std::vector<int> path;
   dijkstraPQ dPQ(size3.size());
   dPQ.setInputCloud(*cloud_with_normal);
   dPQ.setTri(triads);
   dPQ.computeWeight();
   dPQ.shortestPath(start, end);
   dPQ.returnDijkstraPath(start, end, path);

   std::vector<int>::iterator route = path.begin();
   while (route != path.end()) {
   std::cout << *route << " ";
   ++route;
   }
   std::cout << std::endl;
   std::vector<position> POS;
   dPQ.returnDijkstraPathPosition(start, end, POS);
   std::vector<position>::iterator routePos = POS.begin();

   float px,py,pz;
   while (routePos != POS.end()) {
   std::cout << (*routePos).x << " ";  // p.80
   px=(*routePos).x;
   res.path_x.push_back(px);
   ++routePos;
   }
   std::cout << std::endl;
   routePos = POS.begin();
   while (routePos != POS.end()) {
   std::cout << (*routePos).y << " ";
   py=(*routePos).y;
   res.path_y.push_back(py);
   ++routePos;
   }
   std::cout << std::endl;
   routePos = POS.begin();
   while (routePos != POS.end()) {
   std::cout << (*routePos).z << " ";
   pz=(*routePos).z;
   res.path_z.push_back(pz);
   ++routePos;
   }

//---------------------------------------------------//
  return true;
}

/**
 * This tutorial demonstrates simple sending of messages over the ROS system.
 */
int main(int argc, char **argv) {

  ros::init(argc, argv, "talker");
  ros::NodeHandle n, nh;
  ros::Publisher chatter_pub = n.advertise<std_msgs::String>("chatter", 1000);
  // ros::Publisher pub_pcl = nh.advertise<PointCloud>("test", 10);
  ros::Subscriber sub_pcl = n.subscribe("camera/depth/points", 20, &callback);
  ros::ServiceServer service = n.advertiseService("FindTrajectory", &find);

  ros::Rate loop_rate(10);

  int count = 0;
  while (ros::ok()) {
    std_msgs::String msg;
    std::stringstream ss;
    ss << "hello world " << count;
    msg.data = ss.str();

    ROS_INFO("%s", msg.data.c_str());
    chatter_pub.publish(msg);
    ros::spinOnce();
    loop_rate.sleep();
    ++count;
  }

  return 0;
}
