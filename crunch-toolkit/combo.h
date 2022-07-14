#pragma once

#include "pch.h"

namespace slippcrunch {
	constexpr uint8_t COMBO_INTRO_FRAMES = 60;
	constexpr uint8_t COMBO_OUTRO_FRAMES = 60;
	struct Combo {
		std::vector<slip::Attack> attacks;
		slip::Punish punish;

		bool DidKill() const;
		size_t TotalMoveCount() const;
		size_t UniqueMoveCount() const;
		uint16_t HighestSingleAttackDamage() const;
		uint16_t TotalDamage() const;
		float HighestSingleAttackDamageRatio() const;
		bool ExceedsSingleAttackDamageRatioThreshold(float damage_ratio_threshold) const;
		int MovieStartFrame() const;
		int MovieEndFrame() const;
		int Score() const;
	};
}
