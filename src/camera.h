#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <iostream>
#include <glm/gtx/euler_angles.hpp>
using WindowHandler = GLFWwindow;

class Camera
{
	glm::vec3 position;
	glm::vec2 mouse_delta{0};
	glm::vec2 prev_mouse_position{0};
	glm::vec2 angle{0};
	bool init_mouse = false;

public:
	Camera(glm::vec3 position) { this->position = position; }

	// returns true if camera was moved
	bool onUpdate(WindowHandler *window, float dt)
	{
		glm::vec3 offset{0.0f};
		float speed = 10.0f;
		bool moved = false;

		// Collect input first
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		{
			offset.z += speed * dt; // Forward in camera space
			moved = true;
		}
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		{
			offset.z -= speed * dt; // Backward in camera space
			moved = true;
		}
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		{
			offset.x += speed * dt; // Right in camera space
			moved = true;
		}
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		{
			offset.x -= speed * dt; // Left in camera space
			moved = true;
		}
		if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
		{
			offset.y += speed * dt; // Up in world space
			moved = true;
		}
		if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
		{
			offset.y -= speed * dt; // Down in world space
			moved = true;
		}

		// Transform movement based on camera rotation (only Y rotation for WASD)
		if (offset.x != 0.0f || offset.z != 0.0f)
		{
			// Calculate forward and right vectors based on yaw angle
			float yaw = -angle.x; // Use angle.x for yaw (horizontal rotation)
			glm::vec3 forward = glm::vec3(sin(yaw), 0.0f, cos(yaw));
			glm::vec3 right = glm::vec3(cos(yaw), 0.0f, -sin(yaw));

			// Apply movement in the correct directions
			this->position += forward * offset.z; // Forward/backward
			this->position += right * offset.x;	  // Left/right
		}

		// Apply vertical movement directly (world space)
		this->position.y += offset.y;

		// Handle mouse input
		int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);
		if (state != GLFW_PRESS)
		{
			this->init_mouse = false;
			return moved;
		}

		double mouse_x{}, mouse_y{};
		glfwGetCursorPos(window, &mouse_x, &mouse_y);

		glm::vec2 current_mouse_position = {mouse_x, mouse_y};
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);

		current_mouse_position.x /= float(width);
		current_mouse_position.y /= float(height);
		current_mouse_position.y = 1 - current_mouse_position.y;

		if (!this->init_mouse)
		{
			this->prev_mouse_position = current_mouse_position;
			this->init_mouse = true;
			return moved;
		}

		this->mouse_delta = current_mouse_position - prev_mouse_position;
		this->prev_mouse_position = current_mouse_position;

		// Simple mouse sensitivity
		float sensitivity = 3.0f;
		angle.x += mouse_delta.x * sensitivity; // Yaw (horizontal)
		angle.y += mouse_delta.y * sensitivity; // Pitch (vertical)

		// Clamp pitch to prevent flipping
		angle.y = glm::clamp(angle.y, -1.5f, 1.5f);

		if (glm::length(mouse_delta) > 0.0f)
		{
			moved = true;
		}
		return moved;
	}

	auto getPosition() { return position; }
	auto getAngleOffset() { return angle; }
};