#pragma once

#include "pch.h"

namespace slippcrunch {
	constexpr uint8_t COMBO_INTRO_FRAMES = 60;
	constexpr uint8_t COMBO_OUTRO_FRAMES = 60;
	struct Combo {
	public:
		// The actual core combo data
		std::vector<slip::Attack> attacks;
		slip::Punish punish;

		// Other replay-related info (useful for JSON output)
		std::string absolute_replay_file_path;
		std::string timestamp;
		int32_t first_game_frame;
		int32_t last_game_frame;

		bool DidKill() const;
		size_t TotalMoveCount() const;
		size_t UniqueMoveCount() const;
		uint16_t HighestSingleAttackDamage() const;
		uint16_t TotalDamage() const;
		float HighestSingleAttackDamageRatio() const;
		int Score() const;
		
		int32_t MovieStartFrame() const;
		int32_t MovieEndFrame() const;
		std::string ToJson(size_t base_indentation_count, size_t indentation_size) const;

	private:
		int32_t ClampToGameFrames(int32_t target_frame) const;
		static std::string format_file_path(const std::string& original_file_path);
		static std::string format_timestamp(const std::string& original_timestamp);
	};
}
