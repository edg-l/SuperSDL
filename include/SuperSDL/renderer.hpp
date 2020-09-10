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
		vk::PipelineLayout m_PipelineLayout;
		vk::Format m_SwapChainImageFormat;

		std::vector<vk::Image> m_SwapChainImages;
		std::vector<vk::ImageView> m_SwapChainImageViews;

		std::vector<const char*> m_ValidationLayers;

		void createInstance();
		bool isDeviceSuitable(const vk::PhysicalDevice &Device) const;
		int ratePhysicalDevice(const vk::PhysicalDevice &Device) const;
		void pickPhysicalDevice();
		bool checkDeviceExtSupport(const vk::PhysicalDevice &Device) const;
		void createLogicalDevice();
		void create_surface();
		void createImageViews();
		void createRenderPass();
		void createGraphicsPipeline();

		struct QueueFamilyIndices {
			std::optional<uint32_t> m_GraphicsFamily;
			std::optional<uint32_t> m_PresentFamily;

			bool isComplete() const {
				return m_GraphicsFamily.has_value()
					&& m_PresentFamily.has_value();
			}
		};

		QueueFamilyIndices findQueueFamilies(const vk::PhysicalDevice &device) const;

		struct SwapChainSupportDetails {
			vk::SurfaceCapabilitiesKHR m_Capabilities;
			std::vector<vk::SurfaceFormatKHR> m_Formats;
			std::vector<vk::PresentModeKHR> m_PresentModes;
		};

		SwapChainSupportDetails querySwapChainSupport(const vk::PhysicalDevice &Device) const;

		vk::SurfaceFormatKHR chooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &Formats) const;
		vk::PresentModeKHR choosePresentMode(const std::vector<vk::PresentModeKHR> &Modes) const;
		vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR &Capabilities) const;
		void createSwapChain();

		vk::ShaderModule createShaderModule(const std::vector<char> &code);

		void setupDebugCallback();
		bool checkValidationLayerSupport();
		CEngine *engine() { return m_pEngine; }

	public:
		CRenderer(CEngine *pEngine);
		void init();
		void quit();
};

} // namespace sps

#endif
