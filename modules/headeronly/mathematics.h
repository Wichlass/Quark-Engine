#pragma once

#include <cmath>
#include <algorithm>
#include <iostream>

namespace Quark {

    // Constants
    constexpr float PI = 3.14159265358979323846f;
    constexpr float TAU = 6.28318530717958647692f;
    constexpr float DEG2RAD = PI / 180.0f;
    constexpr float RAD2DEG = 180.0f / PI;
    constexpr float EPSILON = 1e-6f;

    // Internal min/max functions to avoid Windows macro conflicts
    template<typename T>
    inline constexpr T Min(const T& a, const T& b) {
        return (a < b) ? a : b;
    }

    template<typename T>
    inline constexpr T Max(const T& a, const T& b) {
        return (a > b) ? a : b;
    }

    // Utility Functions
    inline float Clamp(float v, float min, float max) {
        return Max(min, Min(v, max));
    }

    inline float Lerp(float a, float b, float t) {
        return a + (b - a) * t;
    }

    inline float Radians(float degrees) {
        return degrees * DEG2RAD;
    }

    inline float Degrees(float radians) {
        return radians * RAD2DEG;
    }

    // Vec2 - 2D Vector
    struct Vec2 {
        float x, y;

        Vec2() : x(0), y(0) {}
        Vec2(float v) : x(v), y(v) {}
        Vec2(float x, float y) : x(x), y(y) {}

        Vec2 operator+(const Vec2& v) const { return Vec2(x + v.x, y + v.y); }
        Vec2 operator-(const Vec2& v) const { return Vec2(x - v.x, y - v.y); }
        Vec2 operator*(float s) const { return Vec2(x * s, y * s); }
        Vec2 operator/(float s) const { return Vec2(x / s, y / s); }
        Vec2 operator-() const { return Vec2(-x, -y); }

        Vec2& operator+=(const Vec2& v) { x += v.x; y += v.y; return *this; }
        Vec2& operator-=(const Vec2& v) { x -= v.x; y -= v.y; return *this; }
        Vec2& operator*=(float s) { x *= s; y *= s; return *this; }
        Vec2& operator/=(float s) { x /= s; y /= s; return *this; }

        float Dot(const Vec2& v) const { return x * v.x + y * v.y; }
        float Length() const { return std::sqrt(x * x + y * y); }
        float LengthSq() const { return x * x + y * y; }

        Vec2 Normalized() const {
            float len = Length();
            return len > EPSILON ? Vec2(x / len, y / len) : Vec2(0, 0);
        }

        void Normalize() {
            float len = Length();
            if (len > EPSILON) { x /= len; y /= len; }
        }

        float Distance(const Vec2& v) const { return (*this - v).Length(); }

        static Vec2 Zero() { return Vec2(0, 0); }
        static Vec2 One() { return Vec2(1, 1); }
        static Vec2 UnitX() { return Vec2(1, 0); }
        static Vec2 UnitY() { return Vec2(0, 1); }
    };

    // Vec3 - 3D Vector
    struct Vec3 {
        float x, y, z;

        Vec3() : x(0), y(0), z(0) {}
        Vec3(float v) : x(v), y(v), z(v) {}
        Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

        Vec3 operator+(const Vec3& v) const { return Vec3(x + v.x, y + v.y, z + v.z); }
        Vec3 operator-(const Vec3& v) const { return Vec3(x - v.x, y - v.y, z - v.z); }
        Vec3 operator*(float s) const { return Vec3(x * s, y * s, z * s); }
        Vec3 operator*(const Vec3& v) const { return Vec3(x * v.x, y * v.y, z * v.z); }
        Vec3 operator/(float s) const { return Vec3(x / s, y / s, z / s); }
        Vec3 operator-() const { return Vec3(-x, -y, -z); }

        Vec3& operator+=(const Vec3& v) { x += v.x; y += v.y; z += v.z; return *this; }
        Vec3& operator-=(const Vec3& v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
        Vec3& operator*=(float s) { x *= s; y *= s; z *= s; return *this; }
        Vec3& operator*=(const Vec3& v) { x *= v.x; y *= v.y; z *= v.z; return *this; }
        Vec3& operator/=(float s) { x /= s; y /= s; z /= s; return *this; }

        float Dot(const Vec3& v) const { return x * v.x + y * v.y + z * v.z; }

        Vec3 Cross(const Vec3& v) const {
            return Vec3(
                y * v.z - z * v.y,
                z * v.x - x * v.z,
                x * v.y - y * v.x
            );
        }

        float Length() const { return std::sqrt(x * x + y * y + z * z); }
        float LengthSq() const { return x * x + y * y + z * z; }

        Vec3 Normalized() const {
            float len = Length();
            return len > EPSILON ? Vec3(x / len, y / len, z / len) : Vec3(0, 0, 0);
        }

        void Normalize() {
            float len = Length();
            if (len > EPSILON) { x /= len; y /= len; z /= len; }
        }

        float Distance(const Vec3& v) const { return (*this - v).Length(); }

        static Vec3 Zero() { return Vec3(0, 0, 0); }
        static Vec3 One() { return Vec3(1, 1, 1); }
        static Vec3 UnitX() { return Vec3(1, 0, 0); }
        static Vec3 UnitY() { return Vec3(0, 1, 0); }
        static Vec3 UnitZ() { return Vec3(0, 0, 1); }
        static Vec3 Up() { return Vec3(0, 1, 0); }
        static Vec3 Down() { return Vec3(0, -1, 0); }
        static Vec3 Forward() { return Vec3(0, 0, 1); }
        static Vec3 Back() { return Vec3(0, 0, -1); }
        static Vec3 Right() { return Vec3(1, 0, 0); }
        static Vec3 Left() { return Vec3(-1, 0, 0); }
    };

    // Vec4 - 4D Vector
    struct Vec4 {
        float x, y, z, w;

        Vec4() : x(0), y(0), z(0), w(0) {}
        Vec4(float v) : x(v), y(v), z(v), w(v) {}
        Vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
        Vec4(const Vec3& v, float w) : x(v.x), y(v.y), z(v.z), w(w) {}

        Vec4 operator+(const Vec4& v) const { return Vec4(x + v.x, y + v.y, z + v.z, w + v.w); }
        Vec4 operator-(const Vec4& v) const { return Vec4(x - v.x, y - v.y, z - v.z, w - v.w); }
        Vec4 operator*(float s) const { return Vec4(x * s, y * s, z * s, w * s); }
        Vec4 operator/(float s) const { return Vec4(x / s, y / s, z / s, w / s); }
        Vec4 operator-() const { return Vec4(-x, -y, -z, -w); }

        Vec4& operator+=(const Vec4& v) { x += v.x; y += v.y; z += v.z; w += v.w; return *this; }
        Vec4& operator-=(const Vec4& v) { x -= v.x; y -= v.y; z -= v.z; w -= v.w; return *this; }
        Vec4& operator*=(float s) { x *= s; y *= s; z *= s; w *= s; return *this; }
        Vec4& operator/=(float s) { x /= s; y /= s; z /= s; w /= s; return *this; }

        float Dot(const Vec4& v) const { return x * v.x + y * v.y + z * v.z + w * v.w; }
        float Length() const { return std::sqrt(x * x + y * y + z * z + w * w); }
        float LengthSq() const { return x * x + y * y + z * z + w * w; }

        Vec4 Normalized() const {
            float len = Length();
            return len > EPSILON ? Vec4(x / len, y / len, z / len, w / len) : Vec4(0, 0, 0, 0);
        }

        void Normalize() {
            float len = Length();
            if (len > EPSILON) { x /= len; y /= len; z /= len; w /= len; }
        }

        Vec3 xyz() const { return Vec3(x, y, z); }
    };

    // Color - RGBA Color
    struct Color {
        float r, g, b, a;

        Color() : r(0), g(0), b(0), a(1) {}
        Color(float r, float g, float b, float a = 1.0f) : r(r), g(g), b(b), a(a) {}
        Color(const Vec3& v, float a = 1.0f) : r(v.x), g(v.y), b(v.z), a(a) {}
        Color(const Vec4& v) : r(v.x), g(v.y), b(v.z), a(v.w) {}

        Color operator+(const Color& c) const { return Color(r + c.r, g + c.g, b + c.b, a + c.a); }
        Color operator-(const Color& c) const { return Color(r - c.r, g - c.g, b - c.b, a - c.a); }
        Color operator*(float s) const { return Color(r * s, g * s, b * s, a * s); }
        Color operator*(const Color& c) const { return Color(r * c.r, g * c.g, b * c.b, a * c.a); }

        Color& operator+=(const Color& c) { r += c.r; g += c.g; b += c.b; a += c.a; return *this; }
        Color& operator*=(float s) { r *= s; g *= s; b *= s; a *= s; return *this; }

        Color Clamped() const {
            return Color(
                Clamp(r, 0.0f, 1.0f),
                Clamp(g, 0.0f, 1.0f),
                Clamp(b, 0.0f, 1.0f),
                Clamp(a, 0.0f, 1.0f)
            );
        }

        Vec4 ToVec4() const { return Vec4(r, g, b, a); }

        static Color White() { return Color(1, 1, 1, 1); }
        static Color Black() { return Color(0, 0, 0, 1); }
        static Color Red() { return Color(1, 0, 0, 1); }
        static Color Green() { return Color(0, 1, 0, 1); }
        static Color Blue() { return Color(0, 0, 1, 1); }
        static Color Yellow() { return Color(1, 1, 0, 1); }
        static Color Cyan() { return Color(0, 1, 1, 1); }
        static Color Magenta() { return Color(1, 0, 1, 1); }
        static Color Transparent() { return Color(0, 0, 0, 0); }
    };

    // Mat4 - 4x4 Matrix (Column-major)
    struct Mat4 {
        float m[16];

        Mat4() {
            for (int i = 0; i < 16; i++) m[i] = 0;
            m[0] = m[5] = m[10] = m[15] = 1.0f;
        }

        Mat4(float diagonal) {
            for (int i = 0; i < 16; i++) m[i] = 0;
            m[0] = m[5] = m[10] = m[15] = diagonal;
        }

        float& operator[](int i) { return m[i]; }
        const float& operator[](int i) const { return m[i]; }

        Mat4 operator*(const Mat4& mat) const {
            Mat4 result;
            for (int i = 0; i < 4; i++) {
                for (int j = 0; j < 4; j++) {
                    result.m[i * 4 + j] = 0;
                    for (int k = 0; k < 4; k++) {
                        result.m[i * 4 + j] += m[k * 4 + j] * mat.m[i * 4 + k];
                    }
                }
            }
            return result;
        }

        Vec4 operator*(const Vec4& v) const {
            return Vec4(
                m[0] * v.x + m[4] * v.y + m[8] * v.z + m[12] * v.w,
                m[1] * v.x + m[5] * v.y + m[9] * v.z + m[13] * v.w,
                m[2] * v.x + m[6] * v.y + m[10] * v.z + m[14] * v.w,
                m[3] * v.x + m[7] * v.y + m[11] * v.z + m[15] * v.w
            );
        }

        Vec3 TransformPoint(const Vec3& v) const {
            Vec4 result = (*this) * Vec4(v, 1.0f);
            return Vec3(result.x / result.w, result.y / result.w, result.z / result.w);
        }

        Vec3 TransformDirection(const Vec3& v) const {
            return Vec3(
                m[0] * v.x + m[4] * v.y + m[8] * v.z,
                m[1] * v.x + m[5] * v.y + m[9] * v.z,
                m[2] * v.x + m[6] * v.y + m[10] * v.z
            );
        }

        Mat4 Transposed() const {
            Mat4 result;
            for (int i = 0; i < 4; i++) {
                for (int j = 0; j < 4; j++) {
                    result.m[i * 4 + j] = m[j * 4 + i];
                }
            }
            return result;
        }

        float Determinant() const {
            float a00 = m[0], a01 = m[1], a02 = m[2], a03 = m[3];
            float a10 = m[4], a11 = m[5], a12 = m[6], a13 = m[7];
            float a20 = m[8], a21 = m[9], a22 = m[10], a23 = m[11];
            float a30 = m[12], a31 = m[13], a32 = m[14], a33 = m[15];

            float b00 = a00 * a11 - a01 * a10;
            float b01 = a00 * a12 - a02 * a10;
            float b02 = a00 * a13 - a03 * a10;
            float b03 = a01 * a12 - a02 * a11;
            float b04 = a01 * a13 - a03 * a11;
            float b05 = a02 * a13 - a03 * a12;
            float b06 = a20 * a31 - a21 * a30;
            float b07 = a20 * a32 - a22 * a30;
            float b08 = a20 * a33 - a23 * a30;
            float b09 = a21 * a32 - a22 * a31;
            float b10 = a21 * a33 - a23 * a31;
            float b11 = a22 * a33 - a23 * a32;

            return b00 * b11 - b01 * b10 + b02 * b09 + b03 * b08 - b04 * b07 + b05 * b06;
        }

        Mat4 Inverted() const {
            float a00 = m[0], a01 = m[1], a02 = m[2], a03 = m[3];
            float a10 = m[4], a11 = m[5], a12 = m[6], a13 = m[7];
            float a20 = m[8], a21 = m[9], a22 = m[10], a23 = m[11];
            float a30 = m[12], a31 = m[13], a32 = m[14], a33 = m[15];

            float b00 = a00 * a11 - a01 * a10;
            float b01 = a00 * a12 - a02 * a10;
            float b02 = a00 * a13 - a03 * a10;
            float b03 = a01 * a12 - a02 * a11;
            float b04 = a01 * a13 - a03 * a11;
            float b05 = a02 * a13 - a03 * a12;
            float b06 = a20 * a31 - a21 * a30;
            float b07 = a20 * a32 - a22 * a30;
            float b08 = a20 * a33 - a23 * a30;
            float b09 = a21 * a32 - a22 * a31;
            float b10 = a21 * a33 - a23 * a31;
            float b11 = a22 * a33 - a23 * a32;

            float det = b00 * b11 - b01 * b10 + b02 * b09 + b03 * b08 - b04 * b07 + b05 * b06;

            if (std::abs(det) < EPSILON) {
                return Mat4::Identity();
            }

            float invDet = 1.0f / det;

            Mat4 result;
            result.m[0] = (a11 * b11 - a12 * b10 + a13 * b09) * invDet;
            result.m[1] = (a02 * b10 - a01 * b11 - a03 * b09) * invDet;
            result.m[2] = (a31 * b05 - a32 * b04 + a33 * b03) * invDet;
            result.m[3] = (a22 * b04 - a21 * b05 - a23 * b03) * invDet;
            result.m[4] = (a12 * b08 - a10 * b11 - a13 * b07) * invDet;
            result.m[5] = (a00 * b11 - a02 * b08 + a03 * b07) * invDet;
            result.m[6] = (a32 * b02 - a30 * b05 - a33 * b01) * invDet;
            result.m[7] = (a20 * b05 - a22 * b02 + a23 * b01) * invDet;
            result.m[8] = (a10 * b10 - a11 * b08 + a13 * b06) * invDet;
            result.m[9] = (a01 * b08 - a00 * b10 - a03 * b06) * invDet;
            result.m[10] = (a30 * b04 - a31 * b02 + a33 * b00) * invDet;
            result.m[11] = (a21 * b02 - a20 * b04 - a23 * b00) * invDet;
            result.m[12] = (a11 * b07 - a10 * b09 - a12 * b06) * invDet;
            result.m[13] = (a00 * b09 - a01 * b07 + a02 * b06) * invDet;
            result.m[14] = (a31 * b01 - a30 * b03 - a32 * b00) * invDet;
            result.m[15] = (a20 * b03 - a21 * b01 + a22 * b00) * invDet;

            return result;
        }

        void Inverse() {
            *this = Inverted();
        }

        static Mat4 Identity() { return Mat4(); }

        static Mat4 Translation(const Vec3& v) {
            Mat4 result;
            result.m[12] = v.x;
            result.m[13] = v.y;
            result.m[14] = v.z;
            return result;
        }

        static Mat4 Scaling(const Vec3& v) {
            Mat4 result;
            result.m[0] = v.x;
            result.m[5] = v.y;
            result.m[10] = v.z;
            return result;
        }

        static Mat4 RotationX(float angle) {
            Mat4 result;
            float c = std::cos(angle);
            float s = std::sin(angle);
            result.m[5] = c;
            result.m[6] = s;
            result.m[9] = -s;
            result.m[10] = c;
            return result;
        }

        static Mat4 RotationY(float angle) {
            Mat4 result;
            float c = std::cos(angle);
            float s = std::sin(angle);
            result.m[0] = c;
            result.m[2] = -s;
            result.m[8] = s;
            result.m[10] = c;
            return result;
        }

        static Mat4 RotationZ(float angle) {
            Mat4 result;
            float c = std::cos(angle);
            float s = std::sin(angle);
            result.m[0] = c;
            result.m[1] = s;
            result.m[4] = -s;
            result.m[5] = c;
            return result;
        }

        static Mat4 Rotation(const Vec3& axis, float angle) {
            Mat4 result;
            Vec3 a = axis.Normalized();
            float c = std::cos(angle);
            float s = std::sin(angle);
            float t = 1.0f - c;

            result.m[0] = a.x * a.x * t + c;
            result.m[1] = a.y * a.x * t + a.z * s;
            result.m[2] = a.z * a.x * t - a.y * s;
            result.m[4] = a.x * a.y * t - a.z * s;
            result.m[5] = a.y * a.y * t + c;
            result.m[6] = a.z * a.y * t + a.x * s;
            result.m[8] = a.x * a.z * t + a.y * s;
            result.m[9] = a.y * a.z * t - a.x * s;
            result.m[10] = a.z * a.z * t + c;

            return result;
        }

        static Mat4 LookAt(const Vec3& eye, const Vec3& target, const Vec3& up) {
            Vec3 z = (eye - target).Normalized();
            Vec3 x = up.Cross(z).Normalized();
            Vec3 y = z.Cross(x);

            Mat4 result;
            result.m[0] = x.x;
            result.m[4] = x.y;
            result.m[8] = x.z;
            result.m[12] = -x.Dot(eye);
            result.m[1] = y.x;
            result.m[5] = y.y;
            result.m[9] = y.z;
            result.m[13] = -y.Dot(eye);
            result.m[2] = z.x;
            result.m[6] = z.y;
            result.m[10] = z.z;
            result.m[14] = -z.Dot(eye);
            result.m[3] = 0;
            result.m[7] = 0;
            result.m[11] = 0;
            result.m[15] = 1;

            return result;
        }

        static Mat4 Perspective(float fov, float aspect, float nearPlane, float farPlane) {
            Mat4 result(0);
            float tanHalfFov = std::tan(fov / 2.0f);

            result.m[0] = 1.0f / (aspect * tanHalfFov);
            result.m[5] = 1.0f / tanHalfFov;
            result.m[10] = -(farPlane + nearPlane) / (farPlane - nearPlane);
            result.m[11] = -1.0f;
            result.m[14] = -(2.0f * farPlane * nearPlane) / (farPlane - nearPlane);

            return result;
        }

        static Mat4 Orthographic(float left, float right, float bottom, float top, float nearPlane, float farPlane) {
            Mat4 result;
            result.m[0] = 2.0f / (right - left);
            result.m[5] = 2.0f / (top - bottom);
            result.m[10] = -2.0f / (farPlane - nearPlane);
            result.m[12] = -(right + left) / (right - left);
            result.m[13] = -(top + bottom) / (top - bottom);
            result.m[14] = -(farPlane + nearPlane) / (farPlane - nearPlane);
            return result;
        }
    };

    // Quaternion - Rotation Quat
    struct Quat {
        float x, y, z, w;

        Quat() : x(0), y(0), z(0), w(1) {}
        Quat(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
        Quat(const Vec3& axis, float angle) {
            float halfAngle = angle * 0.5f;
            float s = std::sin(halfAngle);
            Vec3 a = axis.Normalized();
            x = a.x * s;
            y = a.y * s;
            z = a.z * s;
            w = std::cos(halfAngle);
        }

        Quat operator*(const Quat& q) const {
            return Quat(
                w * q.x + x * q.w + y * q.z - z * q.y,
                w * q.y + y * q.w + z * q.x - x * q.z,
                w * q.z + z * q.w + x * q.y - y * q.x,
                w * q.w - x * q.x - y * q.y - z * q.z
            );
        }

        Vec3 operator*(const Vec3& v) const {
            Vec3 u(x, y, z);
            float s = w;
            return u * (2.0f * u.Dot(v)) + v * (s * s - u.Dot(u)) + u.Cross(v) * (2.0f * s);
        }

        Quat operator*(float s) const { return Quat(x * s, y * s, z * s, w * s); }
        Quat operator+(const Quat& q) const { return Quat(x + q.x, y + q.y, z + q.z, w + q.w); }

        float Dot(const Quat& q) const { return x * q.x + y * q.y + z * q.z + w * q.w; }
        float Length() const { return std::sqrt(x * x + y * y + z * z + w * w); }
        float LengthSq() const { return x * x + y * y + z * z + w * w; }

        Quat Normalized() const {
            float len = Length();
            return len > EPSILON ? Quat(x / len, y / len, z / len, w / len) : Quat();
        }

        void Normalize() {
            float len = Length();
            if (len > EPSILON) { x /= len; y /= len; z /= len; w /= len; }
        }

        Quat Conjugate() const { return Quat(-x, -y, -z, w); }

        Quat Inverse() const {
            float lenSq = LengthSq();
            if (lenSq > EPSILON) {
                float invLenSq = 1.0f / lenSq;
                return Quat(-x * invLenSq, -y * invLenSq, -z * invLenSq, w * invLenSq);
            }
            return Quat();
        }

        Mat4 ToMatrix() const {
            Mat4 result;
            float xx = x * x, yy = y * y, zz = z * z;
            float xy = x * y, xz = x * z, yz = y * z;
            float wx = w * x, wy = w * y, wz = w * z;

            result.m[0] = 1.0f - 2.0f * (yy + zz);
            result.m[1] = 2.0f * (xy + wz);
            result.m[2] = 2.0f * (xz - wy);
            result.m[4] = 2.0f * (xy - wz);
            result.m[5] = 1.0f - 2.0f * (xx + zz);
            result.m[6] = 2.0f * (yz + wx);
            result.m[8] = 2.0f * (xz + wy);
            result.m[9] = 2.0f * (yz - wx);
            result.m[10] = 1.0f - 2.0f * (xx + yy);

            return result;
        }

        Vec3 ToEuler() const {
            Vec3 euler;

            float sinr_cosp = 2.0f * (w * x + y * z);
            float cosr_cosp = 1.0f - 2.0f * (x * x + y * y);
            euler.x = std::atan2(sinr_cosp, cosr_cosp);

            float sinp = 2.0f * (w * y - z * x);
            if (std::abs(sinp) >= 1)
                euler.y = std::copysign(PI / 2.0f, sinp);
            else
                euler.y = std::asin(sinp);

            float siny_cosp = 2.0f * (w * z + x * y);
            float cosy_cosp = 1.0f - 2.0f * (y * y + z * z);
            euler.z = std::atan2(siny_cosp, cosy_cosp);

            return euler;
        }

        static Quat Identity() { return Quat(0, 0, 0, 1); }

        static Quat FromEuler(const Vec3& euler) {
            float cx = std::cos(euler.x * 0.5f);
            float sx = std::sin(euler.x * 0.5f);
            float cy = std::cos(euler.y * 0.5f);
            float sy = std::sin(euler.y * 0.5f);
            float cz = std::cos(euler.z * 0.5f);
            float sz = std::sin(euler.z * 0.5f);

            Quat q;
            q.w = cx * cy * cz + sx * sy * sz;
            q.x = sx * cy * cz - cx * sy * sz;
            q.y = cx * sy * cz + sx * cy * sz;
            q.z = cx * cy * sz - sx * sy * cz;

            return q;
        }

        static Quat Slerp(const Quat& a, const Quat& b, float t) {
            Quat qa = a.Normalized();
            Quat qb = b.Normalized();

            float dot = qa.Dot(qb);
            if (dot < 0.0f) {
                qb = qb * -1.0f;
                dot = -dot;
            }

            if (dot > 0.9995f) {
                return (qa + (qb + qa * -1.0f) * t).Normalized();
            }

            float theta = std::acos(dot);
            float sinTheta = std::sin(theta);
            float wa = std::sin((1.0f - t) * theta) / sinTheta;
            float wb = std::sin(t * theta) / sinTheta;

            return qa * wa + qb * wb;
        }

        // Create quaternion from forward and up directions (look rotation)
        static Quat FromLookRotation(const Vec3& forward, const Vec3& up = Vec3::Up()) {
            Vec3 f = forward.Normalized();
            Vec3 r = up.Cross(f).Normalized();
            Vec3 u = f.Cross(r);

            // Build rotation matrix and convert to quaternion
            float m00 = r.x, m01 = r.y, m02 = r.z;
            float m10 = u.x, m11 = u.y, m12 = u.z;
            float m20 = f.x, m21 = f.y, m22 = f.z;

            float trace = m00 + m11 + m22;
            Quat q;

            if (trace > 0) {
                float s = 0.5f / std::sqrt(trace + 1.0f);
                q.w = 0.25f / s;
                q.x = (m12 - m21) * s;
                q.y = (m20 - m02) * s;
                q.z = (m01 - m10) * s;
            }
            else if (m00 > m11 && m00 > m22) {
                float s = 2.0f * std::sqrt(1.0f + m00 - m11 - m22);
                q.w = (m12 - m21) / s;
                q.x = 0.25f * s;
                q.y = (m10 + m01) / s;
                q.z = (m20 + m02) / s;
            }
            else if (m11 > m22) {
                float s = 2.0f * std::sqrt(1.0f + m11 - m00 - m22);
                q.w = (m20 - m02) / s;
                q.x = (m10 + m01) / s;
                q.y = 0.25f * s;
                q.z = (m21 + m12) / s;
            }
            else {
                float s = 2.0f * std::sqrt(1.0f + m22 - m00 - m11);
                q.w = (m01 - m10) / s;
                q.x = (m20 + m02) / s;
                q.y = (m21 + m12) / s;
                q.z = 0.25f * s;
            }

            return q.Normalized();
        }
    };

    // Transform - Position, Rotation, Scale
    struct Transform {
        Vec3 position;
        Quat rotation;
        Vec3 scale;

        Transform() : position(Vec3::Zero()), rotation(Quat::Identity()), scale(Vec3::One()) {}
        Transform(const Vec3& pos, const Quat& rot, const Vec3& scl)
            : position(pos), rotation(rot), scale(scl) {
        }

        Mat4 ToMatrix() const {
            return Mat4::Translation(position) * rotation.ToMatrix() * Mat4::Scaling(scale);
        }

        Vec3 TransformPoint(const Vec3& point) const {
            return rotation * (point * scale) + position;
        }

        Vec3 TransformDirection(const Vec3& dir) const {
            return rotation * dir;
        }

        Vec3 Forward() const { return rotation * Vec3::Forward(); }
        Vec3 Right() const { return rotation * Vec3::Right(); }
        Vec3 Up() const { return rotation * Vec3::Up(); }

        static Transform Identity() { return Transform(); }
    };

    // AABB - Axis-Aligned Bounding Box
    struct AABB {
        Vec3 minBounds, maxBounds;

        AABB() : minBounds(Vec3::Zero()), maxBounds(Vec3::Zero()) {}
        AABB(const Vec3& minBounds, const Vec3& maxBounds) : minBounds(minBounds), maxBounds(maxBounds) {}

        Vec3 Center() const { return (minBounds + maxBounds) * 0.5f; }
        Vec3 Size() const { return maxBounds - minBounds; }
        Vec3 Extents() const { return (maxBounds - minBounds) * 0.5f; }

        bool Contains(const Vec3& point) const {
            return point.x >= minBounds.x && point.x <= maxBounds.x &&
                point.y >= minBounds.y && point.y <= maxBounds.y &&
                point.z >= minBounds.z && point.z <= maxBounds.z;
        }

        bool Intersects(const AABB& other) const {
            return (minBounds.x <= other.maxBounds.x && maxBounds.x >= other.minBounds.x) &&
                (minBounds.y <= other.maxBounds.y && maxBounds.y >= other.minBounds.y) &&
                (minBounds.z <= other.maxBounds.z && maxBounds.z >= other.minBounds.z);
        }

        AABB Merge(const AABB& other) const {
            return AABB(
                Vec3(Min(minBounds.x, other.minBounds.x), Min(minBounds.y, other.minBounds.y), Min(minBounds.z, other.minBounds.z)),
                Vec3(Max(maxBounds.x, other.maxBounds.x), Max(maxBounds.y, other.maxBounds.y), Max(maxBounds.z, other.maxBounds.z))
            );
        }

        void Expand(const Vec3& point) {
            minBounds.x = Min(minBounds.x, point.x);
            minBounds.y = Min(minBounds.y, point.y);
            minBounds.z = Min(minBounds.z, point.z);
            maxBounds.x = Max(maxBounds.x, point.x);
            maxBounds.y = Max(maxBounds.y, point.y);
            maxBounds.z = Max(maxBounds.z, point.z);
        }
    };

    // Ray - 3D Ray
    struct Ray {
        Vec3 origin;
        Vec3 direction;

        Ray() : origin(Vec3::Zero()), direction(Vec3::Forward()) {}
        Ray(const Vec3& origin, const Vec3& direction)
            : origin(origin), direction(direction.Normalized()) {
        }

        Vec3 PointAt(float t) const { return origin + direction * t; }

        bool IntersectAABB(const AABB& aabb, float& tMin, float& tMax) const {
            tMin = 0.0f;
            tMax = INFINITY;

            for (int i = 0; i < 3; i++) {
                float o = ((float*)&origin)[i];
                float d = ((float*)&direction)[i];
                float minVal = ((float*)&aabb.minBounds)[i];
                float maxVal = ((float*)&aabb.maxBounds)[i];

                if (std::abs(d) < EPSILON) {
                    if (o < minVal || o > maxVal) return false;
                }
                else {
                    float t1 = (minVal - o) / d;
                    float t2 = (maxVal - o) / d;
                    if (t1 > t2) {
                        float temp = t1;
                        t1 = t2;
                        t2 = temp;
                    }
                    tMin = Max(tMin, t1);
                    tMax = Min(tMax, t2);
                    if (tMin > tMax) return false;
                }
            }
            return true;
        }

        bool IntersectSphere(const Vec3& center, float radius, float& t) const {
            Vec3 oc = origin - center;
            float a = direction.Dot(direction);
            float b = 2.0f * oc.Dot(direction);
            float c = oc.Dot(oc) - radius * radius;
            float discriminant = b * b - 4 * a * c;

            if (discriminant < 0) return false;

            t = (-b - std::sqrt(discriminant)) / (2.0f * a);
            if (t < 0) {
                t = (-b + std::sqrt(discriminant)) / (2.0f * a);
                if (t < 0) return false;
            }
            return true;
        }

        bool IntersectPlane(const Vec3& planeNormal, const Vec3& planePoint, float& t) const {
            float denom = direction.Dot(planeNormal);
            if (std::abs(denom) > EPSILON) {
                Vec3 p0l0 = planePoint - origin;
                t = p0l0.Dot(planeNormal) / denom;
                return t >= 0;
            }
            return false;
        }
    };

    // Plane - 3D Plane
    struct Plane {
        Vec3 normal;
        float distance;

        Plane() : normal(Vec3::Up()), distance(0) {}
        Plane(const Vec3& normal, float distance) : normal(normal.Normalized()), distance(distance) {}
        Plane(const Vec3& normal, const Vec3& point) : normal(normal.Normalized()) {
            distance = -normal.Dot(point);
        }
        Plane(const Vec3& p1, const Vec3& p2, const Vec3& p3) {
            normal = (p2 - p1).Cross(p3 - p1).Normalized();
            distance = -normal.Dot(p1);
        }

        float DistanceToPoint(const Vec3& point) const {
            return normal.Dot(point) + distance;
        }

        Vec3 ClosestPoint(const Vec3& point) const {
            return point - normal * DistanceToPoint(point);
        }

        bool IsOnPlane(const Vec3& point) const {
            return std::abs(DistanceToPoint(point)) < EPSILON;
        }
    };

    // Sphere - 3D Sphere
    struct Sphere {
        Vec3 center;
        float radius;

        Sphere() : center(Vec3::Zero()), radius(1.0f) {}
        Sphere(const Vec3& center, float radius) : center(center), radius(radius) {}

        bool Contains(const Vec3& point) const {
            return center.Distance(point) <= radius;
        }

        bool Intersects(const Sphere& other) const {
            float dist = center.Distance(other.center);
            return dist <= (radius + other.radius);
        }

        bool Intersects(const AABB& aabb) const {
            Vec3 closest = Vec3(
                Clamp(center.x, aabb.minBounds.x, aabb.maxBounds.x),
                Clamp(center.y, aabb.minBounds.y, aabb.maxBounds.y),
                Clamp(center.z, aabb.minBounds.z, aabb.maxBounds.z)
            );
            return center.Distance(closest) <= radius;
        }
    };

    // Additional Math Functions
    inline float SmoothStep(float edge0, float edge1, float x) {
        float t = Clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }

    inline Vec3 Reflect(const Vec3& incident, const Vec3& normal) {
        return incident - normal * (2.0f * incident.Dot(normal));
    }

    inline Vec3 Refract(const Vec3& incident, const Vec3& normal, float eta) {
        float k = 1.0f - eta * eta * (1.0f - incident.Dot(normal) * incident.Dot(normal));
        if (k < 0.0f) return Vec3::Zero();
        return incident * eta - normal * (eta * incident.Dot(normal) + std::sqrt(k));
    }

    inline Vec3 ProjectOntoPlane(const Vec3& vector, const Vec3& planeNormal) {
        return vector - planeNormal * vector.Dot(planeNormal);
    }

    inline float Angle(const Vec3& from, const Vec3& to) {
        float denominator = std::sqrt(from.LengthSq() * to.LengthSq());
        if (denominator < EPSILON) return 0.0f;
        float dot = Clamp(from.Dot(to) / denominator, -1.0f, 1.0f);
        return std::acos(dot);
    }

    inline float SignedAngle(const Vec3& from, const Vec3& to, const Vec3& axis) {
        float unsignedAngle = Angle(from, to);
        float sign = (from.Cross(to)).Dot(axis);
        return sign >= 0 ? unsignedAngle : -unsignedAngle;
    }

    inline Vec3 Slerp(const Vec3& start, const Vec3& end, float t) {
        float dot = Clamp(start.Dot(end), -1.0f, 1.0f);
        float theta = std::acos(dot) * t;
        Vec3 relative = (end - start * dot).Normalized();
        return start * std::cos(theta) + relative * std::sin(theta);
    }

    inline Vec3 MinVec(const Vec3& a, const Vec3& b) {
        return Vec3(Min(a.x, b.x), Min(a.y, b.y), Min(a.z, b.z));
    }

    inline Vec3 MaxVec(const Vec3& a, const Vec3& b) {
        return Vec3(Max(a.x, b.x), Max(a.y, b.y), Max(a.z, b.z));
    }

}