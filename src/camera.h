#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <iostream>
#include <glm/gtx/euler_angles.hpp>
using WindowHandler = GLFWwindow;

class Camera {
	glm::vec3 position;
	glm::vec2 mouse_delta{ 0 };
	glm::vec2 prev_mouse_position{ 0 };
	glm::vec2 angle{ 0 };
	bool init_mouse = false;

public:
	Camera(glm::vec3 position) { this->position = position; }

	//returns true if camera was moved
	bool onUpdate(WindowHandler* window, float dt) {

		glm::vec4 offset{ 0.0f };
		float speed = 1.0f;
		bool moved = false; // Tracks if the camera moved

		// Move forward and backward
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
			offset.z += speed * dt;
			moved = true;
		}
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
			offset.z -= speed * dt;
			moved = true;
		}

		// Move right and left
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
			offset.x += speed * dt;
			moved = true;
		}
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
			offset.x -= speed * dt;
			moved = true;
		}

		// Move up and down
		if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
			offset.y += speed * dt;
			moved = true;
		}
		if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
			offset.y -= speed * dt;
			moved = true;
		}


		glm::mat4 transform = glm::eulerAngleYXZ(angle.y, angle.x, 0.0f);

		// offset = transform * offset;
		this->position += glm::vec3(offset.x, offset.y, offset.z);

		// Calculate mouse delta for rotation
		int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);
		if (state != GLFW_PRESS) {
			this->init_mouse = false;
			return moved;
		}

		double mouse_x{}, mouse_y{};
		glfwGetCursorPos(window, &mouse_x, &mouse_y);

		glm::vec2 current_mouse_position = { mouse_x, mouse_y };
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);

		current_mouse_position.x /= float(width);
		current_mouse_position.y /= float(height);
		current_mouse_position.y = 1 - current_mouse_position.y;

		if (!this->init_mouse) {
			this->prev_mouse_position = current_mouse_position;
			this->init_mouse = true;
			return false;
		}

		this->mouse_delta = current_mouse_position - prev_mouse_position;
		this->prev_mouse_position = current_mouse_position;
		angle += mouse_delta;
		if (glm::length(mouse_delta) > 0.0f) {
			moved = true; // Camera has rotated, so it counts as moved
		}
		return moved;
		// Update the angle based on mouse delta
	}
	auto getPosition() { return position; }
	auto getAngleOffset() { return angle; }
};
