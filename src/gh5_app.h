// gh5 - the app: the SDK's virtual hooks (rex::ReXApp).

#pragma once

#include <rex/rex_app.h>

#include <rex/cvar.h>
#include <rex/filesystem.h>
#include <rex/kernel/xam/module.h>
#include <rex/logging.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xthread.h>
#include <rex/ui/imgui_dialog.h>

#include <native_x_renderer/native_x_renderer.h>

#include "native_x_renderer_gh5.h"
#include "keyboard.h"
#include "options.h"
#include "project.h"
#include "startup.h"
#include "widescreen.h"

class Gh5App : public rex::ReXApp {
 public:
  using rex::ReXApp::ReXApp;

  static std::unique_ptr<rex::ui::WindowedApp> Create(
      rex::ui::WindowedAppContext& ctx) {
    return std::unique_ptr<Gh5App>(new Gh5App(ctx, "gh5",
        PPCImageConfig));
  }

  // Other hooks the SDK offers (rex/rex_app.h): OnPostInitLogging, OnLoadXexImage, OnPostLoadXexImage,
  // OnCreateDialogs, OnShutdown.
  // Use the "xenos" GPU emulation (rexgpu-xenos.dll) unless another one was given.
  void OnPreSetup(rex::RuntimeConfig& config) override {
    if (config.gpu_plugin.empty()) {
      config.gpu_plugin = "xenos";
    }
  }

  // Game files: without --game_data_root, assets/ next to GH5Recomp.exe or the repository's assets/. User data
  // (shader cache, downloadable content; the saves and achievements before the profiles): userdata/ next to the
  // executable; the saves, settings and achievements in the launchers' current profile (src/startup.cpp).
  void OnConfigurePaths(rex::PathConfig& paths) override {
    if (paths.game_data_root.empty()) {
      paths.game_data_root = gh5::DefaultGameRoot();
    }
    gh5::ConfigureUserData(paths.user_data_root, paths.cache_root);
    gh5::ConfigureProfile(paths.user_data_root);
  }

  // gh5.toml with the built-in defaults if it is missing, then (after the SDK loads it) the game files check
  // and GH5 Launcher's options (separate executable, launcher/): gh5_options.toml, before the window and the GPU
  // are created. The game shows no window of its own. No game files check when the launcher asks for the
  // achievements' list (--achievements_cache_only: no window, a question would wait unseen).
  bool SetupEnvironment() override {
    auto exe_dir = rex::filesystem::GetExecutableFolder();
    gh5::EnsureDefaultConfig(exe_dir / GAME_CONFIG);
    if (!rex::ReXApp::SetupEnvironment()) return false;
    REXLOG_INFO("[startup] User data: {}", user_data_root().string());
    REXLOG_INFO("[startup] {}", native_x_renderer::Describe());
    if (!rex::cvar::Query<bool>("achievements_cache_only") && !gh5::CheckGameFiles(game_data_root())) return false;
    gh5::LoadLauncherOptions(exe_dir / GAME_OPTIONS, gh5::GpuPluginPath());
    return true;
  }

  // Window title (the SDK sets "<app name> <SDK build stamp>"), then NativeXRenderer, before the guest starts: its
  // hooks (src/native_x_renderer_gh5.cpp) are pass-throughs until it is initialized; then ultrawide
  // (src/widescreen.cpp): the cameras' aspect ratio follows the window.
  void OnPostSetup() override {
    if (window()) {
      window()->SetTitle(GAME_TITLE);
    }
    native_x_renderer::Initialize(gh5::NativeXRendererProfile(),
                                  runtime() ? runtime()->graphics_system() : nullptr, window(),
                                  runtime() ? runtime()->cache_root() : std::filesystem::path());
    if (window()) {
      gh5::SetOutputSize(window()->GetActualPhysicalWidth(), window()->GetActualPhysicalHeight());
    }
    gh5::InitializeKeyboard();
  }
  void OnWindowPixelSizeChanged(uint32_t pixel_width, uint32_t pixel_height) override {
    gh5::SetOutputSize(pixel_width, pixel_height);
  }

  // Ultrawide's black bars outside a song (src/widescreen.cpp): a dialog of the SDK's ImGui layer, gone before it.
  void OnCreateDialogs(rex::ui::ImGuiDrawer* drawer) override { menu_bars_ = gh5::CreateMenuBars(drawer); }
  void OnShutdown() override { menu_bars_.reset(); }

  // On exit, log whether the game asked to launch another executable.
  void OnGuestThreadExit(rex::system::XThread* thread) override {
    (void)thread;
    auto* rt = runtime();
    if (!rt || !rt->kernel_state()) return;
    auto xam = rt->kernel_state()->GetKernelModule<rex::kernel::xam::XamModule>("xam.xex");
    if (!xam) return;
    auto& ld = xam->loader_data();
    REXLOG_INFO("[gh5] Title exited. launch_path='{}' flags=0x{:X} launch_data={} bytes",
                ld.launch_path, ld.launch_flags, ld.launch_data.size());
  }

 private:
  std::unique_ptr<rex::ui::ImGuiDialog> menu_bars_;
};
