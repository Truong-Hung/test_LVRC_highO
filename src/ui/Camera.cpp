#include "ui/Camera.hpp"

#include <iostream>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/transform.hpp>

Camera::Camera(float fov) :
	position(0),
	rotation(0, 0, 0, 1),
	viewMatrix(1),
	fieldOfView(fov),
	distance(0.1f)
{
	transformDirty = true;
}

Camera::Camera(Camera& other) :
	position(other.position),
	rotation(other.rotation),
	viewMatrix(other.viewMatrix),
	fieldOfView(other.fieldOfView),
	transformDirty(other.transformDirty),
	zNear(other.zNear),
	zFar(other.zFar),
	distance(other.distance)
{
}

const glm::mat4& Camera::getViewMatrix()
{
	if (transformDirty)
	{
		transformDirty = false;
		viewMatrix = translate(glm::mat4(1), glm::vec3(0, 0, -distance)) * mat4_cast(rotation) * translate(position);
	}
	return viewMatrix;
}

void Camera::setPosition(const glm::vec3& inPosition)
{
	if (position != inPosition)
	{
		position = inPosition;
		transformDirty = true;
		onCameraMoved.execute();
	}
}

void Camera::setRotation(const glm::quat& inRotation)
{
	if (rotation != inRotation)
	{
		rotation = inRotation;
		transformDirty = true;
		onCameraMoved.execute();
	}
}

void Camera::setFov(const float fov)
{
	fieldOfView = fov;
}

glm::vec3 Camera::forwardVector() const
{
	return rotation * glm::vec3(0, 0, 1) ;
}

glm::vec3 Camera::rightVector() const
{
	return rotation * glm::vec3(1, 0, 0);
}

glm::vec3 Camera::upVector() const
{
	return rotation * glm::vec3(0, 1, 0);
}

void Camera::setDistance(float newDistance)
{
	if (std::abs(distance - newDistance) > 0.00000f)
	{
		distance = newDistance;
		transformDirty = true;
		onCameraMoved.execute();
	}
}

CameraPose Camera::getPose(const std::string& name) const
{
	CameraPose pose;
	pose.name = name.empty() ? "Camera Angle" : name;
	pose.position = position;
	pose.rotation = rotation;
	pose.distance = distance;
	pose.fieldOfView = fieldOfView;
	return pose;
}

void Camera::setPose(const CameraPose& pose)
{
	position = pose.position;
	rotation = pose.rotation;
	distance = pose.distance;
	fieldOfView = pose.fieldOfView;
	transformDirty = true;
	onCameraMoved.execute();
}

std::string CameraPose::toString() const
{
	char buf[256];
	snprintf(buf, sizeof(buf), "%.6f,%.6f,%.6f|%.6f,%.6f,%.6f,%.6f|%.6f|%.6f",
	         position.x, position.y, position.z,
	         rotation.w, rotation.x, rotation.y, rotation.z,
	         distance, fieldOfView);
	return std::string(buf);
}

bool CameraPose::fromString(const std::string& str, CameraPose& outPose)
{
	float px = 0.f, py = 0.f, pz = 0.f;
	float rw = 1.f, rx = 0.f, ry = 0.f, rz = 0.f;
	float dist = 0.f, fov = 45.f;

	if (sscanf(str.c_str(), "%f,%f,%f|%f,%f,%f,%f|%f|%f",
	           &px, &py, &pz, &rw, &rx, &ry, &rz, &dist, &fov) == 9)
	{
		outPose.position = glm::vec3(px, py, pz);
		outPose.rotation = glm::quat(rw, rx, ry, rz);
		outPose.distance = dist;
		outPose.fieldOfView = fov;
		return true;
	}
	return false;
}

#include <fstream>
#include <sstream>

bool Camera::savePresetsToFile(const std::string& filepath, const std::vector<CameraPose>& presets)
{
	std::ofstream outFile(filepath);
	if (!outFile.is_open()) return false;

	for (const auto& pose : presets)
	{
		outFile << "Preset: " << pose.name << "\n";
		outFile << "Pos: " << pose.position.x << " " << pose.position.y << " " << pose.position.z << "\n";
		outFile << "Rot: " << pose.rotation.w << " " << pose.rotation.x << " " << pose.rotation.y << " " << pose.rotation.z << "\n";
		outFile << "Dist: " << pose.distance << "\n";
		outFile << "FOV: " << pose.fieldOfView << "\n";
		outFile << "---\n";
	}
	return true;
}

bool Camera::loadPresetsFromFile(const std::string& filepath, std::vector<CameraPose>& outPresets)
{
	std::ifstream inFile(filepath);
	if (!inFile.is_open()) return false;

	std::string line;
	CameraPose currentPose;
	bool hasData = false;
	std::vector<CameraPose> loadedPresets;

	while (std::getline(inFile, line))
	{
		if (line.rfind("Preset: ", 0) == 0)
		{
			if (hasData)
			{
				loadedPresets.push_back(currentPose);
				currentPose = CameraPose();
			}
			currentPose.name = line.substr(8);
			hasData = true;
		}
		else if (line.rfind("Pos: ", 0) == 0)
		{
			std::stringstream ss(line.substr(5));
			ss >> currentPose.position.x >> currentPose.position.y >> currentPose.position.z;
		}
		else if (line.rfind("Rot: ", 0) == 0)
		{
			std::stringstream ss(line.substr(5));
			ss >> currentPose.rotation.w >> currentPose.rotation.x >> currentPose.rotation.y >> currentPose.rotation.z;
		}
		else if (line.rfind("Dist: ", 0) == 0)
		{
			std::stringstream ss(line.substr(6));
			ss >> currentPose.distance;
		}
		else if (line.rfind("FOV: ", 0) == 0)
		{
			std::stringstream ss(line.substr(5));
			ss >> currentPose.fieldOfView;
		}
		else if (line.rfind("---", 0) == 0)
		{
			if (hasData)
			{
				loadedPresets.push_back(currentPose);
				currentPose = CameraPose();
				hasData = false;
			}
		}
	}
	if (hasData)
	{
		loadedPresets.push_back(currentPose);
	}

	if (!loadedPresets.empty())
	{
		outPresets = std::move(loadedPresets);
		return true;
	}
	return false;
}
