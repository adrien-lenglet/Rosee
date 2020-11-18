#pragma once

#include "vector.hpp"
#include "Vk.hpp"
#include <GLFW/glfw3.h>
#include "math.hpp"

namespace Rosee {

class Renderer
{
	size_t m_frame_count;
	bool m_validate;
	bool m_use_render_doc;

	void throwGlfwError(void);
	GLFWwindow *m_window;
	GLFWwindow *createWindow(void);
	svec2 getWindowSize(void) const;

	Vk::Instance m_instance;
	Vk::Instance createInstance(void);
	Vk::DebugUtilsMessengerEXT m_debug_messenger;
	Vk::DebugUtilsMessengerEXT createDebugMessenger(void);
	Vk::SurfaceKHR m_surface;
	Vk::SurfaceKHR createSurface(void);
	VkPhysicalDeviceProperties m_properties;
	VkPhysicalDeviceLimits m_limits;
	VkPhysicalDeviceFeatures m_features;
	VkPresentModeKHR m_present_mode;
	VkSurfaceFormatKHR m_surface_format;
	VkSurfaceCapabilitiesKHR m_surface_capabilities;
	uint32_t m_queue_family_graphics = ~0U;
	Vk::Device m_device;
	Vk::Device createDevice(void);
	Vk::Queue m_queue;
	Vk::SwapchainKHR m_swapchain;
	Vk::SwapchainKHR createSwapchain(void);
	vector<VkImage> m_swapchain_images;
	vector<Vk::ImageView> m_swapchain_image_views;
	vector<Vk::ImageView> createSwapchainImageViews(void);

	Vk::RenderPass m_opaque_pass;
	Vk::RenderPass createOpaquePass(void);
	vector<Vk::Framebuffer> m_opaque_fbs;
	vector<Vk::Framebuffer> createOpaqueFbs(void);
	Vk::CommandPool m_command_pool;

	class Frame
	{
		Renderer &m_r;
		Vk::CommandBuffer m_cmd;
		Vk::Fence m_frame_done;
		Vk::Semaphore m_render_done;
		Vk::Semaphore m_image_ready;
		bool m_ever_submitted = false;

	public:
		Frame(Renderer &r, VkCommandBuffer cmd);
		~Frame(void);

		void render(void);
	};

	vector<Frame> m_frames;
	vector<Frame> createFrames(void);
	size_t m_current_frame = 0;

public:
	Renderer(size_t frameCount, bool validate, bool useRenderDoc);
	~Renderer(void);

	void pollEvents(void);
	bool shouldClose(void) const;
	void render(void);
};

}