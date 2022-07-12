
#include "pch.h"

#include "combo.h"

namespace Crunch {
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

	bool Combo::ExceedsSingleAttackDamageRatioThreshold(float damage_ratio_threshold) const {
		return HighestSingleAttackDamageRatio() > damage_ratio_threshold;
	}

	int Combo::MovieStartFrame() const { 
		return punish.start_frame - COMBO_INTRO_FRAMES; 
	}

	int Combo::MovieEndFrame() const {
		return punish.end_frame + COMBO_OUTRO_FRAMES; 
	}

	int Combo::Score() const {
		return 0;
	}
}
