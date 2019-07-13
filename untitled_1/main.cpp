// clang-format off
#include <tuple>
#include <vector>
#include <memory>
#include <fstream>
#include <iostream>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <naive_vulkan/vulkan.hpp>
// clang-format on


class GraphicBase
{
const bool enableValidationLayers = true;
const std::vector<const char *> validationLayers = {
            "VK_LAYER_LUNARG_standard_validation"
    };
const std::vector<const char*> deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

public:
  GraphicBase()
  {
    auto initWindow = [&]() -> GLFWwindow * {
      glfwInit();

      glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
      glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

      return glfwCreateWindow(640, 480, "Vulkan", nullptr, nullptr);
    };
    GLFWwindow *window = initWindow();

    // --------------------------------
    // ------ step 0: VkInstance ------
    // --------------------------------
    auto createInstance = [&]() -> VkInstance {
      // Information about our application.
      // Optional, driver could use this to optimize for specific app.
      VkApplicationInfo appInfo = {};
      appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
      appInfo.pApplicationName = "Hello Triangle";
      appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
      appInfo.pEngineName = "No Engine";
      appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
      appInfo.apiVersion = VK_API_VERSION_1_1;

      // Information of extensions
      VkInstanceCreateInfo createInfo = {};
      createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
      createInfo.pApplicationInfo = &appInfo;

      // extensions
      auto getRequiredExtensions = [&]() -> std::vector<const char *> {
        uint32_t glfwExtensionCount = 0;
        const char **glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char *> extensions(
            glfwExtensions, glfwExtensions + glfwExtensionCount);
          
        if (enableValidationLayers) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
      };
      auto extensions = getRequiredExtensions();
      createInfo.enabledExtensionCount =
          static_cast<uint32_t>(extensions.size());
      createInfo.ppEnabledExtensionNames = extensions.data();

      if (enableValidationLayers) {
          createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
          createInfo.ppEnabledLayerNames = validationLayers.data();
        } else {
          createInfo.enabledLayerCount = 0;
        }

      // Create
      VkInstance instance;
      if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
      {
        throw std::runtime_error("failed to create instance!");
      }
      std::cout << "create instance OK" << std::endl;

      return instance;
    };
    VkInstance instance = createInstance();

    //
    auto createSurface = [&]() -> VkSurfaceKHR {
      VkSurfaceKHR surface;
      if (glfwCreateWindowSurface(instance, window, nullptr, &surface) !=
          VK_SUCCESS)
      {
        throw std::runtime_error("failed to create window surface!");
      }
      return surface;
    };
    VkSurfaceKHR surface = createSurface();

    // --------------------------------
    // --- step 1: VkPhysicalDevice ---
    // --------------------------------
    auto initPhysicalDevice =
        [&]() -> std::tuple<uint32_t, uint32_t, VkPhysicalDevice> {
      // Count devices
      uint32_t deviceCount = 0;
      vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
      if (deviceCount == 0)
      {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
      }

      // Pick one with graphics and compute pipeline support
      std::vector<VkPhysicalDevice> devices(deviceCount);
      vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

      auto isDeviceSuitable = [](const VkPhysicalDevice &device)
          -> std::tuple<bool, uint32_t, uint32_t> {
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
                                                 nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
                                                 queueFamilies.data());

        // graphic queue family
        uint32_t graphicQueueIndex = 0;
        for (const auto &queueFamily : queueFamilies)
        {
          if (queueFamily.queueCount > 0 &&
              queueFamily.queueFlags &
                  (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT |
                   VK_QUEUE_TRANSFER_BIT))
          {
            break;
          }
          graphicQueueIndex += 1;
        }
        if (graphicQueueIndex == queueFamilies.size())
        {
          throw std::runtime_error("failed to create command pool!");
        }

        // present queue family
        uint32_t presentQueueIndex = 0;
        for (const auto &queueFamily : queueFamilies)
        {
          if (queueFamily.queueCount > 0 &&
              queueFamily.queueFlags &
                  (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT |
                   VK_QUEUE_TRANSFER_BIT))
          {
            break;
          }
          presentQueueIndex += 1;
        }
        if (presentQueueIndex == queueFamilies.size())
        {
          throw std::runtime_error("failed to create command pool!");
        }

        return std::make_tuple(true, graphicQueueIndex, presentQueueIndex);
      };

      uint32_t graphicQueueIndex;
      uint32_t presentQueueIndex;
      VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
      for (const auto &device : devices)
      {
        bool foundDevice;
        std::tie(foundDevice, graphicQueueIndex, presentQueueIndex) =
            isDeviceSuitable(device);
        if (foundDevice)
        {
          physicalDevice = device;
          break;
        }
      }
      if (physicalDevice == VK_NULL_HANDLE)
      {
        throw std::runtime_error("failed to find a suitable GPU!");
      }

      // Information of device
      VkPhysicalDeviceProperties deviceProperties;
      vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
      std::cout << "apiVersion: " << deviceProperties.apiVersion << std::endl;
      std::cout << "driverVersion: " << deviceProperties.driverVersion
                << std::endl;
      std::cout << "deviceType: " << deviceProperties.deviceType << std::endl;
      std::cout << "deviceName: " << deviceProperties.deviceName << std::endl;
      std::cout << "initialize physical device OK" << std::endl;

      return std::make_tuple(graphicQueueIndex, presentQueueIndex,
                             physicalDevice);
    };
    uint32_t graphicQueueIndex;
    uint32_t presentQueueIndex;
    VkPhysicalDevice physicalDevice;
    std::tie(graphicQueueIndex, presentQueueIndex, physicalDevice) =
        initPhysicalDevice();

    // --------------------------------
    // --- step 2: VkDevice VkQueue ---
    // --------------------------------
    auto createLogicalDevice = [&]() -> std::tuple<VkDevice, VkQueue> {
      // Specifying the queues to be created
      VkDeviceQueueCreateInfo queueCreateInfo = {};
      queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      queueCreateInfo.queueFamilyIndex = graphicQueueIndex;
      queueCreateInfo.queueCount = 1;
      float queuePriority = 1.0f;
      queueCreateInfo.pQueuePriorities = &queuePriority;

      // Specifying used device features
      VkPhysicalDeviceFeatures deviceFeatures = {};

      // Infomation of the logical device
      VkDeviceCreateInfo createInfo = {};
      createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
      createInfo.pQueueCreateInfos = &queueCreateInfo;
      createInfo.queueCreateInfoCount = 1;
      createInfo.pEnabledFeatures = &deviceFeatures;

      createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();

        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        } else {
            createInfo.enabledLayerCount = 0;
        }

      VkDevice device;
      if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) !=
          VK_SUCCESS)
      {
        throw std::runtime_error("failed to create logical device!");
      }
      std::cout << "create device OK" << std::endl;

      VkQueue graphicsQueue;
      vkGetDeviceQueue(device, 0, 0, &graphicsQueue);
      std::cout << "create queue OK" << std::endl;

      return std::make_tuple(device, graphicsQueue);
    };
    VkDevice device;
    VkQueue graphicsQueue;
    std::tie(device, graphicsQueue) = createLogicalDevice();

    auto createSwapChain = [&]() {
      // prepare
      VkSurfaceCapabilitiesKHR capabilities;
      vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
          physicalDevice, surface, &capabilities);

      uint32_t formatCount;
      vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface,
                                                    &formatCount, nullptr);
      std::vector<VkSurfaceFormatKHR> formats;
      formats.resize(formatCount);
      vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface,
                                           &formatCount, formats.data());

      uint32_t presentModeCount;
      auto a = vkGetPhysicalDeviceSurfacePresentModesKHR(
          physicalDevice, surface, &presentModeCount, nullptr);
      std::vector<VkPresentModeKHR> presentModes;
      presentModes.resize(presentModeCount);
      vkGetPhysicalDeviceSurfacePresentModesKHR(
          physicalDevice, surface, &presentModeCount, presentModes.data());

      // format
      auto chooseSwapSurfaceFormat =
          [](const std::vector<VkSurfaceFormatKHR> &availableFormats) {
            for (const auto &availableFormat : availableFormats)
            {
              if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM &&
                  availableFormat.colorSpace ==
                      VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
              {
                return availableFormat;
              }
            }
            return availableFormats[0];
          };
      VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(formats);

      // present mode
      auto chooseSwapPresentMode =
          [](const std::vector<VkPresentModeKHR> &availablePresentModes) {
            VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

            for (const auto &availablePresentMode : availablePresentModes)
            {
              if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
              {
                return availablePresentMode;
              }
              else if (availablePresentMode ==
                       VK_PRESENT_MODE_IMMEDIATE_KHR)
              {
                bestMode = availablePresentMode;
              }
            }
            return bestMode;
          };
      VkPresentModeKHR presentMode = chooseSwapPresentMode(presentModes);

      /*
      VkSurfaceFormatKHR surfaceFormat;
      surfaceFormat.format = VK_FORMAT_B8G8R8A8_UNORM;
      surfaceFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
      VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
      */

      // swap extent
      auto chooseSwapExtent = [](const VkSurfaceCapabilitiesKHR &capabilities) {
        if (capabilities.currentExtent.width !=
            std::numeric_limits<uint32_t>::max())
        {
          return capabilities.currentExtent;
        }
        else
        {
          VkExtent2D actualExtent = {640, 480};

          actualExtent.width = std::max(
              capabilities.minImageExtent.width,
              std::min(capabilities.maxImageExtent.width, actualExtent.width));
          actualExtent.height =
              std::max(capabilities.minImageExtent.height,
                       std::min(capabilities.maxImageExtent.height,
                                actualExtent.height));

          return actualExtent;
        }
      };
      VkExtent2D extent = chooseSwapExtent(capabilities);

      // image count
      uint32_t imageCount = capabilities.minImageCount + 1;
      if (capabilities.maxImageCount > 0 &&
          imageCount > capabilities.maxImageCount)
      {
        imageCount = capabilities.maxImageCount;
      }

      // create info
      VkSwapchainCreateInfoKHR createInfo = {};
      createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
      createInfo.surface = surface;
      createInfo.minImageCount = imageCount;
      createInfo.imageFormat = surfaceFormat.format;
      createInfo.imageColorSpace = surfaceFormat.colorSpace;
      createInfo.imageExtent = extent;
      createInfo.imageArrayLayers = 1;
      createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
      if (graphicQueueIndex == presentQueueIndex)
      {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
      }
      else
      {
        uint32_t queueIndexes[] = {graphicQueueIndex, presentQueueIndex};
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueIndexes;
      }
      createInfo.preTransform = capabilities.currentTransform;
      createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
      createInfo.presentMode = presentMode;
      createInfo.clipped = VK_TRUE;
      createInfo.oldSwapchain = VK_NULL_HANDLE;

      // create
      VkSwapchainKHR swapChain;
      if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) !=
          VK_SUCCESS)
      {
        throw std::runtime_error("failed to create swap chain!");
      }

      // images
      std::vector<VkImage> swapChainImages;
      vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
      swapChainImages.resize(imageCount);
      vkGetSwapchainImagesKHR(device, swapChain, &imageCount,
                              swapChainImages.data());

      //
      VkFormat swapChainImageFormat = surfaceFormat.format;
      VkExtent2D swapChainExtent = extent;

      std::cout << "create swapchain OK" << std::endl;
    };
    createSwapChain();
    // std::vector<VkImageView> swapChainImageViews;
  }
};

int main(int argc, char **argv)
{
  // ----------
  // GraphicBase();
  // return 0;
  // ----------
  // compute
  {
    auto instance = vk::createInstance();
    std::cout << "1. Instance ready" << std::endl;

    auto device = instance->getComputeDevice();
    std::cout << "2. Device ready" << std::endl;

    auto shader = device->createShader("./shaders/comp_1.spv",
                                       VK_SHADER_STAGE_COMPUTE_BIT);
    std::cout << "3. Shader ready" << std::endl;

    auto pipeline = device->createComputePipeline(shader, {{std::make_tuple(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)}});
    std::cout << "4. Pipeline ready" << std::endl;

    auto buffer = device->createBuffer(1024, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    pipeline->feedBuffer(0, 0, buffer, 0, 1024);
    std::cout << "5. Buffer ready" << std::endl;

    auto command = pipeline->createCommand();
    auto fence = command->submit();
    std::cout << "6. Command ready" << std::endl;

    fence->wait();
    std::cout << "7. Fence ready" << std::endl;

    buffer->print();
    std::cout << "8. Finish" << std::endl;
  }

  // graphic
  /*
  {
    auto instance = vk::createInstance();
    std::cout << "1. Instance ready" << std::endl;

    auto device = instance->getGraphicDevice();
    std::cout << "2. Device ready" << std::endl;
  }
  */
  return 0;
}
