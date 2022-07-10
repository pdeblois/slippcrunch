#pragma once

#include "pch.h"

namespace Crunch {
	constexpr uint8_t COMBO_INTRO_FRAMES = 60;
	constexpr uint8_t COMBO_OUTRO_FRAMES = 60;
	struct Combo {
		std::vector<slip::Attack> attacks;
		slip::Punish punish;

		bool DidKill() const;
		uint8_t TotalMoveCount() const;
		uint8_t UniqueMoveCount() const;
		uint16_t HighestSingleAttackDamage() const;
		uint16_t TotalDamage() const;
		bool IsBelowMaxSingleAttackDamageRatioThreshold(float damage_ratio_threshold) const;
		int MovieStartFrame() const;
		int MovieEndFrame() const;
		int Score() const;
	};
}
