#pragma once

#include "pch.h"

namespace slippcrunch {
	constexpr size_t DEFAULT_COMBO_INTRO_FRAMES = 60;
	constexpr size_t DEFAULT_COMBO_OUTRO_FRAMES = 60;
	struct Combo {
		// The C++ data that later gets converted to JSON
		// The strings in here do not correspond to the outputted JSON,
		// they are just copies of the parsed slippc replay data
		struct ReplayData {
			std::string absolute_replay_file_path;
			std::string timestamp;
			int32_t first_game_frame;
			int32_t last_game_frame;
		};

		// The actual core combo data
		std::vector<slip::Attack> attacks;
		slip::Punish punish;

		ReplayData replay_data;

		size_t intro_frames = DEFAULT_COMBO_INTRO_FRAMES;
		size_t outro_frames = DEFAULT_COMBO_OUTRO_FRAMES;
		
		bool DidKill() const;
		size_t TotalMoveCount() const;
		size_t UniqueMoveCount() const;
		uint16_t TotalDamage() const;
		uint16_t HighestSingleAttackDamage() const;
		float HighestSingleAttackDamageRatio() const;
		int Score() const;
		
		int32_t MovieStartFrame() const;
		int32_t MovieEndFrame() const;
		int32_t ClampToGameFrames(int32_t target_frame) const;
		std::string ToJson(size_t base_indentation_count, size_t indentation_size) const;

		static std::string FormatFilePath(const std::string& original_file_path);
		static std::string FormatTimestamp(const std::string& original_timestamp);
	};
}
