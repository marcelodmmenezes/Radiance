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
		float yaw = 0.0f,
		float pitch = 0.0f,
		glm::vec3 world_up = glm::vec3(0.0f, 1.0f, 0.0f));

	void move(CameraDirection direction, double delta_time);
	void look(float x_offset, float y_offset, double delta_time);
	void fast(bool is_fast);

	glm::mat4 getViewMatrix();

	glm::vec3 position;
	float yaw;
	float pitch;
	float move_speed;
	float look_speed;

	glm::vec3 new_position;
	float new_yaw;
	float new_pitch;

private:
	glm::vec3 world_up;

	glm::vec3 forward;
	glm::vec3 right;
	glm::vec3 up;

	float move_mix_factor;
	float look_mix_factor;
	float speed_multiplier;
};

#endif // FLY_THROUGH_CAMERA_HPP

