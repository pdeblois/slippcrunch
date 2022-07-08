#pragma once

#include "pch.h"

namespace crunch {
	constexpr int COMBO_INTRO_FRAMES = 60;
	constexpr int COMBO_OUTRO_FRAMES = 60;
	struct Attack {
		int frame;
	};
	struct Kill {
		int frame;
		int direction;
		int mode; // Star KO, Screen Hit, Blast Zone
	};
	struct Punish {
		std::vector<Attack> attacks;
		std::optional<Kill> kill;
		int StartFrame() { return FirstAttackFrame() - COMBO_INTRO_FRAMES; }
		int FirstAttackFrame() { return attacks.front().frame; }
		int LastAttackFrame() { return attacks.back().frame; }
		int KillFrame() { return kill.has_value() ? kill.value().frame : LastAttackFrame(); };
		int EndFrame() { return KillFrame() + COMBO_OUTRO_FRAMES; };
		int GetScore();
	};
}
