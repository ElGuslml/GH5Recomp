// gh5 - NativeXRenderer for Guitar Hero 5 (see native_x_renderer_gh5.cpp).

#pragma once

#include <native_x_renderer/native_x_renderer.h>

namespace gh5 {

// Guitar Hero 5's D3D library: the XDK 2.0.7978 device and object layouts.
const native_x_renderer::Profile& NativeXRendererProfile();

}  // namespace gh5
