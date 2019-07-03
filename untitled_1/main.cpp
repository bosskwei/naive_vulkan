// clang-format off
#include <tuple>
#include <vector>
#include <memory>
#include <fstream>
#include <iostream>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
//#include <vulkan/vulkan.h>
// clang-format on

class VulkanBase
{
public:
  VulkanBase()
  {
    // --------------------------------
    // ------ step 0: VkInstance ------
    // --------------------------------
    auto createInstance = []() -> VkInstance {
      // Information about our application.
      // Optional, driver could use this to optimize for specific app.
      VkApplicationInfo appInfo = {};
      appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
      appInfo.pApplicationName = "Hello Triangle";
      appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
      appInfo.pEngineName = "No Engine";
      appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
      appInfo.apiVersion = VK_API_VERSION_1_0;

      // Information of extensions
      VkInstanceCreateInfo createInfo = {};
      createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
      createInfo.pApplicationInfo = &appInfo;

      // Create
      VkInstance instance;
      if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
      {
        throw std::runtime_error("failed to create instance!");
      }
      std::cout << "Instance creation OK" << std::endl;

      return instance;
    };
    VkInstance instance = createInstance();

    // --------------------------------
    // --- step 1: VkPhysicalDevice ---
    // --------------------------------
    auto initPhysicalDevice = [&]() -> std::tuple<uint32_t, VkPhysicalDevice> {
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

      auto isDeviceSuitable =
          [](const VkPhysicalDevice &device) -> std::tuple<bool, uint32_t> {
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
                                                 nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
                                                 queueFamilies.data());

        uint32_t index = 0;
        for (const auto &queueFamily : queueFamilies)
        {
          if (queueFamily.queueCount > 0 &&
              queueFamily.queueFlags &
                  (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT |
                   VK_QUEUE_TRANSFER_BIT))
          {
            break;
          }
          index += 1;
        }
        if (index == queueFamilies.size())
        {
          throw std::runtime_error("failed to create command pool!");
        }

        return std::make_tuple(true, index);
      };

      uint32_t queueFamilyIndex;
      VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
      for (const auto &device : devices)
      {
        bool foundDevice;
        std::tie(foundDevice, queueFamilyIndex) = isDeviceSuitable(device);
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
      std::cout << "Physical Device initialization OK" << std::endl;

      return std::make_tuple(queueFamilyIndex, physicalDevice);
    };
    uint32_t queueFamilyIndex;
    VkPhysicalDevice physicalDevice;
    std::tie(queueFamilyIndex, physicalDevice) = initPhysicalDevice();

    // --------------------------------
    // --- step 2: VkDevice VkQueue ---
    // --------------------------------
    auto createLogicalDevice = [&]() -> std::tuple<VkDevice, VkQueue> {
      // Specifying the queues to be created
      VkDeviceQueueCreateInfo queueCreateInfo = {};
      queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
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
      createInfo.enabledExtensionCount = 0;
      createInfo.enabledLayerCount = 0;

      VkDevice device;
      if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) !=
          VK_SUCCESS)
      {
        throw std::runtime_error("failed to create logical device!");
      }
      std::cout << "Device creation OK" << std::endl;

      VkQueue graphicsQueue;
      vkGetDeviceQueue(device, 0, 0, &graphicsQueue);
      std::cout << "Queue creation OK" << std::endl;

      return std::make_tuple(device, graphicsQueue);
    };
    VkDevice device;
    VkQueue graphicsQueue;
    std::tie(device, graphicsQueue) = createLogicalDevice();

    // --------------------------------
    // --- step 3: Allocate Buffer  ---
    // --------------------------------
    auto createBuffer =
        [&](uint32_t size) -> std::tuple<VkBuffer, VkDeviceMemory> {
      VkBufferCreateInfo bufferCreateInfo = {};
      bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
      bufferCreateInfo.size = size;
      bufferCreateInfo.usage =
          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT; // buffer is used as a storage
                                              // buffer.
      bufferCreateInfo.sharingMode =
          VK_SHARING_MODE_EXCLUSIVE; // buffer is exclusive to a single queue
                                     // family at a time.

      VkBuffer buffer;
      if (vkCreateBuffer(device, &bufferCreateInfo, NULL, &buffer) !=
          VK_SUCCESS)
      {
        throw std::runtime_error("failed to create buffers!");
      }

      VkMemoryRequirements memoryRequirements;
      vkGetBufferMemoryRequirements(device, buffer, &memoryRequirements);

      VkMemoryAllocateInfo allocateInfo = {};
      allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
      allocateInfo.allocationSize = memoryRequirements.size;

      VkDeviceMemory bufferMemory;
      if (vkAllocateMemory(device, &allocateInfo, nullptr, &bufferMemory) !=
          VK_SUCCESS)
      {
        throw std::runtime_error("failed to allocate buffer memory!");
      }
      vkBindBufferMemory(device, buffer, bufferMemory, 0);
      std::cout << "Bind Buffer OK" << std::endl;

      return std::make_tuple(buffer, bufferMemory);
    };
    VkBuffer buffer;
    VkDeviceMemory bufferMemory;
    std::tie(buffer, bufferMemory) = createBuffer(1024);

    // --------------------------------
    // --- step 4: Binding Buffer   ---
    // --------------------------------
    auto createDescriptorSet =
        [&]() -> std::tuple<VkDescriptorPool, VkDescriptorSet,
                            VkDescriptorSetLayout> {
      // Pool
      VkDescriptorPoolSize descriptorPoolSize = {};
      descriptorPoolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
      descriptorPoolSize.descriptorCount = 1;

      VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
      descriptorPoolCreateInfo.sType =
          VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
      descriptorPoolCreateInfo.maxSets =
          1; // we only need to allocate one descriptor set from the pool.
      descriptorPoolCreateInfo.poolSizeCount = 1;
      descriptorPoolCreateInfo.pPoolSizes = &descriptorPoolSize;

      VkDescriptorPool descriptorPool;
      if (vkCreateDescriptorPool(device, &descriptorPoolCreateInfo, NULL,
                                 &descriptorPool) != VK_SUCCESS)
      {
        throw std::runtime_error("failed to create descriptor pool!");
      }

      // Layout
      VkDescriptorSetLayoutBinding descriptorSetLayoutBinding = {};
      descriptorSetLayoutBinding.binding = 0;
      descriptorSetLayoutBinding.descriptorType =
          VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
      descriptorSetLayoutBinding.descriptorCount = 1;
      descriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

      VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
      descriptorSetLayoutCreateInfo.sType =
          VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
      descriptorSetLayoutCreateInfo.bindingCount = 1;
      descriptorSetLayoutCreateInfo.pBindings = &descriptorSetLayoutBinding;

      VkDescriptorSetLayout descriptorSetLayout;
      if (vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCreateInfo,
                                      NULL,
                                      &descriptorSetLayout) != VK_SUCCESS)
      {
        throw std::runtime_error("failed to create descriptor!");
      }

      // Descriptor
      VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
      descriptorSetAllocateInfo.sType =
          VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
      descriptorSetAllocateInfo.descriptorPool = descriptorPool;
      descriptorSetAllocateInfo.descriptorSetCount = 1;
      descriptorSetAllocateInfo.pSetLayouts = &descriptorSetLayout;

      VkDescriptorSet descriptorSet;
      if (vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo,
                                   &descriptorSet) != VK_SUCCESS)
      {
        throw std::runtime_error("failed to create descriptor pool!");
      }
      std::cout << "DescriptorSet OK" << std::endl;

      // Attach our buffer to this set
      VkDescriptorBufferInfo descriptorBufferInfo = {};
      descriptorBufferInfo.buffer = buffer;
      descriptorBufferInfo.offset = 0;
      descriptorBufferInfo.range = 1024;

      VkWriteDescriptorSet writeDescriptorSet = {};
      writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      writeDescriptorSet.dstSet = descriptorSet;
      writeDescriptorSet.dstBinding = 0;
      writeDescriptorSet.descriptorCount = 1;
      writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
      writeDescriptorSet.pBufferInfo = &descriptorBufferInfo;
      vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, NULL);

      // cleanup
      // vkDestroyDescriptorSetLayout(device, descriptorSetLayout, NULL);

      return std::make_tuple(descriptorPool, descriptorSet,
                             descriptorSetLayout);
    };
    VkDescriptorPool descriptorPool;
    VkDescriptorSet descriptorSet;
    VkDescriptorSetLayout descriptorSetLayout;
    std::tie(descriptorPool, descriptorSet, descriptorSetLayout) =
        createDescriptorSet();

    // --------------------------------
    // ------ step 4: Pipeline   ------
    // --------------------------------
    auto createComputePipeline =
        [&]() -> std::tuple<VkPipeline, VkPipelineLayout> {
      // Shader code
      auto readBinaryFile =
          [](const std::string &path) -> std::vector<uint8_t> {
        std::fstream file(path, std::ios::in | std::ios::binary);
        if (!file.is_open())
        {
          throw std::runtime_error("failed to open file!");
        }

        file.seekg(0, std::ios_base::end);
        size_t fileSize = (size_t)file.tellg();
        std::vector<uint8_t> fileBuffer(fileSize);

        file.seekg(0, std::ios_base::beg);
        file.read(reinterpret_cast<char *>(fileBuffer.data()), fileSize);

        file.close();

        return fileBuffer;
      };
      std::vector<uint8_t> compShaderCode =
          readBinaryFile("./shaders/comp_1.spv");

      // Shader module
      auto createShaderModule =
          [&](const std::vector<uint8_t> &code) -> VkShaderModule {
        VkShaderModuleCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) !=
            VK_SUCCESS)
        {
          throw std::runtime_error("failed to create shader module!");
        }
        return shaderModule;
      };
      VkShaderModule compShaderModule = createShaderModule(compShaderCode);

      // shader stages
      VkPipelineShaderStageCreateInfo compShaderStageInfo = {};
      compShaderStageInfo.sType =
          VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
      compShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
      compShaderStageInfo.module = compShaderModule;
      compShaderStageInfo.pName = "main";

      // Pipeline layout
      VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
      pipelineLayoutCreateInfo.sType =
          VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
      pipelineLayoutCreateInfo.setLayoutCount = 1;
      pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;

      VkPipelineLayout pipelineLayout;
      if (vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, NULL,
                                 &pipelineLayout) != VK_SUCCESS)
      {
        throw std::runtime_error("failed to create compute pipeline!");
      }

      // pipeline
      VkComputePipelineCreateInfo pipelineCreateInfo = {};
      pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
      pipelineCreateInfo.stage = compShaderStageInfo;
      pipelineCreateInfo.layout = pipelineLayout;

      VkPipeline computePipeline;
      if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1,
                                   &pipelineCreateInfo, NULL,
                                   &computePipeline) != VK_SUCCESS)
      {
        throw std::runtime_error("failed to create compute pipeline!");
      }
      std::cout << "ComputePipeline creation OK" << std::endl;

      // cleanup
      vkDestroyShaderModule(device, compShaderModule, nullptr);

      return std::make_tuple(computePipeline, pipelineLayout);
    };
    VkPipeline computePipeline;
    VkPipelineLayout pipelineLayout;
    std::tie(computePipeline, pipelineLayout) = createComputePipeline();

    // --------------------------------
    // --- step 5: Command Pool     ---
    // --------------------------------
    auto createCommandPool = [&]() -> VkCommandPool {
      VkCommandPoolCreateInfo poolInfo = {};
      poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
      poolInfo.queueFamilyIndex = queueFamilyIndex;

      VkCommandPool commandPool;
      if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) !=
          VK_SUCCESS)
      {
        throw std::runtime_error("failed to create command pool!");
      }
      std::cout << "CommandPool creation OK" << std::endl;

      return commandPool;
    };
    VkCommandPool commandPool = createCommandPool();

    // --------------------------------
    // --- step 9: Command Buffers  ---
    // --------------------------------
    auto createCommandBuffer = [&]() -> VkCommandBuffer {
      VkCommandBufferAllocateInfo allocateInfo = {};
      allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
      allocateInfo.commandPool = commandPool;
      allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
      allocateInfo.commandBufferCount = 1;

      VkCommandBuffer commandBuffer;
      if (vkAllocateCommandBuffers(device, &allocateInfo, &commandBuffer) !=
          VK_SUCCESS)
      {
        throw std::runtime_error("failed to allocate command buffers!");
      }

      VkCommandBufferBeginInfo beginInfo = {};
      beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
      beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

      if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
      {
        throw std::runtime_error("failed to begin recording command buffer!");
      }

      vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                        computePipeline);
      vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                              pipelineLayout, 0, 1, &descriptorSet, 0, NULL);
      vkCmdDispatch(commandBuffer, 32, 1, 1);

      if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
      {
        throw std::runtime_error("failed to record command buffer!");
      }
      std::cout << "CommandBuffer creation OK" << std::endl;

      return commandBuffer;
    };
    VkCommandBuffer commandBuffer = createCommandBuffer();

    // --------------------------------
    // ---- step 10: execCompute  -----
    // --------------------------------
    auto execCompute = [&]() {
      // fence
      VkFence fence;
      VkFenceCreateInfo fenceCreateInfo = {};
      fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
      fenceCreateInfo.flags = 0;
      vkCreateFence(device, &fenceCreateInfo, NULL, &fence);

      // submit
      VkSubmitInfo submitInfo = {};
      submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
      submitInfo.commandBufferCount = 1;
      submitInfo.pCommandBuffers = &commandBuffer;
      vkQueueSubmit(graphicsQueue, 1, &submitInfo, nullptr);

      // wait finish
      vkWaitForFences(device, 1, &fence, VK_TRUE, 0xFFFFFFFF);

      vkDestroyFence(device, fence, NULL);
    };
    execCompute();

    // --------------------------------
    // ----- step 11: Get Result  -----
    // --------------------------------
    auto getResult = [&]() {
      void *data = nullptr;
      VkMemoryRequirements memoryRequirements = {};
      vkGetBufferMemoryRequirements(device, buffer, &memoryRequirements);
      //
      vkMapMemory(device, bufferMemory, 0, memoryRequirements.size, 0,
                  reinterpret_cast<void **>(&data));
      for (size_t i = 0; i < memoryRequirements.size / sizeof(uint32_t);
           i += 1)
      {
        std::cout << reinterpret_cast<uint32_t *>(data)[i] << " ";
      }
      std::cout << std::endl;
      //
      vkUnmapMemory(device, bufferMemory);
    };
    getResult();

    // --------------------------------
    // ------ step FF: Cleanup  -------
    // --------------------------------
    auto finalCleanup = [&]() {
      vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
      vkDestroyCommandPool(device, commandPool, NULL);
      //
      vkFreeMemory(device, bufferMemory, NULL);
      vkDestroyBuffer(device, buffer, NULL);
      //
      vkFreeDescriptorSets(device, descriptorPool, 1, &descriptorSet);
      vkDestroyDescriptorPool(device, descriptorPool, NULL);
      //
      vkDestroyPipelineLayout(device, pipelineLayout, NULL);
      vkDestroyPipeline(device, computePipeline, NULL);
      //
      vkDestroyDevice(device, NULL);
      vkDestroyInstance(instance, NULL);
    };
    finalCleanup();
  }
};

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

namespace std {
#if __cplusplus <= 201103L
  // note: this implementation does not disable this overload for array types
  template<typename T, typename... Args>
  std::unique_ptr<T> make_unique(Args&&... args)
  {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
  }
#endif
}

namespace vk {

class Buffer {
public:
  Buffer() = delete;
  Buffer(const VkPhysicalDevice &physicalDevice,
         const VkDevice &device,
         uint32_t size,
         VkBufferUsageFlags usage,
         VkMemoryPropertyFlags properties) : m_physicalDevice(physicalDevice), m_device(device) {
    // Buffer
    VkBufferCreateInfo bufferCreateInfo = {};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = size;
    bufferCreateInfo.usage = usage; // buffer is used as a storage buffer.
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // buffer is exclusive to a single queue family at a time.

    if (vkCreateBuffer(m_device, &bufferCreateInfo, VK_NULL_HANDLE, &m_buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create buffers!");
    }

    VkMemoryRequirements memoryRequirements;
    vkGetBufferMemoryRequirements(m_device, m_buffer, &memoryRequirements);

    // Memory
    VkMemoryAllocateInfo allocateInfo = {};
    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.allocationSize = memoryRequirements.size;
    allocateInfo.memoryTypeIndex = findMemoryType(memoryRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(m_device, &allocateInfo, VK_NULL_HANDLE, &m_bufferMemory) != VK_SUCCESS) {
      throw std::runtime_error("failed to allocate buffer memory!");
    }

    // Bind
    vkBindBufferMemory(m_device, m_buffer, m_bufferMemory, 0);
  }
  ~Buffer() {
    vkFreeMemory(m_device, m_bufferMemory, VK_NULL_HANDLE);
    vkDestroyBuffer(m_device, m_buffer, VK_NULL_HANDLE);
  }

public:
  const VkBuffer &buf() const {
    return m_buffer;
  }
  const VkDeviceMemory &mem() const {
    return m_bufferMemory;
  }

  void print() const {
    void *data;
    VkMemoryRequirements memoryRequirements = { };
    vkGetBufferMemoryRequirements(m_device, m_buffer, &memoryRequirements);
    //
    vkMapMemory(m_device, m_bufferMemory, 0, memoryRequirements.size, 0, reinterpret_cast< void **>(&data));
    for (size_t i = 0; i < memoryRequirements.size / sizeof(uint32_t); i += 1) {
      std::cout << reinterpret_cast<uint32_t *>(data)[i] << " ";
    }
    std::cout << std::endl;
    //
    vkUnmapMemory(m_device, m_bufferMemory);
  }

private:
  uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
  }

private:
  const VkPhysicalDevice &m_physicalDevice;
  const VkDevice &m_device;
  VkBuffer m_buffer;
  VkDeviceMemory m_bufferMemory;
};

class Shader {
public:
  Shader() = delete;
  Shader(const VkDevice &device,
         const std::string &shaderPath,
         VkShaderStageFlagBits shaderStage) : m_device(device), m_shaderStage(shaderStage) {
    // Shader code
    auto readBinaryFile = [](const std::string &filePath) -> std::vector<uint8_t> {
      std::fstream file(filePath, std::ios::in | std::ios::binary);
      if (!file.is_open()) {
          throw std::runtime_error("failed to open file!");
      }

      file.seekg(0, std::ios_base::end);
      size_t fileSize = (size_t) file.tellg();
      std::vector<uint8_t> fileBuffer(fileSize);

      file.seekg(0, std::ios_base::beg);
      file.read(reinterpret_cast<char *>(fileBuffer.data()), fileSize);

      file.close();

      return fileBuffer;
    };
    std::vector<uint8_t> compShaderCode = readBinaryFile(shaderPath);

    // Shader module
    auto createShaderModule = [&](const std::vector<uint8_t>& code) -> VkShaderModule {
      VkShaderModuleCreateInfo createInfo = {};
      createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
      createInfo.codeSize = code.size();
      createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

      VkShaderModule shaderModule;
      if (vkCreateShaderModule(m_device, &createInfo, VK_NULL_HANDLE, &shaderModule) != VK_SUCCESS) {
          throw std::runtime_error("failed to create shader module!");
      }
      return shaderModule;
    };
    m_compShaderModule = createShaderModule(compShaderCode);
  }
  ~Shader() {
    vkDestroyShaderModule(m_device, m_compShaderModule, VK_NULL_HANDLE);
  }

public:
  VkShaderStageFlagBits stage() const {
    return m_shaderStage;
  }

  const VkShaderModule &module() const {
    return m_compShaderModule;
  }

private:
  const VkDevice &m_device;
  VkShaderStageFlagBits m_shaderStage;
  VkShaderModule m_compShaderModule;
};

class Fence {
public:
  Fence() = delete;
  Fence(const VkDevice &device) : m_device(device) {
    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = 0;
    vkCreateFence(m_device, &fenceCreateInfo, VK_NULL_HANDLE, &m_fence);
  }
  ~Fence() {
    vkDestroyFence(m_device, m_fence, VK_NULL_HANDLE);
  }

public:
  const VkFence &get() const {
    return m_fence;
  }

  void wait() const {
    // wait finish
    vkWaitForFences(m_device, 1, &m_fence, VK_TRUE, 0xFFFFFFFF);
  }

private:
  const VkDevice &m_device;
  VkFence m_fence;
};

class Command {
public:
  Command() = delete;
  Command(const VkDevice &device,
                const VkQueue &graphicsQueue,
                const VkPipelineLayout &pipelineLayout,
                const VkPipeline &computePipeline,
                const std::vector<VkDescriptorSet> &descriptorSets,
                const VkCommandPool &commandPool)
  : m_device(device)
  , m_graphicsQueue(graphicsQueue)
  , m_commandPool(commandPool) {
    // Create
    VkCommandBufferAllocateInfo allocateInfo = {};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.commandPool = m_commandPool;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(device, &allocateInfo, &m_commandBuffer) != VK_SUCCESS) {
      throw std::runtime_error("failed to allocate command buffers!");
    }

    // Record
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

    if (vkBeginCommandBuffer(m_commandBuffer, &beginInfo) != VK_SUCCESS) {
      throw std::runtime_error("failed to begin recording command buffer!");
    }

    vkCmdBindPipeline(m_commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);
    vkCmdBindDescriptorSets(m_commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, descriptorSets.size(), descriptorSets.data(), 0, VK_NULL_HANDLE);
    vkCmdDispatch(m_commandBuffer, 256, 1, 1);

    if (vkEndCommandBuffer(m_commandBuffer) != VK_SUCCESS) {
      throw std::runtime_error("failed to record command buffer!");
    }
  }
  ~Command() {
    vkFreeCommandBuffers(m_device, m_commandPool, 1, &m_commandBuffer);
  }

public:
  std::unique_ptr<Fence> submit() {
    auto fence = std::make_unique<Fence>(m_device);
    
    // submit
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_commandBuffer;
    vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, fence->get());

    return fence;
  }

private:
  const VkDevice &m_device;
  const VkQueue &m_graphicsQueue;
  VkCommandBuffer m_commandBuffer;
  const VkCommandPool &m_commandPool;
};

class ComputePipeline {
public:
  ComputePipeline() = delete;
  ComputePipeline(const VkDevice &device,
                  uint32_t queueFamilyIndex,
                  const VkQueue &graphicsQueue,
                  const std::unique_ptr<Shader> &shader,
                  const std::vector<std::vector<std::tuple<uint32_t, VkDescriptorType>>> &setsBindings)
                  : m_device(device),
                    m_queueFamilyIndex(queueFamilyIndex),
                    m_graphicsQueue(graphicsQueue) {
    initDescriptor(setsBindings);
    initPipeline(shader);
    initCommandPool();
  }
  ~ComputePipeline() {
    vkDestroyCommandPool(m_device, m_commandPool, VK_NULL_HANDLE);
    //
    vkDestroyPipeline(m_device, m_computePipeline, VK_NULL_HANDLE);
    vkDestroyPipelineLayout(m_device, m_pipelineLayout, VK_NULL_HANDLE);
    //
    vkFreeDescriptorSets(m_device, m_descriptorPool, m_descriptorSets.size(), m_descriptorSets.data());
    for (auto &setLayout : m_descriptorSetLayouts) {
      vkDestroyDescriptorSetLayout(m_device, setLayout, VK_NULL_HANDLE);
    }
    vkDestroyDescriptorPool(m_device, m_descriptorPool, VK_NULL_HANDLE);
  }

public:
  void feedBuffer(uint32_t set,
                  uint32_t binding,
                  const std::unique_ptr<Buffer> &buffer,
                  uint32_t offset,
                  uint32_t range) {
    // Attach our buffer to this set
    VkDescriptorBufferInfo descriptorBufferInfo = {};
    descriptorBufferInfo.buffer = buffer->buf();
    descriptorBufferInfo.offset = offset;
    descriptorBufferInfo.range = range;

    VkWriteDescriptorSet writeDescriptorSet = {};
    writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet.dstSet = m_descriptorSets[set];
    writeDescriptorSet.dstBinding = binding;
    writeDescriptorSet.descriptorCount = 1;
    writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet.pBufferInfo = &descriptorBufferInfo;
    vkUpdateDescriptorSets(m_device, 1, &writeDescriptorSet, 0, VK_NULL_HANDLE);
  }

  std::unique_ptr<Command> createCommand() {
    return std::make_unique<Command>(m_device,
                                     m_graphicsQueue,
                                     m_pipelineLayout,
                                     m_computePipeline,
                                     m_descriptorSets,
                                     m_commandPool);
  }

private:
  void initDescriptor(const std::vector<std::vector<std::tuple<uint32_t, VkDescriptorType>>> &setsBindings) {
    // ----------
    // n_sets = 2
    // setsBindings = {[(0, SSBO)], [(1, UBO), (1, SSBO)]}
    // ----------

    // Check order, set

    // Pool
    VkDescriptorPoolSize descriptorPoolSize = {};
    descriptorPoolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorPoolSize.descriptorCount = 1;

    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
    descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolCreateInfo.maxSets = setsBindings.size();
    descriptorPoolCreateInfo.poolSizeCount = 1;
    descriptorPoolCreateInfo.pPoolSizes = &descriptorPoolSize;
    descriptorPoolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

    if (vkCreateDescriptorPool(m_device, &descriptorPoolCreateInfo, VK_NULL_HANDLE, &m_descriptorPool) != VK_SUCCESS) {
      throw std::runtime_error("failed to create descriptor pool!");
    }

    // Sets binding layout
    for (const auto &bindings : setsBindings) {
      std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings;
      
      for (auto const &bind : bindings) {
        VkDescriptorSetLayoutBinding setLayoutBinding = {};
        std::tie(setLayoutBinding.binding, setLayoutBinding.descriptorType) = bind;
        setLayoutBinding.descriptorCount = 1;
        setLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        setLayoutBindings.push_back(setLayoutBinding);
      }

      VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
      descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
      descriptorSetLayoutCreateInfo.bindingCount = setLayoutBindings.size();
      descriptorSetLayoutCreateInfo.pBindings = setLayoutBindings.data();

      VkDescriptorSetLayout descriptorSetLayout;
      if (vkCreateDescriptorSetLayout(m_device, &descriptorSetLayoutCreateInfo, VK_NULL_HANDLE, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor!");
      }
      m_descriptorSetLayouts.push_back(descriptorSetLayout);
    }

    // Descriptor
    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
    descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO; 
    descriptorSetAllocateInfo.descriptorPool = m_descriptorPool;
    descriptorSetAllocateInfo.descriptorSetCount = m_descriptorSetLayouts.size();
    descriptorSetAllocateInfo.pSetLayouts = m_descriptorSetLayouts.data();

    m_descriptorSets.resize(m_descriptorSetLayouts.size());
    if (vkAllocateDescriptorSets(m_device, &descriptorSetAllocateInfo, m_descriptorSets.data()) != VK_SUCCESS) {
      throw std::runtime_error("failed to create descriptor pool!");
    }
  }

  void initPipeline(const std::unique_ptr<Shader> &shader) {
    // Shader stages
    VkPipelineShaderStageCreateInfo compShaderStageInfo = {};
    compShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    compShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    compShaderStageInfo.module = shader->module();
    compShaderStageInfo.pName = "main";

    // Pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = m_descriptorSetLayouts.size();
    pipelineLayoutCreateInfo.pSetLayouts = m_descriptorSetLayouts.data();

    if (vkCreatePipelineLayout(m_device, &pipelineLayoutCreateInfo, VK_NULL_HANDLE, &m_pipelineLayout) != VK_SUCCESS) {
      throw std::runtime_error("failed to create pipeline layout!");
    }

    // pipeline
    VkComputePipelineCreateInfo pipelineCreateInfo = {};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stage = compShaderStageInfo;
    pipelineCreateInfo.layout = m_pipelineLayout;

    if (vkCreateComputePipelines(m_device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, VK_NULL_HANDLE, &m_computePipeline) != VK_SUCCESS) {
      throw std::runtime_error("failed to create compute pipeline!");
    }
  }

  void initCommandPool() {
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = m_queueFamilyIndex;

    if (vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS) {
      throw std::runtime_error("failed to create command pool!");
    }
  }

private:
  const VkDevice &m_device;
  uint32_t m_queueFamilyIndex;
  const VkQueue &m_graphicsQueue;
  //
  VkDescriptorPool m_descriptorPool;
  std::vector<VkDescriptorSetLayout> m_descriptorSetLayouts;
  std::vector<VkDescriptorSet> m_descriptorSets;
  //
  VkPipelineLayout m_pipelineLayout;
  VkPipeline m_computePipeline;
  //
  VkCommandPool m_commandPool;
};

class Device {
public:
  Device() = delete;
  Device(VkPhysicalDevice physicalDevice,
         uint32_t queueFamilyIndex)
         : m_physicalDevice(physicalDevice),
           m_queueFamilyIndex(queueFamilyIndex) {
    // Specifying the queues to be created
    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = m_queueFamilyIndex;
    float queuePriority = 1.0f;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    // Specifying used device features
    VkPhysicalDeviceFeatures deviceFeatures = {};

    // Infomation of layers and extensions
    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.queueCreateInfoCount = 1;
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = 0;
    createInfo.enabledLayerCount = 0;

    // create device
    if (vkCreateDevice(m_physicalDevice, &createInfo, VK_NULL_HANDLE, &m_device) != VK_SUCCESS) {
      throw std::runtime_error("failed to create logical device!");
    }

    // get graphic queue
    vkGetDeviceQueue(m_device, 0, 0, &m_graphicsQueue);
  }
  ~Device() {
    vkDestroyDevice(m_device, VK_NULL_HANDLE);
  }

public:
  std::unique_ptr<Buffer> createBuffer(uint32_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) const {
    return std::make_unique<Buffer>(m_physicalDevice, m_device, size, usage, properties);
  }

  std::unique_ptr<Shader> createShader(const std::string &shaderPath,
                                       VkShaderStageFlagBits shaderStage) const {
    return std::make_unique<Shader>(m_device, shaderPath, shaderStage);
  }

  std::unique_ptr<ComputePipeline> createComputePipeline(const std::unique_ptr<Shader> &shader,
      const std::vector<std::vector<std::tuple<uint32_t, VkDescriptorType>>> &setsBindings) const {
    return std::make_unique<ComputePipeline>(m_device, m_queueFamilyIndex, m_graphicsQueue, shader, setsBindings);
  }

private:
  VkPhysicalDevice m_physicalDevice;
  uint32_t m_queueFamilyIndex;
  VkDevice m_device;
  VkQueue m_graphicsQueue;
};

struct Config {
  const bool enableValidationLayers = true;

  /*
  const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
  };
  */

  std::vector<const char *> getRequiredExtensions() {
    std::vector<const char *> extensions;

    if (enableValidationLayers) {
      extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
  };

  std::vector<const char *> getValidationLayers() {
    std::vector<const char *> layers;

    if (enableValidationLayers) {
      layers.push_back("VK_LAYER_LUNARG_standard_validation");
    }

    return layers;
  };
} config;

class Instance {
public:
  Instance() = delete;
  Instance(const std::string &appName,
           uint32_t appVersion,
           const std::string &engineName,
           uint32_t engineVersion) {
    // Information about our application.
    // Optional, driver could use this to optimize for specific app.
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = appName.data();
    appInfo.applicationVersion = appVersion;
    appInfo.pEngineName = engineName.data();
    appInfo.engineVersion = engineVersion;
    appInfo.apiVersion = VK_API_VERSION_1_0;

    // Information of extensions
    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    // extensions
    auto extensions = config.getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    // layers
    auto layers = config.getValidationLayers();
    createInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
    createInfo.ppEnabledLayerNames = layers.data();

    if (vkCreateInstance(&createInfo, VK_NULL_HANDLE, &m_instance) != VK_SUCCESS) {
      throw std::runtime_error("failed to create instance!");
    }
  }
  ~Instance() {
    vkDestroyInstance(m_instance, VK_NULL_HANDLE);
  }

public:
  std::unique_ptr<Device> getDevice(VkQueueFlagBits queueFlag) const {
    // 
    uint32_t queueFamilyIndex;
    VkPhysicalDevice physicalDevice;
    std::tie(queueFamilyIndex, physicalDevice) = initPhyscalDevice(queueFlag);

    //
    return std::make_unique<Device>(physicalDevice, queueFamilyIndex);
  }

  std::unique_ptr<Device> getGraphicDevice() const {
    return getDevice(VK_QUEUE_GRAPHICS_BIT);
  }

  std::unique_ptr<Device> getComputeDevice() const {
    return getDevice(VK_QUEUE_COMPUTE_BIT);
  }

private:
  std::tuple<uint32_t, VkPhysicalDevice> initPhyscalDevice(const VkQueueFlagBits &queueFlag) const {
    // Count devices
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, VK_NULL_HANDLE);
    if (deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    // Enum all physical devices
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

    auto isDeviceSuitable = [&](const VkPhysicalDevice &device) -> std::tuple<bool, uint32_t> {
      uint32_t queueFamilyCount = 0;
      vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, VK_NULL_HANDLE);

      std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
      vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

      uint32_t index = 0;
      for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueCount > 0 && queueFamily.queueFlags & queueFlag) {
          break;
        }
        index += 1;
      }
      if (index == queueFamilies.size()) {
        throw std::runtime_error("failed to find a suitable device!");
      }

      return std::make_tuple(true, index);
    };

    // Pick one with graphics and compute pipeline support
    uint32_t queueFamilyIndex;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    for (const auto& device : devices) {
      bool foundDevice;
      std::tie(foundDevice, queueFamilyIndex) = isDeviceSuitable(device);
      if (foundDevice) {
        physicalDevice = device;
        break;
      }
    }
    if (physicalDevice == VK_NULL_HANDLE) {
      throw std::runtime_error("failed to find a suitable device!");
    }

    return std::make_tuple(queueFamilyIndex, physicalDevice);
  }

private:
  VkInstance m_instance;
};

std::unique_ptr<Instance> createInstance(const std::string &appName = "Demo",
           uint32_t appVersion = VK_MAKE_VERSION(1, 0, 0),
           const std::string &engineName = "No Engine",
           uint32_t engineVersion = VK_MAKE_VERSION(1, 0, 0)) {
  return std::make_unique<Instance>(appName, appVersion, engineName, engineVersion);
}

}

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
