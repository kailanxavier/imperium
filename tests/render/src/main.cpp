#include <app/application.h>
#include <sandbox/sandbox_app.h>
#include <gfx/image.h>

#include <core/log/log.h>
#include <core/platform/exe_path.h>

#include <cstring>
#include <string>

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"

using namespace imp;

namespace
{
    constexpr u32 kWidth = 3840;
    constexpr u32 kHeight = 2160;
    constexpr int kFramesToRender = 30;
    constexpr u8 kPerPixelThreshold = 2;
    constexpr double kMaxAllowedMeanError = 0.5;

    const std::string kReferencePath = "reference/basic_scene.png";
    const std::string kFailureOutputPath = "basic_scene_actual.png";
}

int main(int argc, char** argv)
{
    bool recordMode = false;
    for (int i = 1; i < argc; ++i)
    {
        if (std::strcmp(argv[i], "--record") == 0)
            recordMode = true;
    }

    log::Logger::get().initialise();
    LOG_INFO("RenderSmokeTest", "Starting ({})", recordMode ? "record" : "compare");

    app::ApplicationDesc desc{};
    desc.window.title = "render_smoke_test";
    desc.window.width = kWidth;
    desc.window.height = kHeight;
    desc.window.startVisible = false;
    desc.enableValidation = true;
    desc.vsync = false;

    const auto assetsPath = ( platform::executableDir() / "assets" ).string();
    desc.vfsMounts.push_back(app::VfsMountDesc{ "assets/", assetsPath, 0, true, true });

    app::Application application;
    if (!application.initialise(desc, std::make_unique<app::SandboxApp>()))
    {
        LOG_FATAL("RenderSmokeTest", "Application failed to initialise");
        log::Logger::get().shutdown();
        return 2;
    }

    auto* sandbox = dynamic_cast<app::SandboxApp*>( application.app().get() );
    if (!sandbox)
    {
        LOG_FATAL("RenderSmokeTest", "Expected a SandboxApp instance");
        application.shutdown();
        log::Logger::get().shutdown();
        return 2;
    }

    const gfx::TextureFormat backBufferFormat = application.device().backBuffer().format();
    gfx::TextureDesc readbackDesc{};
    readbackDesc.width = kWidth;
    readbackDesc.height = kHeight;
    readbackDesc.format = backBufferFormat;
    readbackDesc.usage = gfx::TextureUsage::RenderTarget | gfx::TextureUsage::TransferSrc;
    readbackDesc.debugName = "SmokeTestReadbackTarget";

    std::unique_ptr<gfx::IRenderTarget> readbackTarget = application.device().createRenderTarget(readbackDesc);
    if (!readbackTarget)
    {
        LOG_FATAL("RenderSmokeTest", "Failed to create readback target");
        application.shutdown();
        log::Logger::get().shutdown();
        return 2;
    }
    sandbox->setReadbackTarget(readbackTarget.get());

    app::AppContext ctx{
        application.window(), application.device(), application.window().input(),
        application.layers(), application.vfs(), application.world(),
        application.jobs(), application.gfxAllocator() };

    constexpr float kFixedDeltaSeconds = 1.f / 60.f;

    for (int frame = 0; frame < kFramesToRender; ++frame)
    {
        application.window().input().newFrame();
        application.window().pollEvents();

        ImGui_ImplGlfw_NewFrame();
        application.device().newImGuiFrame();
        ImGui::NewFrame();

        application.app()->onUpdate(ctx, kFixedDeltaSeconds);
        application.layers().updateAll(kFixedDeltaSeconds);

        gfx::ICommandList* cmd = application.device().beginFrame();
        if (cmd)
            application.app()->onRender(ctx, *cmd);

        ImGui::Render();

        if (cmd)
        {
            application.device().renderImGui(*cmd);
            application.device().endFrame();
        }
    }

    application.device().waitIdle();

    gfx::ImageData actual;
    actual.width = kWidth;
    actual.height = kHeight;
    if (!application.device().readbackTexture(*readbackTarget, actual.pixels))
    {
        LOG_FATAL("RenderSmokeTest", "readbackTexture() failed");
        sandbox->setReadbackTarget(nullptr);
        application.shutdown();
        log::Logger::get().shutdown();
        return 2;
    }

    sandbox->setReadbackTarget(nullptr);

    if (backBufferFormat == gfx::TextureFormat::BGRA8Srgb)
    {
        for (size_t i = 0; i + 3 < actual.pixels.size(); i += 4)
            std::swap(actual.pixels[i], actual.pixels[i + 2]);
    }

    int exitCode = 0;

    if (recordMode)
    {
        if (!gfx::saveImageToFile(actual, kReferencePath))
        {
            LOG_FATAL("RenderSmokeTest", "Failed to write reference image to '{}'", kReferencePath.c_str());
            exitCode = 2;
        }
        else
        {
            LOG_INFO("RenderSmokeTest", "Wrote new reference image to '{}'", kReferencePath.c_str());
        }
    }
    else
    {
        const gfx::ImageData reference = gfx::loadImageFromFile(kReferencePath);
        if (!reference.isValid())
        {
            LOG_FATAL("RenderSmokeTest", "Failed to load reference image '{}'. Did you forget to --record first?", kReferencePath.c_str());
            exitCode = 2;
        }
        else
        {
            const gfx::ImageDiffResult diff = gfx::compareImages(reference, actual, kPerPixelThreshold);

            if (!diff.sameSize)
            {
                LOG_ERROR("RenderSmokeTest", "Size mismatch: reference is {}x{}, actual is {}x{}",
                    reference.width, reference.height, actual.width, actual.height);
                exitCode = 1;
            }
            else if (diff.meanAbsError > kMaxAllowedMeanError)
            {
                LOG_ERROR("RenderSmokeTest", "FAIL: meanAbsError={:.3f} (max {:.3f}), maxAbsError={}, diffPixels={}",
                    diff.meanAbsError, kMaxAllowedMeanError, diff.maxAbsError, diff.diffPixelCount);
                gfx::saveImageToFile(actual, kFailureOutputPath);
                LOG_ERROR("RenderSmokeTest", "Actual frame written to '{}'", kFailureOutputPath.c_str());
                exitCode = 1;
            }
            else
            {
                LOG_INFO("RenderSmokeTest", "PASS: meanAbsError={:.3f}, maxAbsError={}, diffPixels={}",
                    diff.meanAbsError, diff.maxAbsError, diff.diffPixelCount);
            }
        }
    }

    application.shutdown();
    log::Logger::get().shutdown();
    return exitCode;
}
