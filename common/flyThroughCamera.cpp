#include "flyThroughCamera.hpp"

FlyThroughCamera::FlyThroughCamera(
	glm::vec3 position,
	glm::vec3 forward,
	glm::vec3 up,
	float yaw,
	float pitch,
	float move_speed,
	float look_speed,
	float move_mix_factor,
	float look_mix_factor)
	:
	position { position },
	forward { forward },
	up { up },
	yaw { yaw },
	pitch { pitch },
	move_speed { move_speed },
	look_speed { look_speed },
	move_mix_factor { move_mix_factor },
	look_mix_factor { look_mix_factor },
	new_position { position },
	new_yaw { yaw },
	new_pitch { pitch } {}

glm::vec3 FlyThroughCamera::getPosition() const {
	return position;
}

glm::vec3 FlyThroughCamera::getForward() const {
	return forward;
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
	right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
	up = glm::normalize(glm::cross(right, forward));

	new_position = glm::mix(position, new_position, move_mix_factor);

	return glm::lookAt(new_position, new_position + forward, up);
}

void FlyThroughCamera::setSpeed(float value) {
	move_speed = value;
}

float FlyThroughCamera::getSpeed() const {
	return move_speed;
}

void FlyThroughCamera::move(CameraDirection direction, double delta_time) {
	float velocity = move_speed * delta_time;

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

