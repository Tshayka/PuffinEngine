#pragma once

#include "../headers/Device.hpp"

namespace puffinengine {
	namespace tool {
		class RenderPass {
		public:
			RenderPass();
			~RenderPass();

			bool m_Initialized = false;

			enum class Type { 
				SCREEN, 
				OFFSCREEN
			};
			
			bool init(Device* device, const RenderPass::Type type);
			void deInit();

			bool createRenderPass(const RenderPass::Type type);

			VkRenderPass get() const;
			VkFormat getFormat() const;

		private:
			VkRenderPass m_RenderPass;
			VkFormat m_DepthFormat;

			Device* p_Device = nullptr;
		};
	} // namespace tool
} // namespace puffinengine