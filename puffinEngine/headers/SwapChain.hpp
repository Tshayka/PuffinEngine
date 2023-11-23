#pragma once

#include "../headers/Device.hpp"

namespace puffinengine {
	namespace tool {
		class SwapChain {
		public:
			SwapChain();
			~SwapChain();

			bool m_Initialized = false;

			bool init(Device* device, GLFWwindow* window);
			void deInit();

			bool createSwapChain();
			void initSwapchainImageViews();
			void deInitSwapchainImageViews();

			VkSwapchainKHR get() const;
			const std::vector<VkImageView>& getSwapchainImageViews();
			VkFormat getSwapchainImageFormat() const;
			VkExtent2D getExtent() const;
			uint32_t& getImageIndex();
			VkResult acquireNextImage(VkSemaphore& imageAvailableSemaphore);

		private:
			Device* p_Device;
			GLFWwindow* p_Window;
			VkSwapchainKHR m_SwapChain;
			std::vector<VkImage> m_SwapchainImages;
			std::vector<VkImageView> m_SwapchainImageViews;
			VkFormat m_SwapchainImageFormat;
			VkExtent2D m_SwapChainExtent;
			uint32_t m_ImageIndex;

			VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
			VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
			VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
			VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags);
		};
	} // namespace tool
} // namespace puffinengine