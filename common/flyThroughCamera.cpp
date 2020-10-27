#include "flyThroughCamera.hpp"

FlyThroughCamera::FlyThroughCamera(
	glm::vec3 position,
	float yaw,
	float pitch,
	glm::vec3 world_up)
	:
	position { position },
	yaw { yaw },
	pitch { pitch },
	world_up { world_up },
	new_position { position },
	new_yaw { yaw },
	new_pitch { pitch } {

	move_speed = 5.0f;
	look_speed = 10.0f;
	move_mix_factor = 0.9f;
	look_mix_factor = 0.7f;
	speed_multiplier = 1.0f;
}

glm::mat4 FlyThroughCamera::getViewMatrix() {
	new_yaw = look_mix_factor * new_yaw + (1.0f - look_mix_factor) * yaw;
	new_pitch = look_mix_factor * new_pitch + (1.0f - look_mix_factor) * pitch;

	if (new_pitch > 89.0f)
		new_pitch = 89.0f;
	else if (new_pitch < -89.0f)
		new_pitch = -89.0f;

	forward =  {
		cos(glm::radians(new_pitch)) * -sin(glm::radians(new_yaw)),
		sin(glm::radians(new_pitch)),
		cos(glm::radians(new_pitch)) * -cos(glm::radians(new_yaw))
	};

	forward = glm::normalize(forward);
	right = glm::normalize(glm::cross(forward, world_up));
	up = glm::normalize(glm::cross(right, forward));

	new_position = glm::mix(position, new_position, move_mix_factor);

	return glm::lookAt(new_position, new_position + forward, up);
}

void FlyThroughCamera::move(CameraDirection direction, double delta_time) {
	float velocity = speed_multiplier * move_speed * delta_time;

	switch (direction) {
		case CameraDirection::FORWARD:
			position += forward * velocity;
			break;

		case CameraDirection::BACKWARD:
			position -= forward * velocity;
			break;

		case CameraDirection::LEFT:
			position -= right * velocity;
			break;

		case CameraDirection::RIGHT:
			position += right * velocity;
			break;

		case CameraDirection::UP:
			position += up * velocity;
			break;

		case CameraDirection::DOWN:
			position -= up * velocity;
			break;
	}
}

void FlyThroughCamera::look(float x_offset, float y_offset, double delta_time) {
	yaw += x_offset * look_speed * delta_time;
	pitch += y_offset * look_speed * delta_time;

	if (pitch > 89.0f)
		pitch = 89.0f;
	else if (pitch < -89.0f)
		pitch = -89.0f;
}

void FlyThroughCamera::fast(bool is_fast) {
	if (is_fast)
		speed_multiplier = 5.0f;
	else
		speed_multiplier = 1.0f;
}

