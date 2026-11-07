// gh5 - NativeXRenderer for Guitar Hero 5: the game's D3D entry points and its D3D device and
// object layouts.
//
// The XDK Direct3D library is linked into default.xex (XDK 2.0.8276) at 0x8230A8D0-0x8232A5AC, the draws included
// (0x8230BEC8-0x8230D198), with XGraphics-style helpers inside it (0x823123C8-0x82313EE8). It is GH Van Halen's
// (XDK 2.0.7978) code: 561 of its 575 functions the same instruction for instruction, Present among the 14 others
// (recompiled, the same logic and offsets), so GH Van Halen's map and layouts are this game's with the addresses
// translated. The device layout is Dante's Inferno's up to the register shadow (POTCAWE's + 128), the current objects
// POTCAWE's + 144; the resource objects are POTCAWE's. Every address and offset here, with its evidence, is in
// docs/native-x-renderer-gh5.md. The hooks only run the original function unless a NativeXRenderer feature
// is on (--native_x_renderer_trace, --native_x_renderer_verify, --native_x_renderer).

#include "native_x_renderer_gh5.h"

#include <native_x_renderer/hooks.h>
#include <rex/cvar.h>
#include <rex/ppc/context.h>

// Draws. DrawVerticesUP and DrawIndexedVerticesUP (sub_8230C8C0, 4 engine sites, no call of its own) commit their
// packets with an inlined EndVertices ([dev+48] = [dev+13444]); the engine's three BeginVertices / EndVertices pairs
// (sub_822FE350, sub_822FFA78, sub_82303C00) call EndVertices (sub_8230BEB8), which is that same store. EndVertices is
// not hooked: inline_draw_end commits all of them.
NATIVE_X_RENDERER_HOOK(8230CD50, DrawIndexedVertices)
NATIVE_X_RENDERER_HOOK(8230C960, DrawVertices)
NATIVE_X_RENDERER_HOOK(8230C360, DrawVerticesUP)  // calls BeginVertices, inlines the end
NATIVE_X_RENDERER_HOOK(8230BEC8, BeginVertices)
NATIVE_X_RENDERER_HOOK(8230C3A8, BeginIndexedVertices)  // only from DrawIndexedVerticesUP

// Render targets, clears, resolves, tiling. The engine sets targets through sub_82310850, an alias that branches to
// SetRenderTarget. EndTiling resolves the tiles itself (the engine passes flags 0x300 and a destination).
NATIVE_X_RENDERER_HOOK(82310228, SetRenderTarget)
NATIVE_X_RENDERER_HOOK(82310578, SetDepthStencilSurface)
NATIVE_X_RENDERER_HOOK(82315950, Clear)  // calls ClearF
NATIVE_X_RENDERER_HOOK(82315828, ClearF)
NATIVE_X_RENDERER_HOOK(8231EEF8, Resolve)
NATIVE_X_RENDERER_HOOK(82320ED8, BeginTiling)
NATIVE_X_RENDERER_HOOK(82321248, EndTiling)
NATIVE_X_RENDERER_HOOK(82320D68, SetPredication)
NATIVE_X_RENDERER_HOOK(823100F8, SetViewport)  // D3DVIEWPORT9 (integers), stored as floats at +12656
NATIVE_X_RENDERER_HOOK(8230F758, SetScissorRect)

// Shaders, vertex input, textures, constants. The engine writes constants straight into the device too (in GH Van
// Halen VS in 18 functions, PS in 33; not counted here) and patches sampler states in the fetch constants, without a
// call. SetTexture, SetStreamSource
// and the constant setters take the dirty bit or mask from their caller.
NATIVE_X_RENDERER_HOOK(8230B5A8, SetVertexShader)
NATIVE_X_RENDERER_HOOK(8230B3E8, SetPixelShader)
NATIVE_X_RENDERER_HOOK(8230B778, SetVertexDeclaration)
NATIVE_X_RENDERER_HOOK(8230F8C8, SetStreamSource)
NATIVE_X_RENDERER_HOOK(8230FA70, SetIndices)
NATIVE_X_RENDERER_HOOK(823144F8, SetTexture)
NATIVE_X_RENDERER_HOOK(8230B160, SetVertexShaderConstantF)
NATIVE_X_RENDERER_HOOK(8230B238, SetPixelShaderConstantF)

// Resources: typed thunks over the common Lock (sub_82311E68) and Unlock (sub_82311098), and the cache flush over a
// range. The locks return the pointer (no data argument) and ignore the offset and size.
NATIVE_X_RENDERER_HOOK(82312230, VertexBufferLock)
NATIVE_X_RENDERER_HOOK(823113E0, VertexBufferUnlock)
NATIVE_X_RENDERER_HOOK(82312280, IndexBufferLock)
NATIVE_X_RENDERER_HOOK(823113F0, IndexBufferUnlock)
NATIVE_X_RENDERER_HOOK(82312500, TextureUnlock)
NATIVE_X_RENDERER_HOOK(8231E2A8, FlushCpuRange)

// Command buffer, queries, frame. The kick submits the segment and continues in the same memory; NewSegment moves the
// write pointer to a newly allocated segment (from the kick, the reservation sub_82317238 and Present). The engine
// calls Synchronize (sub_82320610, Swap with no front buffer), then Present, which calls VdSwap with the front buffer
// in r4; the library's worker thread sub_82322BF8 can present again.
NATIVE_X_RENDERER_HOOK(82317000, KickSegment)
NATIVE_X_RENDERER_HOOK(82316538, NewSegment)
NATIVE_X_RENDERER_HOOK(8230B900, SetShaderGPRAllocation)
NATIVE_X_RENDERER_HOOK(8231AF80, QueryIssue)
NATIVE_X_RENDERER_HOOK(8231B208, QueryGetData)
NATIVE_X_RENDERER_HOOK(823201E8, Swap)
NATIVE_X_RENDERER_HOOK(82320618, Present)

// The shadows' filter follows the map (a shadow quality above Original, D92). The characters' shadows are read by a
// pixel shader in nine samples, each compared with the pixel's depth (sub_825CCE98 writes their offsets to c10-c13, one
// texel of the 400x400 map apart: 1/400, from f0 and f12). A map drawn four times as large kept those offsets: its samples
// fell four texels apart, three stepped copies of each edge instead of a soft one (Gustavo: better, still pixelated).
// The offsets are divided by the shadow scale, so the nine samples are neighbours on the larger map again.
REXCVAR_DEFINE_BOOL(gh5_shadow_filter_fix, true, "GH5",
                    "With a shadow quality above Original, the filter of the shadows follows the map's size: sharper "
                    "edges (false = its width as on the console)");

// [[midasm_hook]] 0x825CD048 (sub_825CCE98): after "lfs f12,172(r1)", f0 and f12 = the shadow map's texel size.
void GH5ShadowFilterHook(PPCRegister& f0, PPCRegister& f12) {
  static const double scale =
      rex::cvar::Query<bool>("native_x_renderer") ? rex::cvar::Query<double>("native_x_renderer_shadow_scale") : 0.0;
  if (scale > 1.0 && REXCVAR_GET(gh5_shadow_filter_fix)) {
    f0.f64 = double(float(f0.f64 / scale));
    f12.f64 = double(float(f12.f64 / scale));
  }
}

namespace gh5 {

const native_x_renderer::Profile& NativeXRendererProfile() {
  static const native_x_renderer::Profile profile = [] {
    native_x_renderer::Profile p;
    p.game = "Guitar Hero 5 (41560840)";
    p.xdk_version = "2.0.8276";
    p.output_width = 1280;  // the presentation parameters come from a video-mode table at run time
    p.output_height = 720;
    // The whole scene's copy, before the post-processing, the HUD and the note highway. GH5 resolves it through the
    // render targets' helpers sub_82571A30 -> sub_82571288, so the outer caller names it too; the same pair also
    // copies the 320x180 passes (the size rule leaves them out) and the composite's first pass into the same texture
    // (the second 1280x720 copy of the frame: scene_resolve_once). Not a later copy: 0x8256A294 < 0x82570AE4, the
    // finished image, is read only by the next frame, so it would antialias every other frame. The frames that draw
    // only the stage and the crowd copy theirs from 0x82571B90: not taken, the band's frame restores them and copies
    // the whole scene here.
    p.scene_resolve_caller = 0x82571324;
    p.scene_resolve_outer_caller = 0x82571A64;
    p.scene_resolve_once = true;
    // The ultrawide hooks (src/widescreen.cpp) widen the 16:9 cameras by AspectFactor() and keep the 2D layer at
    // 16:9 in the center: NativeXRenderer may fill a wider output (native_x_renderer_fill_screen).
    p.widens_cameras = true;
    native_x_renderer::DeviceLayout& d = p.device;
    // Six dirty masks from +0: VS constants, PS constants, register groups and the program, fetch slots (low half,
    // slot 0 in bit 31) and the 0x2280 group, the 0x2300/0x2380 groups with the clip planes and bool/loop constants,
    // the tile-dependent state.
    d.vs_constants_dirty = 0;
    d.ps_constants_dirty = 8;
    d.fetch_constants_dirty = 24;
    d.fetch_dirty_slot0_bit = 31;
    d.command_buffer_write = 48;
    d.fetch_constants = 1152;
    d.vs_float_constants = 1920;
    d.ps_float_constants = 6016;
    d.bool_loop_constants = 10112;
    d.register_groups = {{
        {10368, 0x2000, 16},
        {10444, 0x2100, 21},
        {10528, 0x2180, 5},
        {10548, 0x2200, 12},
        {10596, 0x2280, 21},
        {10680, 0x2300, 38},
        {10832, 0x2380, 8},
    }};
    d.fence_counter = 10908;
    d.write_back_block = 10896;
    d.flags = 10940;
    d.vertex_declaration = 11824;
    d.index_buffer = 12444;
    d.render_targets = 12448;
    d.depth_stencil = 12464;
    d.vertex_buffers = 12468;  // one pointer per stream
    d.textures = 12552;
    d.viewport = 12656;  // x, y, width, height, min z, max z (floats)
    d.pixel_shader = 12700;
    d.vertex_shader = 12704;
    d.predication_mask = 12716;
    d.tile_count = 12756;
    d.tile_rects = 12760;
    // DrawVerticesUP, DrawIndexedVerticesUP and EndVertices commit BeginVertices' / BeginIndexedVertices' packets
    // with [dev+48] = [dev+13444].
    d.inline_draw_end = 13444;
    native_x_renderer::ObjectLayout& o = p.objects;
    o.texture_fetch = 28;
    o.surface_info = 24;
    o.surface_color_depth_info = 28;
    o.surface_size = 36;
    o.surface_width_minus_one = {18, 14};
    o.surface_height_minus_one = {3, 15};
    o.buffer_address = 24;
    o.buffer_size = 28;
    return p;
  }();
  return profile;
}

}  // namespace gh5
