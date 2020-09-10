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
#include <fstream>
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

static std::vector<char> readFile(const std::string &filename) {
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("failed to open file!");
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();

	return buffer;
}

CRenderer::CRenderer(CEngine *pEngine) : CLoggable("renderer"), m_Window(nullptr, nullptr) {
	m_pEngine = pEngine;

	m_ValidationLayers = {"VK_LAYER_KHRONOS_validation"};
}

void CRenderer::init() {
	// TODO: Use a config
	Log()->info("Starting vulkan renderer...");
	m_Window = util::makeResource(SDL_CreateWindow, SDL_DestroyWindow, "", SDL_WINDOWPOS_UNDEFINED,
								  SDL_WINDOWPOS_UNDEFINED,
								  640, 480,
								  SDL_WINDOW_VULKAN);
	createInstance();
	setupDebugCallback();
	create_surface();
	pickPhysicalDevice();
	createLogicalDevice();
	createSwapChain();
	createImageViews();
	Log()->info("Renderer started.");
}

void CRenderer::quit() {
}

void CRenderer::createInstance() {
	{
		PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = m_DynamicLoader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
		if (vkGetInstanceProcAddr == nullptr) {
			Log()->info("vkGetInstanceProcAddr is nullptr!");
		}
		VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

		if (EnableValidationLayers && !checkValidationLayerSupport()) {
			throw std::runtime_error("validation layers requested, but not available!");
		}
		auto AppInfo = vk::ApplicationInfo(
			engine()->getGameName(),
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

int CRenderer::ratePhysicalDevice(const vk::PhysicalDevice &device) const {
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

	if (!isDeviceSuitable(device))
		return 0;

	Log()->debug("Score: {}", score);

	return score;
}

bool CRenderer::checkDeviceExtSupport(const vk::PhysicalDevice &Device) const {
	std::set<std::string> RequiredExtensions(DeviceExtensions.begin(), DeviceExtensions.end());

	for (const auto &ext : Device.enumerateDeviceExtensionProperties()) {
		RequiredExtensions.erase(ext.extensionName.data());
	}

	return RequiredExtensions.empty();
}

bool CRenderer::isDeviceSuitable(const vk::PhysicalDevice &Device) const {
	QueueFamilyIndices Indices = findQueueFamilies(Device);

	bool ExtensionsSupported = checkDeviceExtSupport(Device);

	bool SwapChainGood = false;
	if (ExtensionsSupported) {
		SwapChainSupportDetails Details = querySwapChainSupport(Device);
		SwapChainGood = !Details.m_Formats.empty() && !Details.m_PresentModes.empty();
	}

	return Indices.isComplete() && ExtensionsSupported && SwapChainGood;
}

void CRenderer::pickPhysicalDevice() {
	Log()->debug("Picking a physical device");
	std::multimap<int, vk::PhysicalDevice> Candidates;

	for (const auto &device : m_Instance.enumeratePhysicalDevices()) {
		int score = ratePhysicalDevice(device);
		Candidates.insert(std::make_pair(score, device));
	}

	if (Candidates.rbegin()->first > 0) {
		m_PhysicalDevice = Candidates.rbegin()->second;
		Log()->debug("Picked physical device: {}", m_PhysicalDevice.getProperties().deviceName.data());
	} else {
		throw std::runtime_error("Failed to find a suitable GPU.");
	}
}

CRenderer::QueueFamilyIndices CRenderer::findQueueFamilies(const vk::PhysicalDevice &Device) const {
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

		if (Indices.isComplete())
			break;
		i++;
	}

	return Indices;
}

void CRenderer::createLogicalDevice() {
	Log()->debug("Creating logical device");
	QueueFamilyIndices Indices = findQueueFamilies(m_PhysicalDevice);

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

void CRenderer::setupDebugCallback() {
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

CRenderer::SwapChainSupportDetails CRenderer::querySwapChainSupport(const vk::PhysicalDevice &Device) const {
	SwapChainSupportDetails Details;

	Details.m_Capabilities = Device.getSurfaceCapabilitiesKHR(m_Surface);
	Details.m_Formats = Device.getSurfaceFormatsKHR(m_Surface);
	Details.m_PresentModes = Device.getSurfacePresentModesKHR(m_Surface);

	return Details;
}

vk::SurfaceFormatKHR CRenderer::chooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &Formats) const {
	for (const auto &Format : Formats) {
		if (Format.format == vk::Format::eB8G8R8A8Srgb && Format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
			return Format;
		}
	}

	return Formats[0];
}

vk::PresentModeKHR CRenderer::choosePresentMode(const std::vector<vk::PresentModeKHR> &Modes) const {
	// https://vulkan-tutorial.com/en/Drawing_a_triangle/Presentation/Swap_chain

	for (const auto &Mode : Modes) {
		if (Mode == vk::PresentModeKHR::eMailbox)
			return Mode;
	}

	return vk::PresentModeKHR::eFifo;
}
vk::Extent2D CRenderer::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR &Capabilities) const {
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

void CRenderer::createSwapChain() {
	Log()->debug("Creating swap chain");
	SwapChainSupportDetails Details = querySwapChainSupport(m_PhysicalDevice);

	vk::SurfaceFormatKHR Format = chooseSurfaceFormat(Details.m_Formats);
	vk::PresentModeKHR Mode = choosePresentMode(Details.m_PresentModes);
	vk::Extent2D Extent = chooseSwapExtent(Details.m_Capabilities);

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

	QueueFamilyIndices Indices = findQueueFamilies(m_PhysicalDevice);
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

void CRenderer::createImageViews() {
	Log()->debug("Creating image views");
	m_SwapChainImageViews.resize(m_SwapChainImages.size());

	for (size_t i = 0; i < m_SwapChainImageViews.size(); i++) {
		vk::ImageViewCreateInfo CreateInfo = {};
		CreateInfo.image = m_SwapChainImages[i];
		CreateInfo.viewType = vk::ImageViewType::e2D;
		CreateInfo.format = m_SwapChainImageFormat;
		CreateInfo.components.r = vk::ComponentSwizzle::eIdentity;
		CreateInfo.components.g = vk::ComponentSwizzle::eIdentity;
		CreateInfo.components.b = vk::ComponentSwizzle::eIdentity;
		CreateInfo.components.a = vk::ComponentSwizzle::eIdentity;
		CreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		CreateInfo.subresourceRange.baseMipLevel = 0;
		CreateInfo.subresourceRange.levelCount = 1;
		CreateInfo.subresourceRange.baseArrayLayer = 0;
		CreateInfo.subresourceRange.layerCount = 1;

		try {
			m_SwapChainImageViews[i] = m_Device.createImageView(CreateInfo);
		} catch (vk::SystemError &err) {
			throw std::runtime_error("failed to create image views!");
		}
	}

	Log()->debug("Created image views");
}

void CRenderer::createRenderPass() {
	vk::AttachmentDescription colorAttachment = {};
	colorAttachment.format = m_SwapChainImageFormat;
	colorAttachment.samples = vk::SampleCountFlagBits::e1;
	colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
	colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
	colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
	colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

	vk::AttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

	vk::SubpassDescription subpass = {};
	subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	vk::RenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
}

void CRenderer::createGraphicsPipeline() {
	auto VertShaderCode = readFile("shaders/vert.spv");
	auto FragShaderCode = readFile("shaders/frag.spv");

	vk::ShaderModule VertShader = createShaderModule(VertShaderCode);
	vk::ShaderModule FragShader = createShaderModule(FragShaderCode);

	vk::PipelineShaderStageCreateInfo Stages[] = {
		{vk::PipelineShaderStageCreateFlags(),
		 vk::ShaderStageFlagBits::eVertex,
		 VertShader,
		 "main"},
		{vk::PipelineShaderStageCreateFlags(),
		 vk::ShaderStageFlagBits::eFragment,
		 FragShader,
		 "main"}};

	auto VertexInputInfo = vk::PipelineVertexInputStateCreateInfo(
		vk::PipelineVertexInputStateCreateFlags(),
		0,
		nullptr,
		0,
		nullptr);

	auto InputAssembly = vk::PipelineInputAssemblyStateCreateInfo(
		vk::PipelineInputAssemblyStateCreateFlags(),
		vk::PrimitiveTopology::eTriangleList,
		VK_FALSE);

	vk::Viewport Viewport(0, 0, m_SwapChainExtent.width, m_SwapChainExtent.height, 0.0f, 1.0f);

	vk::Rect2D Scissor(vk::Offset2D(0, 0), m_SwapChainExtent);

	vk::PipelineViewportStateCreateInfo ViewportState = {};
	ViewportState.viewportCount = 1;
	ViewportState.pViewports = &Viewport;
	ViewportState.scissorCount = 1;
	ViewportState.pScissors = &Scissor;

	vk::PipelineRasterizationStateCreateInfo Rasterizer = {};
	Rasterizer.depthClampEnable = VK_FALSE;
	Rasterizer.rasterizerDiscardEnable = VK_FALSE;
	Rasterizer.polygonMode = vk::PolygonMode::eFill;
	Rasterizer.lineWidth = 1.0f;
	Rasterizer.cullMode = vk::CullModeFlagBits::eBack;
	Rasterizer.frontFace = vk::FrontFace::eClockwise;
	Rasterizer.depthBiasEnable = VK_FALSE;

	vk::PipelineMultisampleStateCreateInfo Multisampling = {};
	Multisampling.sampleShadingEnable = VK_FALSE;
	Multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
	Multisampling.minSampleShading = 1.0f;			// Optional
	Multisampling.pSampleMask = nullptr;			// Optional
	Multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	Multisampling.alphaToOneEnable = VK_FALSE;		// Optional

	vk::PipelineColorBlendAttachmentState ColorBlendAttachment;
	ColorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
	ColorBlendAttachment.blendEnable = VK_FALSE;

	vk::PipelineColorBlendStateCreateInfo ColorBlending = {};
	ColorBlending.logicOpEnable = VK_FALSE;
	ColorBlending.logicOp = vk::LogicOp::eCopy;
	ColorBlending.attachmentCount = 1;
	ColorBlending.pAttachments = &ColorBlendAttachment;
	ColorBlending.blendConstants[0] = 0.0f;
	ColorBlending.blendConstants[1] = 0.0f;
	ColorBlending.blendConstants[2] = 0.0f;
	ColorBlending.blendConstants[3] = 0.0f;

	vk::PipelineLayoutCreateInfo PipelineLayoutInfo = {};
	PipelineLayoutInfo.setLayoutCount = 0;			  // Optional
	PipelineLayoutInfo.pSetLayouts = nullptr;		  // Optional
	PipelineLayoutInfo.pushConstantRangeCount = 0;	  // Optional
	PipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

	m_PipelineLayout = m_Device.createPipelineLayout(PipelineLayoutInfo);

	m_Device.destroyShaderModule(FragShader);
}

vk::ShaderModule CRenderer::createShaderModule(const std::vector<char> &code) {
	auto CreateInfo = vk::ShaderModuleCreateInfo(
		vk::ShaderModuleCreateFlags(),
		code.size(),
		reinterpret_cast<const uint32_t *>(code.data()));

	return m_Device.createShaderModule(CreateInfo);
}

bool CRenderer::checkValidationLayerSupport() {
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
