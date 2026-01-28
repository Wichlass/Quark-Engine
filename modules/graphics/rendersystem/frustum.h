#pragma once

#include "../../headeronly/mathematics.h"

// ==================== FRUSTUM ====================
struct Frustum
{
    Quark::Plane planes[6];

    void extractFromViewProjection(const Quark::Mat4& vp)
    {
        auto extractAndNormalize = [&](int index, float a, float b, float c, float d)
        {
            float len = std::sqrt(a * a + b * b + c * c);
            if (len > Quark::EPSILON)
            {
                planes[index].normal = Quark::Vec3(a, b, c) / len;
                planes[index].distance = d / len;
            }
        };

        // Left plane (row3 + row0)
        extractAndNormalize(0, 
            vp.m[3] + vp.m[0], vp.m[7] + vp.m[4], vp.m[11] + vp.m[8], vp.m[15] + vp.m[12]);

        // Right plane (row3 - row0)
        extractAndNormalize(1,
            vp.m[3] - vp.m[0], vp.m[7] - vp.m[4], vp.m[11] - vp.m[8], vp.m[15] - vp.m[12]);

        // Bottom plane (row3 + row1)
        extractAndNormalize(2,
            vp.m[3] + vp.m[1], vp.m[7] + vp.m[5], vp.m[11] + vp.m[9], vp.m[15] + vp.m[13]);

        // Top plane (row3 - row1)
        extractAndNormalize(3,
            vp.m[3] - vp.m[1], vp.m[7] - vp.m[5], vp.m[11] - vp.m[9], vp.m[15] - vp.m[13]);

        // Near plane (row2 for z in [0,1] depth range)
        extractAndNormalize(4,
            vp.m[2], vp.m[6], vp.m[10], vp.m[14]);

        // Far plane (row3 - row2)
        extractAndNormalize(5,
            vp.m[3] - vp.m[2], vp.m[7] - vp.m[6], vp.m[11] - vp.m[10], vp.m[15] - vp.m[14]);
    }

    bool containsPoint(const Quark::Vec3& point) const
    {
        for (int i = 0; i < 6; i++)
        {
            if (planes[i].DistanceToPoint(point) < 0)
                return false;
        }
        return true;
    }

    bool intersectsSphere(const Quark::Sphere& sphere) const
    {
        for (int i = 0; i < 6; i++)
        {
            if (planes[i].DistanceToPoint(sphere.center) < -sphere.radius)
                return false;
        }
        return true;
    }

    bool intersectsAABB(const Quark::AABB& aabb) const
    {
        for (int i = 0; i < 6; i++)
        {
            Quark::Vec3 pVertex = aabb.minBounds;

            if (planes[i].normal.x >= 0) pVertex.x = aabb.maxBounds.x;
            if (planes[i].normal.y >= 0) pVertex.y = aabb.maxBounds.y;
            if (planes[i].normal.z >= 0) pVertex.z = aabb.maxBounds.z;

            if (planes[i].DistanceToPoint(pVertex) < 0)
                return false;
        }
        return true;
    }
};
