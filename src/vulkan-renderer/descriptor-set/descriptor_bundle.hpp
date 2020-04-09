#pragma once

#include "../descriptor-pool/InexorDescriptorPool.hpp"

#include <vulkan/vulkan.h>

#include <string>
#include <vector>
#include <memory>


namespace inexor {
namespace vulkan_renderer {


	struct InexorDescriptorBundle
	{
		public:


			// Force to use overloaded constructor!
			InexorDescriptorBundle() = delete;


			/// @brief Allow name and descriptor pool to be set in constructor only!
			InexorDescriptorBundle(const std::string& internal_descriptor_set_name,
			                       std::shared_ptr<InexorDescriptorPool> descriptor_pool)
				: name(internal_descriptor_set_name),
				  associated_descriptor_pool(descriptor_pool)
			{
			}
			

			const std::string name;

			const std::shared_ptr<InexorDescriptorPool> associated_descriptor_pool;

			VkDescriptorSetLayout descriptor_set_layout;

			std::vector<VkDescriptorSet> descriptor_sets;
			
			std::vector<VkWriteDescriptorSet> write_descriptor_sets;
			
			std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings;


	};


};
};