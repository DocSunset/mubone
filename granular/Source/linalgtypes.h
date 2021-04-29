#pragma once

#include "Eigen/Geometry"
#include "Eigen/Core"

namespace mubone
{
    using Quaternion = Eigen::Quaternionf;
    using Matrix = Eigen::Matrix3f;
    using Vector = Eigen::Vector3f;
    using AngleAxis = Eigen::AngleAxis<float>;
}
