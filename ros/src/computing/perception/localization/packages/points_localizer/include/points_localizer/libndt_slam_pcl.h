/*
 *  Copyright (c) 2017, Tier IV, Inc.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither the name of Autoware nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 *  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 *  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 *  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef LIBNDT_SLMA_PCL_H
#define LIBNDT_SLMA_PCL_H

#include "liblocalizer.h"
#include "libndt_slam_param_interface.h"

#include <iostream>
#include <string>
#include <chrono>

#include <pcl/io/io.h>
#include <pcl/io/pcd_io.h>
#include <pcl/point_types.h>
#include <pcl/registration/ndt.h>

template <class PointT>
class LibNdtSlamPCL
    : public LibLocalizer<PointT>
    , public LibNdtSlamParamInterface
{
    public:
        LibNdtSlamPCL();
        ~LibNdtSlamPCL(){};

        void setTransformationEpsilon(double trans_eps) override;
        void setStepSize(double step_size) override;
        void setResolution(float res) override;
        void setMaximumIterations(int max_iter) override;

        double getTransformationEpsilon() override;
        double getStepSize() const override;
        float getResolution() const override;
        int getMaximumIterations() override;

    protected:
        void align(const Pose& predict_pose) override;
        double getFitnessScore() override;
        void setInputTarget(const boost::shared_ptr< pcl::PointCloud<PointT> const>& map_ptr) override;
        void setInputSource(const boost::shared_ptr< pcl::PointCloud<PointT> const>& scan_ptr) override;
        Pose getFinalPose() override;

    private:
        pcl::NormalDistributionsTransform<PointT, PointT> ndt_;
};

template <class PointT>
LibNdtSlamPCL<PointT>::LibNdtSlamPCL()
{
}

template <class PointT>
void LibNdtSlamPCL<PointT>::setTransformationEpsilon(double trans_eps)
{
    ndt_.setTransformationEpsilon(trans_eps);
}

template <class PointT>
void LibNdtSlamPCL<PointT>::setStepSize(double step_size)
{
    ndt_.setStepSize(step_size);
}

template <class PointT>
void LibNdtSlamPCL<PointT>::setResolution(float res)
{
    ndt_.setResolution(res);
}

template <class PointT>
void LibNdtSlamPCL<PointT>::setMaximumIterations(int max_iter)
{
    ndt_.setMaximumIterations(max_iter);
}

template <class PointT>
double LibNdtSlamPCL<PointT>::getTransformationEpsilon()
{
    return ndt_.getTransformationEpsilon();
}

template <class PointT>
double LibNdtSlamPCL<PointT>::getStepSize() const
{
    return ndt_.getStepSize();
}

template <class PointT>
float LibNdtSlamPCL<PointT>::getResolution() const
{
    return ndt_.getResolution();
}

template <class PointT>
int LibNdtSlamPCL<PointT>::getMaximumIterations()
{
    return ndt_.getMaximumIterations();
}

template <class PointT>
void LibNdtSlamPCL<PointT>::align(const Pose& predict_pose)
{
    const auto predict_matrix = convertToEigenMatrix4f(predict_pose);
    pcl::PointCloud<PointT> output_cloud;
    ndt_.align(output_cloud, predict_matrix);
}

template <class PointT>
void LibNdtSlamPCL<PointT>::setInputTarget(const boost::shared_ptr< pcl::PointCloud<PointT> const>& map_ptr)
{
    ndt_.setInputTarget(map_ptr);
}

template <class PointT>
void LibNdtSlamPCL<PointT>::setInputSource(const boost::shared_ptr< pcl::PointCloud<PointT> const>& scan_ptr)
{
    ndt_.setInputSource(scan_ptr);
}

template <class PointT>
double LibNdtSlamPCL<PointT>::getFitnessScore()
{
    return ndt_.getFitnessScore();
}

template <class PointT>
Pose LibNdtSlamPCL<PointT>::getFinalPose()
{
    return convertToPose(ndt_.getFinalTransformation());
}


#endif
