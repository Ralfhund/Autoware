/*
 * NdtLocalizer.cpp
 *
 *  Created on: Sep 21, 2018
 *      Author: sujiwo
 */

#include <stdlib.h>
#include <pcl/io/pcd_io.h>

#include "utilities.h"
#include "NdtLocalizer.h"


using namespace std;
using pcl::PointXYZ;
using namespace Eigen;


void pose_mod(Posture *pose)
{
	while (pose->theta < -M_PI)
		pose->theta += 2 * M_PI;
	while (pose->theta > M_PI)
		pose->theta -= 2 * M_PI;
	while (pose->theta2 < -M_PI)
		pose->theta2 += 2 * M_PI;
	while (pose->theta2 > M_PI)
		pose->theta2 -= 2 * M_PI;
	while (pose->theta3 < -M_PI)
		pose->theta3 += 2 * M_PI;
	while (pose->theta3 > M_PI)
		pose->theta3 -= 2 * M_PI;
}


inline double nrand(double n)
{
	double r;
	r = n * sqrt(-2.0 * log((double)rand() / RAND_MAX)) * cos(2.0 * M_PI * rand() / RAND_MAX);
	return r;
}


NdtLocalizer::NdtLocalizer(const NdtLocalizerInitialConfig &initialConfig) :

	ndMap(initialize_NDmap())

{}


void
NdtLocalizer::loadMap (const std::string &filename)
{
	pcl::PointCloud<PointXYZ>::Ptr mapInp (new pcl::PointCloud<PointXYZ>);

	if (mapLoaded==false) {

		pcl::PCDReader fReader;
		fReader.read(filename, *mapInp);
		loadMap(mapInp);
	}
}


NdtLocalizer::~NdtLocalizer()
{}


void
NdtLocalizer::putEstimation (const Pose &pEst)
{
	Vector3d rotations = quaternionToRPY(pEst.orientation());
	prev_pose.x, prev_pose.y, prev_pose.z =
		pEst.position().x(), pEst.position().y(), pEst.position().z();
	prev_pose.theta, prev_pose.theta2, prev_pose.theta3 =
		rotations.x(), rotations.y(), rotations.z();
	prev_pose2 = prev_pose;
}


void
NdtLocalizer::loadMap (pcl::PointCloud<PointXYZ>::ConstPtr mapcloud)
{
	for (auto pointIt=mapcloud->begin(); pointIt!=mapcloud->end(); ++pointIt) {
		auto &pointSrc = *pointIt;
		Point pnd = {
			pointSrc.x,
			pointSrc.y,
			pointSrc.z};
		add_point_map(ndMap, &pnd);
	}

	mapLoaded = true;
}


Pose
NdtLocalizer::localize (pcl::PointCloud<pcl::PointXYZ>::ConstPtr scan)
{
	Posture npose, bpose;
	double e = 0;
	double x_offset, y_offset, z_offset, theta_offset;
	int iteration;

	vector<Point> ndtScanPoints (scan->size());
	int j = 0;
	for (int i=0, j=0; i<scan->size(); i++) {
		auto &p = scan->at(i);
		auto &pt = ndtScanPoints.at(i);
		pt.x = p.x + nrand(0.01);
		pt.y = p.y + nrand(0.01);
		pt.z = p.z + nrand(0.01);
		double dist = pt.x*pt.x + pt.y*pt.y + pt.z*pt.z;
		if (dist < 3*3) {

		}
		j++;
		if (j > 130000)
			break;
	}
	int scan_points_num = j;

	// calculate offset
	x_offset = prev_pose.x - prev_pose2.x;
	y_offset = prev_pose.y - prev_pose2.y;
	z_offset = prev_pose.z - prev_pose2.z;
	theta_offset = prev_pose.theta3 - prev_pose2.theta3;

	if (theta_offset < -M_PI)
		theta_offset += 2 * M_PI;
	if (theta_offset > M_PI)
		theta_offset -= 2 * M_PI;

	// calc estimated initial position
	npose.x = prev_pose.x + x_offset;
	npose.y = prev_pose.y + y_offset;
	npose.z = prev_pose.z + z_offset;
	npose.theta = prev_pose.theta;
	npose.theta2 = prev_pose.theta2;
	npose.theta3 = prev_pose.theta3 + theta_offset;

	int layer_select = LAYER_NUM - 1;
	for (layer_select = 1; layer_select >= 1; layer_select -= 1) {
		for (int j=0; j<100; j++) {
			if (layer_select != 1 && j > 2) {
				break;
			}
			bpose = npose;

			e = adjust3d(ndtScanPoints.data(), scan_points_num, &npose, layer_select);
			pose_mod(&npose);

			if ((bpose.x - npose.x) * (bpose.x - npose.x) + (bpose.y - npose.y) * (bpose.y - npose.y) +
				(bpose.z - npose.z) * (bpose.z - npose.z) + 3 * (bpose.theta - npose.theta) * (bpose.theta - npose.theta) +
				3 * (bpose.theta2 - npose.theta2) * (bpose.theta2 - npose.theta2) +
				3 * (bpose.theta3 - npose.theta3) * (bpose.theta3 - npose.theta3) <
				1e-5) {
				break;
			}
		}
		iteration = j;

		if (layer_select == 2) {
			double rate, xrate, yrate, dx, dy, dtheta;
			double tempx, tempy;
			int i;

			tempx = (npose.x - prev_pose.x);
			tempy = (npose.y - prev_pose.y);
			dx = tempx * cos(-prev_pose.theta3) - tempy * sin(-prev_pose.theta3);
			dy = tempx * sin(-prev_pose.theta3) + tempy * cos(-prev_pose.theta3);
			dtheta = npose.theta3 - prev_pose.theta3;
			if (dtheta < -M_PI) {
				dtheta += 2 * M_PI;
			}
			if (dtheta > M_PI) {
				dtheta -= 2 * M_PI;
			}

			rate = dtheta / (double)scan_points_num;
			xrate = dx / (double)scan_points_num;
			yrate = dy / (double)scan_points_num;

			printf("antidist x %f y %f yaw %f\n", dx, dy, dtheta);

			dx = -dx;
			dy = -dy;
			dtheta = -dtheta;
			for (i = 0; i < scan_points_num; i++) {
				tempx = ndtScanPoints[i].x * cos(dtheta) - ndtScanPoints[i].y * sin(dtheta) + dx;
				tempy = ndtScanPoints[i].x * sin(dtheta) + ndtScanPoints[i].y * cos(dtheta) + dy;

				ndtScanPoints[i].x = tempx;
				ndtScanPoints[i].y = tempy;

				dtheta += rate;
				dx += xrate;
				dy += yrate;
			}
		}
	}

	// Localizer output
	Eigen::Translation3f translation(npose.x, npose.y, npose.z);
	Eigen::AngleAxisf rotation_x(npose.theta, Eigen::Vector3f::UnitX());
	Eigen::AngleAxisf rotation_y(npose.theta2, Eigen::Vector3f::UnitY());
	Eigen::AngleAxisf rotation_z(npose.theta3, Eigen::Vector3f::UnitZ());
	Eigen::Matrix4f local_t = (translation * rotation_z * rotation_y * rotation_x).matrix();
//	Eigen::Matrix4f global_t = tf_local_to_global * local_t;

	Pose ndtCPose = Pose::from_XYZ_RPY(Eigen::Vector3d(npose.x, npose.y,npose.z), npose.theta, npose.theta2, npose.theta3);

	return ndtCPose;
}
