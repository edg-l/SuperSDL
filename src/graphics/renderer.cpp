#include "SuperSDL/engine.hpp"
#include <SDL.h>
#include <SDL_vulkan.h>
#include <SuperSDL/renderer.hpp>
#include <SuperSDL/util.hpp>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <map>
#include <set>
#include <stdexcept>
#include <utility>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace sps {

#ifdef NDEBUG
const bool EnableValidationLayers = false;
#else
const bool EnableValidationLayers = true;
#endif

const std::vector<const char *> DeviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME};

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData) {
	(void)pUserData;
	if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		fprintf(stderr, "Vulkan layer: (severity=%d, type=%d) %s\n", messageSeverity, messageType, pCallbackData->pMessage);
	return VK_FALSE;
}

CRenderer::CRenderer(CEngine *pEngine) : CLoggable("renderer"), m_Window(nullptr, nullptr) {
	m_pEngine = pEngine;

	m_ValidationLayers = {"VK_LAYER_KHRONOS_validation"};
}

void CRenderer::init() {
	// TODO: Use a config
	Log()->info("Starting vulkan renderer...");
	m_Window = util::make_resource(SDL_CreateWindow, SDL_DestroyWindow, "", SDL_WINDOWPOS_UNDEFINED,
								   SDL_WINDOWPOS_UNDEFINED,
								   640, 480,
								   SDL_WINDOW_VULKAN);
	create_instance();
	setup_debug_callback();
	create_surface();
	pick_physical_device();
	create_logical_device();
	create_swap_chain();
	Log()->info("Renderer started.");
}

void CRenderer::quit() {
}

void CRenderer::create_instance() {
	{
		PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = m_DynamicLoader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
		if (vkGetInstanceProcAddr == nullptr) {
			Log()->info("vkGetInstanceProcAddr is nullptr!");
		}
		VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

		if (EnableValidationLayers && !check_validation_layer_support()) {
			throw std::runtime_error("validation layers requested, but not available!");
		}
		auto AppInfo = vk::ApplicationInfo(
			engine()->get_game_name(),
			VK_MAKE_VERSION(1, 0, 0),
			"SuperSDL",
			VK_MAKE_VERSION(1, 0, 0),
			VK_API_VERSION_1_2);

		unsigned int count;
		std::vector<const char *> RequiredExtensions = {};

#ifndef NDEBUG
		RequiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

		size_t AdditionExtCount = RequiredExtensions.size();

		if (SDL_Vulkan_GetInstanceExtensions(m_Window.get(), &count, nullptr)) {
			RequiredExtensions.resize(AdditionExtCount + count);
		}

		SDL_Vulkan_GetInstanceExtensions(m_Window.get(), &count, RequiredExtensions.data() + AdditionExtCount);

		for (const auto &ext : RequiredExtensions) {
			Log()->info("Vulkan - Required extension: {}", ext);
		}

		// This shows all the available extensions
		auto AvailableExtensions = vk::enumerateInstanceExtensionProperties();

		for (const auto &ext : AvailableExtensions) {
			Log()->debug("Vulkan - Available extension: {}", ext.extensionName);
		}

		auto CreateInfo = vk::InstanceCreateInfo(
			vk::InstanceCreateFlags(),
			&AppInfo,
			0, nullptr,
			RequiredExtensions.size(), RequiredExtensions.data());

		if (EnableValidationLayers) {
			CreateInfo.enabledLayerCount = m_ValidationLayers.size();
			CreateInfo.ppEnabledLayerNames = m_ValidationLayers.data();
			Log()->debug("Vulkan - Enabled validation layers (size={})", m_ValidationLayers.size());
		}

		try {
			m_Instance = vk::createInstance(CreateInfo);
			VULKAN_HPP_DEFAULT_DISPATCHER.init(m_Instance);
		} catch (vk::SystemError &err) {
			Log()->error("Error creating vulkan instance: {}", err.what());
		}
	}

	// TODO: Initialize it also with a device later: https://github.com/KhronosGroup/Vulkan-Hpp#extensions--per-device-function-pointers
}

int CRenderer::rate_physical_device(const vk::PhysicalDevice &device) const {
	Log()->debug("Rating physical device: {}", device.getProperties().deviceName.data());
	int score = 0;

	vk::PhysicalDeviceProperties Properties = device.getProperties();
	vk::PhysicalDeviceFeatures Features = device.getFeatures();

	// Favor dedicated gpus
	if (Properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
		score += 1000;

	Log()->debug("MaxImageDimension2D: {}", Properties.limits.maxImageDimension2D);
	score += Properties.limits.maxImageDimension2D;

	if (!Features.geometryShader)
		return 0;

	if (!is_device_suitable(device))
		return 0;

	Log()->debug("Score: {}", score);

	return score;
}

bool CRenderer::check_device_ext_support(const vk::PhysicalDevice &Device) const {
	std::set<std::string> RequiredExtensions(DeviceExtensions.begin(), DeviceExtensions.end());

	for (const auto &ext : Device.enumerateDeviceExtensionProperties()) {
		RequiredExtensions.erase(ext.extensionName.data());
	}

	return RequiredExtensions.empty();
}

bool CRenderer::is_device_suitable(const vk::PhysicalDevice &Device) const {
	QueueFamilyIndices Indices = find_queue_families(Device);

	bool ExtensionsSupported = check_device_ext_support(Device);

	bool SwapChainGood = false;
	if (ExtensionsSupported) {
		SwapChainSupportDetails Details = query_swap_chain_support(Device);
		SwapChainGood = !Details.m_Formats.empty() && !Details.m_PresentModes.empty();
	}

	return Indices.is_complete() && ExtensionsSupported && SwapChainGood;
}

void CRenderer::pick_physical_device() {
	Log()->debug("Picking a physical device");
	std::multimap<int, vk::PhysicalDevice> Candidates;

	for (const auto &device : m_Instance.enumeratePhysicalDevices()) {
		int score = rate_physical_device(device);
		Candidates.insert(std::make_pair(score, device));
	}

	if (Candidates.rbegin()->first > 0) {
		m_PhysicalDevice = Candidates.rbegin()->second;
		Log()->debug("Picked physical device: {}", m_PhysicalDevice.getProperties().deviceName.data());
	} else {
		throw std::runtime_error("Failed to find a suitable GPU.");
	}
}

CRenderer::QueueFamilyIndices CRenderer::find_queue_families(const vk::PhysicalDevice &Device) const {
	QueueFamilyIndices Indices;

	auto Properties = Device.getQueueFamilyProperties();

	int i = 0;
	for (const auto &QueueFamily : Properties) {
		if (QueueFamily.queueCount > 0 && QueueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
			Indices.m_GraphicsFamily = i;
		}

		if (QueueFamily.queueCount > 0 && Device.getSurfaceSupportKHR(i, m_Surface)) {
			Indices.m_PresentFamily = i;
		}

		if (Indices.is_complete())
			break;
		i++;
	}

	return Indices;
}

void CRenderer::create_logical_device() {
	Log()->debug("Creating logical device");
	QueueFamilyIndices Indices = find_queue_families(m_PhysicalDevice);

	float priority = 1.0f;

	auto QueueCreateInfo = vk::DeviceQueueCreateInfo(
		vk::DeviceQueueCreateFlags(),
		Indices.m_GraphicsFamily.value(),
		1, &priority);

	auto DeviceFeatures = vk::PhysicalDeviceFeatures();

	auto DeviceCreateInfo = vk::DeviceCreateInfo(
		vk::DeviceCreateFlags(),
		1, &QueueCreateInfo);

	DeviceCreateInfo.pEnabledFeatures = &DeviceFeatures;
	DeviceCreateInfo.enabledExtensionCount = DeviceExtensions.size();
	DeviceCreateInfo.ppEnabledExtensionNames = DeviceExtensions.data();

	if (EnableValidationLayers) {
		DeviceCreateInfo.enabledLayerCount = m_ValidationLayers.size();
		DeviceCreateInfo.ppEnabledLayerNames = m_ValidationLayers.data();
	}

	try {
		m_Device = m_PhysicalDevice.createDevice(DeviceCreateInfo);
	} catch (vk::SystemError &err) {
		Log()->error("Failed to create logical device: {}", err.what());
		throw std::runtime_error("Failed to create logical device:");
	}

	VULKAN_HPP_DEFAULT_DISPATCHER.init(m_Device);
	m_GraphicsQueue = m_Device.getQueue(Indices.m_GraphicsFamily.value(), 0);
	m_PresentQueue = m_Device.getQueue(Indices.m_PresentFamily.value(), 0);
	Log()->debug("Logical device created");
}

void CRenderer::create_surface() {
	Log()->debug("Creating surface");
	if (!SDL_Vulkan_CreateSurface(m_Window.get(), static_cast<VkInstance>(m_Instance), reinterpret_cast<VkSurfaceKHR *>(&m_Surface))) {
		Log()->error("Error creating a surface to draw on!");
		throw std::runtime_error("Error creating a surface to draw on!");
	}
	Log()->debug("Surface created");
}

void CRenderer::setup_debug_callback() {
	if (!EnableValidationLayers)
		return;

	auto CreateDebugInfo = vk::DebugUtilsMessengerCreateInfoEXT(
		vk::DebugUtilsMessengerCreateFlagsEXT(),
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
		vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
		debugCallback,
		nullptr);

	m_Instance.createDebugUtilsMessengerEXT(CreateDebugInfo);
}

CRenderer::SwapChainSupportDetails CRenderer::query_swap_chain_support(const vk::PhysicalDevice &Device) const {
	SwapChainSupportDetails Details;

	Details.m_Capabilities = Device.getSurfaceCapabilitiesKHR(m_Surface);
	Details.m_Formats = Device.getSurfaceFormatsKHR(m_Surface);
	Details.m_PresentModes = Device.getSurfacePresentModesKHR(m_Surface);

	return Details;
}

vk::SurfaceFormatKHR CRenderer::choose_surface_format(const std::vector<vk::SurfaceFormatKHR> &Formats) const {
	for (const auto &Format : Formats) {
		if (Format.format == vk::Format::eB8G8R8A8Srgb && Format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
			return Format;
		}
	}

	return Formats[0];
}

vk::PresentModeKHR CRenderer::choose_present_mode(const std::vector<vk::PresentModeKHR> &Modes) const {
	// https://vulkan-tutorial.com/en/Drawing_a_triangle/Presentation/Swap_chain

	for (const auto &Mode : Modes) {
		if (Mode == vk::PresentModeKHR::eMailbox)
			return Mode;
	}

	return vk::PresentModeKHR::eFifo;
}
vk::Extent2D CRenderer::choose_swap_extent(const vk::SurfaceCapabilitiesKHR &Capabilities) const {
	if (Capabilities.currentExtent.width != UINT32_MAX)
		return Capabilities.currentExtent;
	else {
		int w, h;
		SDL_Vulkan_GetDrawableSize(m_Window.get(), &w, &h);
		vk::Extent2D ActualExtent = {(uint32_t)w, (uint32_t)h};
		ActualExtent.width = std::max(Capabilities.minImageExtent.width, std::min(Capabilities.maxImageExtent.width, ActualExtent.width));
		ActualExtent.height = std::max(Capabilities.minImageExtent.height, std::min(Capabilities.maxImageExtent.height, ActualExtent.height));

		return ActualExtent;
	}
}

void CRenderer::create_swap_chain() {
	Log()->debug("Creating swap chain");
	SwapChainSupportDetails Details = query_swap_chain_support(m_PhysicalDevice);

	vk::SurfaceFormatKHR Format = choose_surface_format(Details.m_Formats);
	vk::PresentModeKHR Mode = choose_present_mode(Details.m_PresentModes);
	vk::Extent2D Extent = choose_swap_extent(Details.m_Capabilities);

	uint32_t ImageCount = Details.m_Capabilities.minImageCount + 1;

	if (Details.m_Capabilities.maxImageCount > 0 && ImageCount > Details.m_Capabilities.maxImageCount)
		ImageCount = Details.m_Capabilities.maxImageCount;

	vk::SwapchainCreateInfoKHR CreateInfo(
		vk::SwapchainCreateFlagsKHR(),
		m_Surface,
		ImageCount,
		Format.format,
		Format.colorSpace,
		Extent,
		1,
		vk::ImageUsageFlagBits::eColorAttachment);

	QueueFamilyIndices Indices = find_queue_families(m_PhysicalDevice);
	uint32_t FamilyIndices[] = {Indices.m_GraphicsFamily.value(), Indices.m_PresentFamily.value()};

	if (Indices.m_GraphicsFamily != Indices.m_PresentFamily) {
		CreateInfo.imageSharingMode = vk::SharingMode::eConcurrent;
		CreateInfo.queueFamilyIndexCount = 2;
		CreateInfo.pQueueFamilyIndices = FamilyIndices;
		Log()->debug("Using concurrent image sharing mode");
	} else {
		CreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
		CreateInfo.queueFamilyIndexCount = 0;
		CreateInfo.pQueueFamilyIndices = nullptr;
		// Best perfomance
		Log()->debug("Using exclusive image sharing mode");
	}

	CreateInfo.preTransform = Details.m_Capabilities.currentTransform;
	CreateInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;

	CreateInfo.presentMode = Mode;
	CreateInfo.clipped = VK_TRUE;

	try {
		m_SwapChain = m_Device.createSwapchainKHR(CreateInfo);
	} catch (vk::SystemError &err) {
		Log()->error("failed to create swap chain");
		throw std::runtime_error("failed to create swap chain");
	}

	m_SwapChainImages = m_Device.getSwapchainImagesKHR(m_SwapChain);
	m_SwapChainImageFormat = Format.format;
	m_SwapChainExtent = Extent;

	Log()->debug("Swap chain created");
}

bool CRenderer::check_validation_layer_support() {
	auto AvailableLayers = vk::enumerateInstanceLayerProperties();
	for (auto LayerName : m_ValidationLayers) {
		bool Found = false;

		for (const auto &LayerProperties : AvailableLayers) {
			if (strcmp(LayerName, LayerProperties.layerName) == 0) {
				Found = true;
				break;
			}
		}

		if (!Found)
			return false;
	}

	return true;
}

} // namespace sps
