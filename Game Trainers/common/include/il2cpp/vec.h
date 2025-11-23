#pragma once

#define _USE_MATH_DEFINES
#include <math.h>
#include <iostream>

struct Vector2 {
	float x; float y;

	Vector2(float x_, float y_) {
		x = x_;
		y = y_;
	}
};

struct Vector3
{
public:
	Vector3() {};
	Vector3(float _x, float _y, float _z) {
		x = _x;
		y = _y;
		z = _z;
	};

	float Length() {
		return sqrt(x * x + y * y + z * z);
	}

	float Length2D() {
		return sqrt(x * x + y * y);
	}

	Vector2 DeltaAngle(Vector3 target) {
		float dx = target.x - x;
		float dy = target.z - z;
		float yaw = -atan2(dy, dx) * 180 / M_PI + 90;

		float distance = sqrt(dx * dx + dy * dy);
		float dz = target.y - y;
		float pitch = -atan2(dz, distance) * 180 / M_PI;

		return Vector2(pitch, yaw);
	}

	Vector3 operator+(Vector3 b) { return Vector3(x + b.x, y + b.y, z + b.z); }
	Vector3 operator-(Vector3 b) { return Vector3(x - b.x, y - b.y, z - b.z); }
	Vector3 operator-() { return Vector3(-x, -y, -z); }
	Vector3 operator*(float d) { return Vector3(x * d, y * d, z * d); }
	Vector3 operator/(float d) { return Vector3(x / d, y / d, z / d); }

	friend std::ostream& operator<<(std::ostream& os, const Vector3& v) {
		return os << "Vector3(" << v.x << ", " << v.y << ", " << v.z << ")";
	}

	float x, y, z;
};