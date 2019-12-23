#include "InexorRenderer.hpp"

using namespace std;


namespace inexor {
namespace vulkan_renderer {

	
	void InexorRenderer::init_window(const int width, const int height, const std::string& window_name)
	{
		glfwInit();

		// We do not want to use the OpenGL API.
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		// The window shall not be resizable for now.
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		window_width = width;
		window_height = height;
		
		window = glfwCreateWindow(width, height, window_name.c_str(), nullptr, nullptr);
	}


	void InexorRenderer::shutdown_window()
	{
		glfwDestroyWindow(window);
		glfwTerminate();
	}


	VkResult InexorRenderer::create_vulkan_instance(const std::string& application_name, const std::string& engine_name, const uint32_t application_version, const uint32_t engine_version, bool enable_validation_layers)
	{
		// Print some debug messages to the console.
		cout << "Application name: " << application_name.c_str() << endl;
		cout << "Application version: " << VK_VERSION_MAJOR(application_version) << "." << VK_VERSION_MINOR(application_version) << "." << VK_VERSION_PATCH(application_version) << endl;
		cout << "Engine name: " << engine_name.c_str() << endl;
		cout << "Engine version: " << VK_VERSION_MAJOR(engine_version) << "." << VK_VERSION_MINOR(engine_version) << "." << VK_VERSION_PATCH(engine_version) << endl;
		cout << endl;

		// TODO: Check which version of Vulkan is available before trying to create an instance!
		// https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkApplicationInfo.html
		// "Because Vulkan 1.0 implementations may fail with VK_ERROR_INCOMPATIBLE_DRIVER,
		// applications should determine the version of Vulkan available before calling vkCreateInstance.
		// If the vkGetInstanceProcAddr returns NULL for vkEnumerateInstanceVersion, it is a Vulkan 1.0
		// implementation. Otherwise, the application can call vkEnumerateInstanceVersion to determine the
		// version of Vulkan."

		VkApplicationInfo app_info = {};
		app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		app_info.pNext = NULL;
		app_info.pApplicationName = application_name.c_str();
		app_info.applicationVersion = application_version;
		app_info.pEngineName = engine_name.c_str();
		app_info.engineVersion = engine_version;
		app_info.apiVersion = VK_API_VERSION_1_0;

		// TODO: Should we switch to Vulkan 1.1?
		// TODO: Check if we need more device or instance extensions!

		uint32_t number_of_GLFW_extensions = 0;
		auto glfw_extensions = glfwGetRequiredInstanceExtensions(&number_of_GLFW_extensions);

		cout << "Required GLFW instance extensions: " << endl;

		for(std::size_t i=0; i<number_of_GLFW_extensions; i++)
		{
			cout << glfw_extensions[i] << endl;
		}

		VkInstanceCreateInfo instance_create_info = {};
		instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instance_create_info.pNext = NULL;
		instance_create_info.flags = NULL;
		instance_create_info.pApplicationInfo = &app_info;
		instance_create_info.enabledExtensionCount = number_of_GLFW_extensions;
		instance_create_info.ppEnabledExtensionNames = glfw_extensions;

		// The layers that we want to enable.
		std::vector<const char*> enabled_instance_layers ={
			//"VK_LAYER_VALVE_steam_overlay",
			//"VK_LAYER_RENDERDOC_Capture"
		};
	
		// TODO: Use VK_LAYER_LUNARG_standard_validation instead?
		const char* validation_layer_name = "VK_LAYER_KHRONOS_validation";
		bool validation_layer_available = false;
		
		// Check if Khronos validation layer is available.
		if(enable_validation_layers)
		{
			// Check if this layer is available at instance leve..
			uint32_t instance_layer_count = 0;
			vkEnumerateInstanceLayerProperties(&instance_layer_count, nullptr);
			
			std::vector<VkLayerProperties> instance_layer_properties(instance_layer_count);
			vkEnumerateInstanceLayerProperties(&instance_layer_count, instance_layer_properties.data());

			for(VkLayerProperties layer : instance_layer_properties)
			{
				if(0 == strcmp(validation_layer_name, layer.layerName))
				{
					// Yes, this validation layer is available!
					validation_layer_available = true;
					break;
				}
			}
		}

		if(validation_layer_available)
		{
			enabled_instance_layers.push_back(validation_layer_name);
		}
		else
		{
			display_error_message("Error: Validation layer VK_LAYER_KHRONOS_validation not present, validation is disabled.");
		}
		
		// Pass all the enabled layers to Vulkan.
		instance_create_info.ppEnabledLayerNames = enabled_instance_layers.data();
		instance_create_info.enabledLayerCount = static_cast<uint32_t>(enabled_instance_layers.size());

		VkResult result = vkCreateInstance(&instance_create_info, nullptr, &vulkan_instance);
		vulkan_error_check(result);


		result = glfwCreateWindowSurface(vulkan_instance, window, nullptr, &vulkan_surface);
		vulkan_error_check(result);

		return result;
	}


	void InexorRenderer::enumerate_physical_devices()
	{
		VkResult result = vkEnumeratePhysicalDevices(vulkan_instance, &number_of_physical_devices, NULL);
		vulkan_error_check(result);

		if(number_of_physical_devices <= 0)
		{
			display_error_message("Error: Could not find any GPU's!");
		}

		cout << "--------------------------------------------------------------------------" << endl;
		cout << "Number of available GPUs: " << number_of_physical_devices << endl;
		cout << "--------------------------------------------------------------------------" << endl;

		// Preallocate memory for the available graphics cards.
		graphics_cards.resize(number_of_physical_devices);

		result = vkEnumeratePhysicalDevices(vulkan_instance, &number_of_physical_devices, graphics_cards.data());
		vulkan_error_check(result);

		// TODO: Add GPU selection based on command line arguments.

		for(std::size_t i=0; i<number_of_physical_devices; i++)
		{
			print_graphics_card_info(graphics_cards[i]);
			print_physical_device_queue_families(graphics_cards[i]);
			print_surface_capabilities(graphics_cards[i]);
			print_supported_surface_formats(graphics_cards[i]);
			print_presentation_modes(graphics_cards[i]);
			cout << endl;
		}
	}


	void InexorRenderer::print_physical_device_queue_families(const VkPhysicalDevice& graphics_card)
	{
		uint32_t number_of_queue_families = 0;

		// Ask for the family queues.
		vkGetPhysicalDeviceQueueFamilyProperties(graphics_card, &number_of_queue_families, NULL);

		cout << "--------------------------------------------------------------------------" << endl;
		cout << "Number of queue families: " << number_of_queue_families << endl;
		cout << "--------------------------------------------------------------------------" << endl;

		// The queue families of the selected graphics card.
		std::vector<VkQueueFamilyProperties> queue_family_properties;

		// Preallocate memory for the family queues.
		queue_family_properties.resize(number_of_queue_families);

		// Get information about physical device queue family properties.
		vkGetPhysicalDeviceQueueFamilyProperties(graphics_card, &number_of_queue_families, queue_family_properties.data());

		// Loop through all available queue families.
		for(std::size_t i=0; i< number_of_queue_families; i++)
		{
			cout << "Queue family " << i << ": " << endl;
			cout << "Queue Count: " << queue_family_properties[i].queueCount << endl;
			cout << "Timestamp Valid Bits: " << queue_family_properties[i].timestampValidBits << endl;

			if(queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) cout << "VK_QUEUE_GRAPHICS_BIT" << endl;
			if(queue_family_properties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) cout << "VK_QUEUE_COMPUTE_BIT" << endl;
			if(queue_family_properties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) cout << "VK_QUEUE_COMPUTE_BIT" << endl;
			if(queue_family_properties[i].queueFlags & VK_QUEUE_TRANSFER_BIT) cout << "VK_QUEUE_TRANSFER_BIT" << endl;
			if(queue_family_properties[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) cout << "VK_QUEUE_SPARSE_BINDING_BIT" << endl;
			if(queue_family_properties[i].queueFlags & VK_QUEUE_PROTECTED_BIT) cout << "VK_QUEUE_PROTECTED_BIT" << endl;

			uint32_t width = queue_family_properties[i].minImageTransferGranularity.width;
			uint32_t height = queue_family_properties[i].minImageTransferGranularity.width;
			uint32_t depth = queue_family_properties[i].minImageTransferGranularity.depth;
			
			cout << "Min Image Timestamp Granularity: " << width << ", " << height << ", " << depth << endl;
			cout << endl;
		}
	}


	VkResult InexorRenderer::create_physical_device(const VkPhysicalDevice& graphics_card)
	{
		cout << "Creating a physical device" << endl;

		// TODO: Lets pick the best device instead of the default device.
		// TODO: Let the user choose which device to use.
		
		const float queue_priorities[] = {1.0f, 1.0f, 1.0f, 1.0f};

		VkDeviceQueueCreateInfo device_queue_create_info = {};
		device_queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		device_queue_create_info.pNext = NULL;
		device_queue_create_info.flags = NULL;

		// TODO: Look which queue family fits best for what we want to do.
		// For now we will use index number 0.
		device_queue_create_info.queueFamilyIndex = 0;
		
		// TODO: Check if 4 queues are even supported!
		device_queue_create_info.queueCount = 4;
		device_queue_create_info.pQueuePriorities = queue_priorities;

		VkPhysicalDeviceFeatures used_features = {};
		
		VkDeviceCreateInfo device_create_info = {};
		device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		device_create_info.pNext = NULL;
		device_create_info.flags = NULL;

		// TODO: Maybe create multiple queues at once?
		device_create_info.queueCreateInfoCount = 1;

		const std::vector<const char*> device_extensions ={
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};

		device_create_info.pQueueCreateInfos = &device_queue_create_info;
		device_create_info.enabledLayerCount = NULL;
		device_create_info.ppEnabledLayerNames = NULL;
		device_create_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
		device_create_info.ppEnabledExtensionNames = device_extensions.data();
		device_create_info.pEnabledFeatures = &used_features;

		return vkCreateDevice(graphics_card, &device_create_info, NULL, &vulkan_device);
	}


	void InexorRenderer::print_instance_layer_properties()
	{
		// The number of available Vulkan layers.
		uint32_t number_of_layers = 0;

		// Ask for the number of available Vulkan layers.
		vkEnumerateInstanceLayerProperties(&number_of_layers, NULL);

		cout << "--------------------------------------------------------------------------" << endl;
		cout << "Number of instance layers: " << number_of_layers << endl;
		cout << "--------------------------------------------------------------------------" << endl;

		std::vector<VkLayerProperties> instance_layer_properties;

		instance_layer_properties.resize(number_of_layers);

		vkEnumerateInstanceLayerProperties(&number_of_layers, instance_layer_properties.data());

		// Loop through all available layers and print information about them.
		for(std::size_t i=0; i< number_of_layers; i++)
		{
			// Extract major, minor and patch version of spec.
			uint32_t spec_major = VK_VERSION_MAJOR(instance_layer_properties[i].specVersion);
			uint32_t spec_minor = VK_VERSION_MINOR(instance_layer_properties[i].specVersion);
			uint32_t spec_patch = VK_VERSION_PATCH(instance_layer_properties[i].specVersion);

			cout << "Name: "         << instance_layer_properties[i].layerName << endl;
			cout << "Spec Version: " << spec_major << "." << spec_minor << "." << spec_patch << endl;
			cout << "Impl Version: " << instance_layer_properties[i].implementationVersion << endl;
			cout << "Description: "  << instance_layer_properties[i].description << endl;
			cout << endl;
		}
		
		cout << endl;
	}


	void InexorRenderer::print_instance_extensions()
	{
		uint32_t number_of_extensions = 0;

		vkEnumerateInstanceExtensionProperties(NULL, &number_of_extensions, NULL);

		cout << "--------------------------------------------------------------------------" << endl;
		cout << "Number of extensions: " << number_of_extensions << endl;
		cout << "--------------------------------------------------------------------------" << endl;

		std::vector<VkExtensionProperties> extensions;

		// Preallocate memory for extension properties.
		extensions.resize(number_of_extensions);

		vkEnumerateInstanceExtensionProperties(NULL, &number_of_extensions, extensions.data());

		for(std::size_t i=0; i<number_of_extensions; i++)
		{
			cout << "Name: " << extensions[i].extensionName << endl;
			cout << "Spec: " << extensions[i].specVersion << endl;
			cout << endl;
		}

		cout << endl;
	}

	
	void InexorRenderer::print_device_layers(const VkPhysicalDevice& graphics_card)
	{
		uint32_t number_of_device_layers = 0;
		vkEnumerateDeviceLayerProperties(graphics_card, &number_of_device_layers, NULL);

		cout << "--------------------------------------------------------------------------" << endl;
		cout << "Number of device layers: " << number_of_device_layers << endl;
		cout << "--------------------------------------------------------------------------" << endl;

		std::vector<VkLayerProperties> device_layer_properties;

		device_layer_properties.resize(number_of_device_layers);

		vkEnumerateDeviceLayerProperties(graphics_card, &number_of_device_layers, device_layer_properties.data());

		for(std::size_t i=0; i<number_of_device_layers; i++)
		{
			uint32_t spec_major = VK_VERSION_MAJOR(device_layer_properties[i].specVersion);
			uint32_t spec_minor = VK_VERSION_MINOR(device_layer_properties[i].specVersion);
			uint32_t spec_patch = VK_VERSION_PATCH(device_layer_properties[i].specVersion);

			cout << "Name: "          << device_layer_properties[i].description << endl;
			cout << "Spec Version: "  << spec_major << "." << spec_minor << "." << spec_patch << endl;
			cout << "Impl Version : " << device_layer_properties[i].implementationVersion << endl;
			cout << "Description: "   << device_layer_properties[i].description << endl;
			cout << endl;
		}
		
		cout << endl;
	}


	void InexorRenderer::print_surface_capabilities(const VkPhysicalDevice& graphics_card)
	{
		VkSurfaceCapabilitiesKHR surface_capabilities;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(graphics_card, vulkan_surface, &surface_capabilities);

		cout << "Printing surface capabilities" << endl;

		cout << "minImageCount: "           << surface_capabilities.minImageCount << endl;
		cout << "maxImageCount: "           << surface_capabilities.maxImageCount << endl;
		cout << "currentExtent.width: "     << surface_capabilities.currentExtent.width << endl;
		cout << "currentExtent.height: "    << surface_capabilities.currentExtent.height << endl;
		cout << "minImageExtent.width: "    << surface_capabilities.minImageExtent.width << endl;
		cout << "minImageExtent.height: "   << surface_capabilities.minImageExtent.height << endl;
		cout << "maxImageExtent.width: "    << surface_capabilities.maxImageExtent.width << endl;
		cout << "maxImageExtent.height: "   << surface_capabilities.maxImageExtent.height << endl;
		cout << "maxImageArrayLayers: "     << surface_capabilities.maxImageArrayLayers << endl;
		cout << "supportedTransforms: "     << surface_capabilities.supportedTransforms << endl;
		cout << "currentTransform: "        << surface_capabilities.currentTransform << endl;
		cout << "supportedCompositeAlpha: " << surface_capabilities.supportedCompositeAlpha << endl;
		cout << "supportedUsageFlags: "     << surface_capabilities.supportedUsageFlags << endl;
		cout << endl;
	}


	void InexorRenderer::print_supported_surface_formats(const VkPhysicalDevice& graphics_card)
	{
		uint32_t number_of_supported_formats = 0;
		
		// First check how many formats are supported
		vkGetPhysicalDeviceSurfaceFormatsKHR(graphics_card, vulkan_surface, &number_of_supported_formats, nullptr);

		cout << "Supported surface formats: " << number_of_supported_formats << endl;

		std::vector<VkSurfaceFormatKHR> surface_formats(number_of_supported_formats);

		vkGetPhysicalDeviceSurfaceFormatsKHR(graphics_card, vulkan_surface, &number_of_supported_formats, surface_formats.data());

		for(std::size_t i=0; i<number_of_supported_formats; i++)
		{
			// We will not print the text which corresponds to the format.
			// You can look it up in the official Vulkan documentation.
			cout << surface_formats[i].format << endl;
		}
	}

	
	void InexorRenderer::print_presentation_modes(const VkPhysicalDevice& graphics_card)
	{
		uint32_t number_of_present_modes = 0;
		
		// First check how many presentation modes are available.
		vkGetPhysicalDeviceSurfacePresentModesKHR(graphics_card, vulkan_surface, &number_of_present_modes, nullptr);
	
		cout << "Available present modes: " << number_of_present_modes << endl;

		std::vector<VkPresentModeKHR> present_modes(number_of_present_modes);

		vkGetPhysicalDeviceSurfacePresentModesKHR(graphics_card, vulkan_surface, &number_of_present_modes, present_modes.data());

		for(std::size_t i=0; i<number_of_present_modes; i++)
		{
			cout << present_modes[i] << endl;
		}
	}
	

	void InexorRenderer::setup_swap_chain()
	{
		cout << "Creating swap chain." << endl;

		VkSwapchainCreateInfoKHR swap_chain_create_info = {};

		swap_chain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swap_chain_create_info.pNext = nullptr;
		swap_chain_create_info.flags = 0;
		swap_chain_create_info.surface = vulkan_surface;

		// TODO: Check if system supports the number specified here!
		swap_chain_create_info.minImageCount = 3;

		// TODO: Check if system supports this image format!
		swap_chain_create_info.imageFormat = VK_FORMAT_B8G8R8A8_UNORM;

		// TODO: Check if system supports this image color space!
		swap_chain_create_info.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

		swap_chain_create_info.imageExtent = VkExtent2D{window_width, window_height};
		swap_chain_create_info.imageArrayLayers = 1;
		swap_chain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		// TODO: Check if system supports this image sharing mode!
		swap_chain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swap_chain_create_info.queueFamilyIndexCount = 0;
		swap_chain_create_info.pQueueFamilyIndices = nullptr;

		swap_chain_create_info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		swap_chain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

		// TODO: Check if there is a better presentation mode available!
		// An alternative would be VK_PRESENT_MODE_MAILBOX_KHR.
		swap_chain_create_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;

		swap_chain_create_info.clipped = VK_TRUE;

		// TODO: Make window resizable and recreate swap chain.
		// When recreating the swap chain this is needed.
		swap_chain_create_info.oldSwapchain = VK_NULL_HANDLE;

		VkResult result = vkCreateSwapchainKHR(vulkan_device, &swap_chain_create_info, nullptr, &vulkan_swapchain);
		vulkan_error_check(result);

		vkGetSwapchainImagesKHR(vulkan_device, vulkan_swapchain, &number_of_images_in_swap_chain, nullptr);

		cout << "Images in swap chain: " << number_of_images_in_swap_chain << endl;

		std::vector<VkImage> swapchain_images(number_of_images_in_swap_chain);

		result = vkGetSwapchainImagesKHR(vulkan_device, vulkan_swapchain, &number_of_images_in_swap_chain, swapchain_images.data());
		vulkan_error_check(result);

		// Preallocate memory for the image views.
		image_views.resize(number_of_images_in_swap_chain);
	
		VkImageViewCreateInfo image_view_create_info = {};
		image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		image_view_create_info.pNext = nullptr;
		image_view_create_info.flags = 0;
		image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		
		// TODO: Check if system supports this image format!
		image_view_create_info.format = VK_FORMAT_B8G8R8A8_UNORM;

		image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

		// TODO: Implement mip-mapping?
		image_view_create_info.subresourceRange.baseMipLevel = 0;
		image_view_create_info.subresourceRange.levelCount = 1;

		image_view_create_info.subresourceRange.baseArrayLayer = 0;

		// TODO: Implement awesome stereographic VR textures?
		image_view_create_info.subresourceRange.layerCount = 1;


		for(std::size_t i=0; i<number_of_images_in_swap_chain; i++)
		{
			image_view_create_info.image = swapchain_images[i];

			result = vkCreateImageView(vulkan_device, &image_view_create_info, nullptr, &image_views[i]);
			vulkan_error_check(result);
		}
	}
	

	void InexorRenderer::create_shader_module(const std::vector<char>& SPIRV_shader_bytes, VkShaderModule* shader_module)
	{
		VkShaderModuleCreateInfo shader_create_info = {};
		shader_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shader_create_info.pNext = nullptr;
		shader_create_info.flags = 0;
		shader_create_info.codeSize = SPIRV_shader_bytes.size();
		
		// TODO: Is this the right type of cast for this?
		shader_create_info.pCode = reinterpret_cast<const uint32_t*>(SPIRV_shader_bytes.data());

		// Create the shader module.
		VkResult result = vkCreateShaderModule(vulkan_device, &shader_create_info, nullptr, shader_module);

		vulkan_error_check(result);
	}


	void InexorRenderer::create_shader_module_from_file(const std::string& SPIRV_file_name, VkShaderModule* shader_module)
	{
		cout << "Creating shader module: " << SPIRV_file_name.c_str() << endl;

		VulkanShader vulkan_shader;
		vulkan_shader.load_file(SPIRV_file_name);
		
		if(vulkan_shader.file_size > 0)
		{
			create_shader_module(vulkan_shader.file_data, shader_module);
		}
		else
		{
			cout << "Error: Shader file is empty!" << endl;
		}
	}


	bool InexorRenderer::init_vulkan()
	{
		cout << "Initialising Vulkan instance." << endl;

		VkResult result = create_vulkan_instance(INEXOR_APPLICATION_NAME, INEXOR_ENGINE_NAME, INEXOR_APPLICATION_VERSION, INEXOR_ENGINE_VERSION, true);

		vulkan_error_check(result);

		// List up all GPUs that are available on this system and print their stats.
		enumerate_physical_devices();

		// Let's just use the first one in the array for now.
		// TODO: Implement a mechanism to select a graphics card.
		// TODO: In case multiple graphics cards are available let the user select one.
		VkPhysicalDevice selected_graphics_card = graphics_cards[0];

		result = create_physical_device(selected_graphics_card);

		vulkan_error_check(result);

		print_instance_layer_properties();
		print_instance_extensions();
		print_device_layers(selected_graphics_card);

		// Query if presentation is supported.
		VkBool32 surface_support = false;
		result = vkGetPhysicalDeviceSurfaceSupportKHR(selected_graphics_card, 0, vulkan_surface, &surface_support);

		vulkan_error_check(result);

		if(!surface_support)
		{
			display_error_message("Error: Surface not supported!");
		}

		cout << "Presentation is supported." << endl;

		VkQueue queue;
		vkGetDeviceQueue(vulkan_device, 0, 0, &queue);

		setup_swap_chain();

		// TODO: Setup shaders from JSON file?
		
		// Load shaders from file.
		// Make sure your debug directory contains these files!
		create_shader_module_from_file("vertex_shader.spv", &vertex_shader);
		create_shader_module_from_file("fragment_shader.spv", &fragment_shader);

		return true;
	}


	void InexorRenderer::print_graphics_card_info(const VkPhysicalDevice& graphics_card)
	{
		// The properties of the graphics card.
		VkPhysicalDeviceProperties graphics_card_properties;

		// Get the information about that graphics card.
		vkGetPhysicalDeviceProperties(graphics_card, &graphics_card_properties);

		// Print the name of the graphics card.
		cout << "Graphics card: " << graphics_card_properties.deviceName << endl;

		uint32_t VulkanAPIversion = graphics_card_properties.apiVersion;

		// The Vulkan version which is supported by the graphics card.
		cout << "Vulkan API supported version: " << VK_VERSION_MAJOR(VulkanAPIversion) << "." << VK_VERSION_MINOR(VulkanAPIversion) << "." << VK_VERSION_PATCH(VulkanAPIversion) << endl;

		// The driver version.
		// Always keep your graphics drivers up to date!
		// Note: The driver version format is NOT standardised!
		cout << "Driver version: " << VK_VERSION_MAJOR(graphics_card_properties.driverVersion) << "." << VK_VERSION_MINOR(graphics_card_properties.driverVersion) << "." << VK_VERSION_PATCH(graphics_card_properties.driverVersion) << endl;
		cout << "Vendor ID: " << graphics_card_properties.vendorID << endl;
		cout << "Device ID: " << graphics_card_properties.deviceID << endl;

		// Graphics card types.
		// TODO: Is there any other way to get the graphics card type name by id?
		const std::string graphics_card_types[] = {
			"VK_PHYSICAL_DEVICE_TYPE_OTHER",
			"VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU",
			"VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU",
			"VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU",
			"VK_PHYSICAL_DEVICE_TYPE_CPU",
		};

		// Check if array index is in bounds.
		if(graphics_card_properties.deviceType <= 4)
		{
			cout << "Device type: " << graphics_card_types[graphics_card_properties.deviceType] << endl;
		}

		// From the Vulkan documentation:
		// The number of discrete priorities that can be assigned to a queue based on the value of each member
		// of VkDeviceQueueCreateInfo::pQueuePriorities.This must be at least 2, and levels must be spread evenly
		// over the range, with at least one level at 1.0, and another at 0.0.
		cout << "Discrete queue priorities: " << graphics_card_properties.limits.discreteQueuePriorities << endl;

		VkPhysicalDeviceFeatures graphics_card_features;

		// Check which features are supported by this graphics card.
		vkGetPhysicalDeviceFeatures(graphics_card, &graphics_card_features);

		// We will only print some of the features in the structure.
		// For more information check the Vulkan documentation.
		// Check if geometry shader is supported.
		cout << "Geometry shader supported: " << ((graphics_card_features.geometryShader) ? "yes" : "no") << endl;

		// TODO: Check for more features if neccesary.

		cout << endl;
		cout << "Checking memory properties." << endl;

		// Check memory properties of this graphics card.
		VkPhysicalDeviceMemoryProperties graphics_card_memory_properties;

		vkGetPhysicalDeviceMemoryProperties(graphics_card, &graphics_card_memory_properties);

		cout << "Number of memory types: " << graphics_card_memory_properties.memoryTypeCount << endl;
		cout << "Number of heap types: " << graphics_card_memory_properties.memoryHeapCount << endl;

		// Loop through all memory types and list their features.
		for(std::size_t i=0; i<graphics_card_memory_properties.memoryTypeCount; i++)
		{
			cout << "Heap index: "<< graphics_card_memory_properties.memoryTypes[i].heapIndex << endl;
			
			auto &propertyFlag = graphics_card_memory_properties.memoryTypes[i].propertyFlags;

			if(propertyFlag & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) cout << "VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT" << endl;
			if(propertyFlag & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) cout << "VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT" << endl;
			if(propertyFlag & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) cout << "VK_MEMORY_PROPERTY_HOST_COHERENT_BIT" << endl;
			if(propertyFlag & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) cout << "VK_MEMORY_PROPERTY_HOST_CACHED_BIT" << endl;
			if(propertyFlag & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT) cout << "VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT" << endl;
			if(propertyFlag & VK_MEMORY_PROPERTY_PROTECTED_BIT) cout << "VK_MEMORY_PROPERTY_PROTECTED_BIT" << endl;
			if(propertyFlag & VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD) cout << "VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD" << endl;
			if(propertyFlag & VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD) cout << "VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD" << endl;
			if(propertyFlag & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) cout << "VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT" << endl;

			cout << endl;
		}
	}


	void InexorRenderer::shutdown_vulkan()
	{
		// Important: destroy objects in reverse order of initialisation.
		vkDeviceWaitIdle(vulkan_device);

		for(std::size_t i=0; i<number_of_images_in_swap_chain; i++)
		{
			vkDestroyImageView(vulkan_device, image_views[i], nullptr);
		}

		// Destroy shader modules:
		vkDestroyShaderModule(vulkan_device, vertex_shader, nullptr);
		vkDestroyShaderModule(vulkan_device, fragment_shader, nullptr);

		vkDestroySwapchainKHR(vulkan_device, vulkan_swapchain, nullptr);
		vkDestroySurfaceKHR(vulkan_instance, vulkan_surface, nullptr);
		vkDestroyDevice(vulkan_device, nullptr);
		vkDestroyInstance(vulkan_instance, nullptr);
	}


	InexorRenderer::InexorRenderer()
	{
		window = nullptr;
		vulkan_instance = {};
		vulkan_device = {};
		number_of_physical_devices = 0;
		graphics_cards.clear();
		window_width = 0;
		window_height = 0;
		image_views.clear();
		number_of_images_in_swap_chain = 0;
	}


	InexorRenderer::~InexorRenderer()
	{
	}


	void InexorRenderer::init()
	{
		init_window(800, 600, "Inexor Vulkan Renderer");
		init_vulkan();
	}


	void InexorRenderer::run()
	{
		// TODO: Run this in a separated thread?
		while(!glfwWindowShouldClose(window))
		{
			glfwPollEvents();
		}
	}


	void InexorRenderer::cleanup()
	{
		shutdown_vulkan();
		shutdown_window();
	}


};
};
