#include <cmath>
#include <tuple>
#include <vector>
#include <string>
#include <memory>
#include <jni.h>
#include <android/log.h>
#include <android/bitmap.h>
#include <naive_vulkan/vulkan.hpp>

#define LOG_TAG "native"
#define LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)


class Engine {
public:
    Engine() : m_firstRender(true) {
        wrapper_init();
        LOGI("0. Vulkan env ready");

        m_instance = vk::createInstance();
        LOGI("1. Instance ready");

        m_device = m_instance->getComputeDevice();
        LOGI("2. Device ready");
    }

    ~Engine() {
        LOGI("8. Finish");
    }

    void init(const uint8_t *shader, size_t size) {
        std::vector<uint8_t> code;
        for (size_t i = 0; i < size; i += 1) {
            code.push_back(shader[i]);
        }
        m_shader = m_device->createShader(code, VK_SHADER_STAGE_COMPUTE_BIT);
        LOGI("3. Shader ready");

        m_pipeline = m_device->createComputePipeline(m_shader, {{std::make_tuple(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)}});
        LOGI("4. Pipeline ready");

        m_buffer = m_device->createBuffer(1024 * 1024 * 4, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        m_pipeline->feedBuffer(0, 0, m_buffer, 0, 1024 * 1024 * 4);
        LOGI("5. Buffer ready");

        m_command = m_pipeline->createCommand(1024, 1024);
        LOGI("6. Command ready");
    }

    void render(void *out, size_t size) {
        if (m_firstRender) {
            m_fence = m_command->submit();
            m_firstRender = false;
            return;
        }
        m_fence->wait();
        LOGI("7. execute ready");

        m_buffer->dump(out, size);
        m_fence = m_command->submit();
    }

private:
    std::unique_ptr<vk::Instance> m_instance;
    std::unique_ptr<vk::Device> m_device;
    std::unique_ptr<vk::Shader> m_shader;
    std::unique_ptr<vk::ComputePipeline> m_pipeline;
    std::unique_ptr<vk::Buffer> m_buffer;
    std::unique_ptr<vk::Command> m_command;
    bool m_firstRender;
    std::unique_ptr<vk::Fence> m_fence;
};
std::unique_ptr<Engine> engine;


extern "C" JNIEXPORT void JNICALL
Java_com_example_jniview2_CustomImageView_initNative(JNIEnv *env, jobject obj, jbyteArray bytes) {
    assert(obj);

    jsize size = env->GetArrayLength(bytes);
    if (size < 0) {
        LOGE("env->GetArrayLength() failed!, size = %d", size);
        return;
    }

    engine = std::make_unique<Engine>();

    auto *shader = reinterpret_cast<uint8_t *>(env->GetByteArrayElements(bytes, nullptr));
    engine->init(shader, static_cast<size_t >(size));
    env->ReleaseByteArrayElements(bytes, reinterpret_cast<jbyte *>(shader), 0);
}


extern "C" JNIEXPORT void JNICALL
Java_com_example_jniview2_CustomImageView_renderNative(JNIEnv *env, jobject obj, jobject bitmap) {
    assert(obj);

    int ret;
    AndroidBitmapInfo info;

    if ((ret = AndroidBitmap_getInfo(env, bitmap, &info)) < 0) {
        LOGE("AndroidBitmap_getInfo() failed! error=%d", ret);
        return;
    }

    if (info.format != ANDROID_BITMAP_FORMAT_RGBA_8888) {
        LOGE("Bitmap format is not RGBA_8888!");
        return;
    }

    void *pixels;
    if ((ret = AndroidBitmap_lockPixels(env, bitmap, &pixels)) < 0) {
        LOGE("AndroidBitmap_lockPixels() failed! error=%d", ret);
        return;
    }

    engine->render(pixels, 4 * 1024 * 1024);

    if ((ret = AndroidBitmap_unlockPixels(env, bitmap)) < 0) {
        LOGE("AndroidBitmap_unlockPixels() failed! error=%d", ret);
        return;
    }
}
