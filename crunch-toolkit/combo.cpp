
#include "pch.h"

#include "combo.h"

namespace slippcrunch {
	bool Combo::DidKill() const { 
		return Dir::NEUT < punish.kill_dir && punish.kill_dir < Dir::__LAST; 
	}

	size_t Combo::TotalMoveCount() const {
		return attacks.size();
	}

	size_t Combo::UniqueMoveCount() const {
		size_t unique_move_count = 0;
		std::vector<uint8_t> move_ids(attacks.size());
		for (const auto& attack : attacks) {
			if (std::find(move_ids.begin(), move_ids.end(), attack.move_id) == move_ids.end()) {
				move_ids.push_back(attack.move_id);
				unique_move_count++;
			}
		}
		return unique_move_count;
	}

	uint16_t Combo::HighestSingleAttackDamage() const {
		auto compare_func = [](const slip::Attack& a1, const slip::Attack& a2) { return a1.damage < a2.damage; };
		auto highest_attack = std::max_element(attacks.begin(), attacks.end(), compare_func);
		return highest_attack != attacks.end() ? highest_attack->damage : 0;
	}

	uint16_t Combo::TotalDamage() const {
		uint16_t total_damage = 0;
		for (const auto& attack : attacks) {
			total_damage += attack.damage;
		}
		return total_damage;
	}

	float Combo::HighestSingleAttackDamageRatio() const {
		auto highest_single_attack_damage = HighestSingleAttackDamage();
		auto total_damage = TotalDamage();
		auto ratio = static_cast<float>(highest_single_attack_damage) / static_cast<float>(total_damage);
		return ratio;
	}

	int Combo::Score() const {
		return 0;
	}

	int32_t Combo::ClampToGameFrames(int32_t target_frame) const {
		return std::clamp<int>(target_frame, replay_data.first_game_frame, replay_data.last_game_frame);
	}

	int32_t Combo::MovieStartFrame() const {
		int32_t target_frame = (punish.start_frame - LOAD_FRAME) - intro_frames;
		return ClampToGameFrames(target_frame);
	}

	int32_t Combo::MovieEndFrame() const {
		int32_t target_frame = (punish.end_frame - LOAD_FRAME) + outro_frames;
		return ClampToGameFrames(target_frame);
	}

	std::string Combo::ToJson(size_t base_indentation_count, size_t indentation_size) const {
		std::stringstream json_output;
		
		std::string single_indent(indentation_size, ' ');
		std::stringstream base_indent;
		for (size_t iBaseIndent = 0; iBaseIndent < base_indentation_count; ++iBaseIndent) {
			base_indent << single_indent;
		}

		json_output << base_indent.str() << "{" << std::endl;
		json_output << base_indent.str() << single_indent << "\"path\": \"" << FormatFilePath(replay_data.absolute_replay_file_path) << "\"," << std::endl;
		json_output << base_indent.str() << single_indent << "\"gameStartAt\": \"" << FormatTimestamp(replay_data.timestamp) << "\"," << std::endl;
		json_output << base_indent.str() << single_indent << "\"startFrame\": " << MovieStartFrame() << "," << std::endl;
		json_output << base_indent.str() << single_indent << "\"endFrame\": " << MovieEndFrame() << std::endl;
		json_output << base_indent.str() << "}";

		return json_output.str();
	}

	// Converts C:\Users\Xyz\... into C:\\Users\\Xyz\\...
	std::string Combo::FormatFilePath(const std::string& original_file_path) {
		std::string formatted_file_path = original_file_path;
		size_t start_pos = formatted_file_path.find("\\");
		while (start_pos != formatted_file_path.npos) {
			formatted_file_path.replace(start_pos, 1, "@");
			start_pos = formatted_file_path.find("\\");
		}
		start_pos = formatted_file_path.find("@");
		while (start_pos != formatted_file_path.npos) {
			formatted_file_path.replace(start_pos, 1, "\\\\");
			start_pos = formatted_file_path.find("@");
		}
		return formatted_file_path;
	}

	// Converts YYYY-MM-DDTHH:MM:SSZ to MM/DD/YY HH:MM (am/pm)
	std::string Combo::FormatTimestamp(const std::string& original_timestamp) {
		std::stringstream formatted_timestamp;

		// Converts YYYY-MM-DD to MM/DD/YY (+space)
		formatted_timestamp << original_timestamp.substr(5, 2) << "/"; // month
		formatted_timestamp << original_timestamp.substr(8, 2) << "/"; // day
		formatted_timestamp << original_timestamp.substr(2, 2) << " "; // year

		// Converts HH:MM:SS to HH:MM (am/pm)
		std::string minutes = original_timestamp.substr(14, 2);
		int hour = std::stoi(original_timestamp.substr(11, 2));
		bool is_pm = hour >= 12;
		if (is_pm && hour > 12) hour -= 12;
		if (hour == 0) hour = 12;
		formatted_timestamp << hour << ":" << minutes << " " << (is_pm ? "pm" : "am");

		return formatted_timestamp.str();
	}
}
