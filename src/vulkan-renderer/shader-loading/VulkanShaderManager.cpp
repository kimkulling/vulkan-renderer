#include "VulkanShaderManager.hpp"


namespace inexor {
namespace vulkan_renderer {

	
	VulkanShaderManager::VulkanShaderManager()
	{
	}


	VulkanShaderManager::~VulkanShaderManager()
	{
	}


	void VulkanShaderManager::create_shader_module(const VkDevice& vulkan_device, const std::vector<char>& SPIRV_shader_bytes, VkShaderModule* shader_module)
	{
		VkShaderModuleCreateInfo shader_create_info = {};
		
		shader_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shader_create_info.pNext = nullptr;
		shader_create_info.flags = 0;
		shader_create_info.codeSize = SPIRV_shader_bytes.size();
		shader_create_info.pCode = reinterpret_cast<const uint32_t*>(SPIRV_shader_bytes.data());

		VkResult result = vkCreateShaderModule(vulkan_device, &shader_create_info, nullptr, shader_module);
		vulkan_error_check(result);
	}


	void VulkanShaderManager::create_shader_module_from_file(const VkDevice& vulkan_device, const std::string& SPIRV_file_name, VkShaderModule* shader_module)
	{
		cout << "Creating shader from file: " << SPIRV_file_name.c_str() << endl;

		VulkanShader vulkan_shader;

		vulkan_shader.load_file(SPIRV_file_name);
		
		if(0 == vulkan_shader.file_size)
		{
			std::string error_message = "Error: SPIR-V shader file " + SPIRV_file_name + "is empty!";
			display_error_message(error_message);		}

		create_shader_module(vulkan_device, vulkan_shader.file_data, shader_module);
	}


};
};