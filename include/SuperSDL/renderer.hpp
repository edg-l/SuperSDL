#ifndef SUPERSDL_RENDERER_HPP
#define SUPERSDL_RENDERER_HPP

#include "SuperSDL/engine.hpp"
#include "SuperSDL/loggable.hpp"
#include "util.hpp"
#include <optional>
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#include <vector>


namespace sps {

class CRenderer : CLoggable {
	private:
		CEngine *m_pEngine;
		util::window_ptr_t m_Window;

		// Vulkan stuff
		vk::Instance m_Instance;
		vk::DynamicLoader m_DynamicLoader;
		vk::PhysicalDevice m_PhysicalDevice;
		vk::Device m_Device;
		vk::Queue m_GraphicsQueue;
		vk::SurfaceKHR m_Surface;
		std::vector<const char*> m_ValidationLayers;

		void create_instance();
		bool is_device_suitable(const vk::PhysicalDevice &device) const;
		int rate_physical_device(const vk::PhysicalDevice &device) const;
		void pick_physical_device();
		void create_logical_device();

		struct QueueFamilyIndices {
			std::optional<uint32_t> m_GraphicsFamily;

			bool is_complete() const {
				return m_GraphicsFamily.has_value();
			}
		};

		QueueFamilyIndices find_queue_families(const vk::PhysicalDevice &device) const;
		void setup_debug_callback();
		bool check_validation_layer_support();
		CEngine *engine() { return m_pEngine; }

	public:
		CRenderer(CEngine *pEngine);
		void init();
		void quit();
};

} // namespace sps

#endif
