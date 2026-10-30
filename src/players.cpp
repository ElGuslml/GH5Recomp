// gh5 - the profile follows the controller that starts the game.
//
// On the console the controller that presses a button at the title screen plays with its own profile. Here there is
// one profile, the first controller's (the order Windows connected them in), and another controller that started the
// game got the game's "not signed in" message. While the title screen is shown (src/quit_prompt.cpp, each frame), the
// SDK's primary press window is open: the controller that presses first becomes user 0, the profile's, and the other
// controllers are its guests ("Name (1)", the SDK's signin_guests, set in src/startup.cpp), as when guests sign in on
// the console. GH Van Halen's ("PRESS ANY BUTTON TO ROCK"); GH5 starts in Party Play, not told apart yet, so the
// window stays closed for now (docs/fixes-from-gh-van-halen.md).

#include "players.h"

#include <rex/cvar.h>
#include <rex/input/input_system.h>
#include <rex/logging.h>

REXCVAR_DEFINE_BOOL(gh5_profile_follows_press, true, "GH5",
                    "The controller that presses a button at the title screen plays with the profile; off: the "
                    "first controller connected does, as before");

namespace gh5 {

void SetTitleScreen(bool shown) {
  static bool open = false;
  const bool want = shown && REXCVAR_GET(gh5_profile_follows_press);
  if (want == open) {
    return;
  }
  open = want;
  rex::input::SetPrimaryPressWindow(open);
  REXLOG_DEBUG("[players] the title screen's press window {}", open ? "open" : "closed");
}

}  // namespace gh5
