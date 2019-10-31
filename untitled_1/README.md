## TODO
1. [20%] vulkan graphic pipeline, windows surface
2. [90%] full android support
3. [50%] refine code, (maybe) make all reference to shared_ptr
4. [50%] full test
5. [0%] multi-pipeline support, for complex tasks

```CPP
void test_uniform() {
  auto instance = vk::createInstance();
  std::cout << "1. Instance ready" << std::endl;

  auto device = instance->getComputeDevice();
  std::cout << "2. Device ready" << std::endl;

  auto shader =
      device->createShader("./shaders/test_2.spv", VK_SHADER_STAGE_COMPUTE_BIT);
  std::cout << "3. Shader ready" << std::endl;

  auto pipeline = device->createComputePipeline(
      shader, {{std::make_tuple(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER),
                std::make_tuple(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)}});
  std::cout << "4. Pipeline ready" << std::endl;

  auto buffer = device->createBuffer(64 * sizeof(uint32_t), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
  pipeline->feedBuffer(0, 1, buffer, 0, 64 * sizeof(uint32_t));
  auto uniform = device->createBuffer(1 * sizeof(uint32_t), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
  pipeline->feedBuffer(0, 0, uniform, 0, 1 * sizeof(uint32_t));
  uint32_t scalar = 2;
  uniform->update(&scalar, sizeof(scalar));
  std::cout << "5. Buffer ready" << std::endl;

  auto command = pipeline->createCommand(64);
  auto fence = command->submit();
  std::cout << "6. Command ready" << std::endl;

  fence->wait();
  std::cout << "7. Fence ready" << std::endl;

  auto data = std::array<uint32_t, 64>();
  buffer->dump(data.data(), 64 * sizeof(uint32_t));
  for (size_t i = 0; i < data.size(); i += 1) {
    if (data[i] != scalar * i) {
      throw std::runtime_error("check error");
    }
  }
  std::cout << "8. Finish" << std::endl;
}
```
