#pragma once

#include "../3rdparty/Eigen/Geometry"
#include "../3rdparty/Eigen/Core"

namespace mubone
{
    using Quaternion = Eigen::Quaternionf;
    using Matrix = Eigen::Matrix3f;
    using Vector = Eigen::Vector3f;
    using AngleAxis = Eigen::AngleAxis<float>;
}
