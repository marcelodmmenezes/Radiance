#ifndef FLY_THROUGH_CAMERA_HPP
#define FLY_THROUGH_CAMERA_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum class CameraDirection {
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT,
	UP,
	DOWN
};

class FlyThroughCamera {
public:
	FlyThroughCamera(
		glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3 forward = glm::vec3(0.0f, 0.0f, -1.0f),
		glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
		float yaw = 0.0f,
		float pitch = 0.0f,
		float move_speed = 5.0f,
		float look_speed = 10.0f,
		float move_mix_factor = 0.9f,
		float look_mix_factor = 0.8f);

	glm::vec3 getPosition() const;
	glm::vec3 getForward() const;
	glm::mat4 getViewMatrix();

	void setSpeed(float move_speed);
	float getSpeed() const;

	void move(CameraDirection direction, double delta_time);
	void look(float x_offset, float y_offset, double delta_time);

private:
	glm::vec3 position;
	glm::vec3 forward;
	glm::vec3 up;
	glm::vec3 right;

	glm::mat4 view_matrix;

	float yaw;
	float pitch;
	float move_speed;
	float look_speed;
	float move_mix_factor;
	float look_mix_factor;

	// Interpolation
	glm::vec3 new_position;
	float new_yaw;
	float new_pitch;
};

#endif // FLY_THROUGH_CAMERA_HPP

