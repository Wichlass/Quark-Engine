#pragma once

#include "../../headeronly/mathematics.h"
#include "frustum.h"

// ==================== PROJECTION TYPE ====================
enum class ProjectionType
{
    Perspective,
    Orthographic
};

// ==================== CAMERA ====================
struct Camera
{
    // Transform
    Quark::Vec3 position = Quark::Vec3::Zero();
    Quark::Quat rotation = Quark::Quat::Identity();

    // Projection
    ProjectionType projectionType = ProjectionType::Perspective;
    float fov = Quark::Radians(60.0f);
    float aspect = 16.0f / 9.0f;
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;
    float orthoHeight = 10.0f;

    // Cached Matrices
    Quark::Mat4 view = Quark::Mat4::Identity();
    Quark::Mat4 projection = Quark::Mat4::Identity();
    Quark::Mat4 viewProjection = Quark::Mat4::Identity();
    Quark::Mat4 invView = Quark::Mat4::Identity();
    Quark::Mat4 invProjection = Quark::Mat4::Identity();
    Quark::Mat4 invViewProjection = Quark::Mat4::Identity();

    // Frustum
    Frustum frustum;

    // Dirty Flags
    bool viewDirty = true;
    bool projDirty = true;

    // Direction Helpers
    Quark::Vec3 forward() const { return rotation * Quark::Vec3::Forward(); }
    Quark::Vec3 right()   const { return rotation * Quark::Vec3::Right(); }
    Quark::Vec3 up()      const { return rotation * Quark::Vec3::Up(); }
    Quark::Vec3 back()    const { return rotation * Quark::Vec3::Back(); }
    Quark::Vec3 left()    const { return rotation * Quark::Vec3::Left(); }
    Quark::Vec3 down()    const { return rotation * Quark::Vec3::Down(); }

    // Position & Rotation Setters
    void setPosition(const Quark::Vec3& pos)
    {
        position = pos;
        viewDirty = true;
    }

    void setRotation(const Quark::Quat& rot)
    {
        rotation = rot.Normalized();
        viewDirty = true;
    }

    void setEulerAngles(const Quark::Vec3& euler)
    {
        rotation = Quark::Quat::FromEuler(euler);
        viewDirty = true;
    }

    void lookAt(const Quark::Vec3& target, const Quark::Vec3& worldUp = Quark::Vec3::Up())
    {
        Quark::Vec3 dir = (target - position).Normalized();
        rotation = Quark::Quat::FromLookRotation(dir, worldUp);
        viewDirty = true;
    }

    // Movement
    void translate(const Quark::Vec3& delta)
    {
        position += delta;
        viewDirty = true;
    }

    void translateLocal(const Quark::Vec3& delta)
    {
        position += rotation * delta;
        viewDirty = true;
    }

    void rotate(const Quark::Quat& deltaRotation)
    {
        rotation = (rotation * deltaRotation).Normalized();
        viewDirty = true;
    }

    void rotateAxis(const Quark::Vec3& axis, float angle)
    {
        rotation = (rotation * Quark::Quat(axis, angle)).Normalized();
        viewDirty = true;
    }

    void rotateEuler(const Quark::Vec3& eulerDelta)
    {
        rotation = (rotation * Quark::Quat::FromEuler(eulerDelta)).Normalized();
        viewDirty = true;
    }

    // Projection Setters
    void setPerspective(float _fov, float _aspect, float _near, float _far)
    {
        projectionType = ProjectionType::Perspective;
        fov = _fov;
        aspect = _aspect;
        nearPlane = _near;
        farPlane = _far;
        projDirty = true;
    }

    void setOrthographic(float height, float _aspect, float _near, float _far)
    {
        projectionType = ProjectionType::Orthographic;
        orthoHeight = height;
        aspect = _aspect;
        nearPlane = _near;
        farPlane = _far;
        projDirty = true;
    }

    void setAspect(float _aspect)
    {
        aspect = _aspect;
        projDirty = true;
    }

    void setFOV(float _fov)
    {
        fov = _fov;
        projDirty = true;
    }

    void setClipPlanes(float _near, float _far)
    {
        nearPlane = _near;
        farPlane = _far;
        projDirty = true;
    }

    // Ray Casting
    Quark::Ray screenPointToRay(const Quark::Vec2& screenPoint, const Quark::Vec2& screenSize) const
    {
        float x = (2.0f * screenPoint.x) / screenSize.x - 1.0f;
        float y = 1.0f - (2.0f * screenPoint.y) / screenSize.y;

        Quark::Vec4 rayClip(x, y, -1.0f, 1.0f);
        Quark::Vec4 rayEye = invProjection * rayClip;
        rayEye.z = -1.0f;
        rayEye.w = 0.0f;

        Quark::Vec4 rayWorld = invView * rayEye;
        Quark::Vec3 rayDir(rayWorld.x, rayWorld.y, rayWorld.z);
        rayDir.Normalize();

        return Quark::Ray(position, rayDir);
    }

    Quark::Vec3 worldToScreenPoint(const Quark::Vec3& worldPoint, const Quark::Vec2& screenSize) const
    {
        Quark::Vec4 clipSpace = viewProjection * Quark::Vec4(worldPoint, 1.0f);

        if (std::abs(clipSpace.w) < Quark::EPSILON)
            return Quark::Vec3::Zero();

        Quark::Vec3 ndc(
            clipSpace.x / clipSpace.w,
            clipSpace.y / clipSpace.w,
            clipSpace.z / clipSpace.w
        );

        return Quark::Vec3(
            (ndc.x + 1.0f) * 0.5f * screenSize.x,
            (1.0f - ndc.y) * 0.5f * screenSize.y,
            ndc.z
        );
    }

    // Update Matrices
    void update()
    {
        if (viewDirty) updateView();
        if (projDirty) updateProjection();

        if (viewDirty || projDirty)
        {
            viewProjection = projection * view;
            invViewProjection = viewProjection.Inverted();
            frustum.extractFromViewProjection(viewProjection);
            viewDirty = false;
            projDirty = false;
        }
    }

    // GPU Data
    struct GPUData
    {
        Quark::Mat4 view;
        Quark::Mat4 projection;
        Quark::Mat4 viewProjection;
        Quark::Vec3 cameraPosition;
        float _pad;
    };

    GPUData getGPUData()
    {
        update();
        GPUData data{};
        data.view = view;
        data.projection = projection;
        data.viewProjection = viewProjection;
        data.cameraPosition = position;
        return data;
    }

    // Visibility Tests
    bool isVisible(const Quark::Sphere& sphere) const { return frustum.intersectsSphere(sphere); }
    bool isVisible(const Quark::AABB& aabb) const { return frustum.intersectsAABB(aabb); }
    bool isVisible(const Quark::Vec3& point) const { return frustum.containsPoint(point); }

private:
    void updateView()
    {
        view = Quark::Mat4::LookAt(position, position + forward(), up());
        invView = view.Inverted();
    }

    void updateProjection()
    {
        if (projectionType == ProjectionType::Perspective)
        {
            projection = Quark::Mat4::Perspective(fov, aspect, nearPlane, farPlane);
        }
        else
        {
            float halfH = orthoHeight * 0.5f;
            float halfW = halfH * aspect;
            projection = Quark::Mat4::Orthographic(-halfW, halfW, -halfH, halfH, nearPlane, farPlane);
        }
        invProjection = projection.Inverted();
    }
};