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
		vk::Queue m_PresentQueue;
		vk::SurfaceKHR m_Surface;
		vk::SwapchainKHR m_SwapChain;
		vk::Format m_SwapChainImageFormat;
		vk::Extent2D m_SwapChainExtent;

		std::vector<vk::Image> m_SwapChainImages;

		std::vector<const char*> m_ValidationLayers;

		void create_instance();
		bool is_device_suitable(const vk::PhysicalDevice &Device) const;
		int rate_physical_device(const vk::PhysicalDevice &Device) const;
		void pick_physical_device();
		bool check_device_ext_support(const vk::PhysicalDevice &Device) const;
		void create_logical_device();
		void create_surface();

		struct QueueFamilyIndices {
			std::optional<uint32_t> m_GraphicsFamily;
			std::optional<uint32_t> m_PresentFamily;

			bool is_complete() const {
				return m_GraphicsFamily.has_value()
					&& m_PresentFamily.has_value();
			}
		};

		QueueFamilyIndices find_queue_families(const vk::PhysicalDevice &device) const;

		struct SwapChainSupportDetails {
			vk::SurfaceCapabilitiesKHR m_Capabilities;
			std::vector<vk::SurfaceFormatKHR> m_Formats;
			std::vector<vk::PresentModeKHR> m_PresentModes;
		};

		SwapChainSupportDetails query_swap_chain_support(const vk::PhysicalDevice &Device) const;

		vk::SurfaceFormatKHR choose_surface_format(const std::vector<vk::SurfaceFormatKHR> &Formats) const;
		vk::PresentModeKHR choose_present_mode(const std::vector<vk::PresentModeKHR> &Modes) const;
		vk::Extent2D choose_swap_extent(const vk::SurfaceCapabilitiesKHR &Capabilities) const;
		void create_swap_chain();

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
