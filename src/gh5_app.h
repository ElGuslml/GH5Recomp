// gh5 - the app: the SDK's virtual hooks (rex::ReXApp).

#pragma once

#include <rex/rex_app.h>

#include <rex/cvar.h>
#include <rex/filesystem.h>
#include <rex/kernel/xam/module.h>
#include <rex/logging.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xthread.h>

#include "options.h"
#include "project.h"
#include "startup.h"

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
  // (saves, profile, shader cache): userdata/ next to the executable (src/startup.cpp).
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
    if (!rex::cvar::Query<bool>("achievements_cache_only") && !gh5::CheckGameFiles(game_data_root())) return false;
    gh5::LoadLauncherOptions(exe_dir / GAME_OPTIONS, gh5::GpuPluginPath());
    return true;
  }

  // Window title (the SDK sets "<app name> <SDK build stamp>"). NativeXRenderer is initialized here too, before
  // the guest starts, once the game has its side (native_x_renderer/docs/integrating-a-game.md, step 2):
  //   native_x_renderer::Initialize(gh5::NativeXRendererProfile(),
  //                                 runtime() ? runtime()->graphics_system() : nullptr, window(),
  //                                 runtime() ? runtime()->cache_root() : std::filesystem::path());
  void OnPostSetup() override {
    if (window()) {
      window()->SetTitle(GAME_TITLE);
    }
  }

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
};
