
#include "pch.h"

#include "cruncher.h"
#include "combo.h"

bool is_combo_valid(const Crunch::Combo& combo) {
	return combo.DidKill() && combo.TotalMoveCount() >= 7 && combo.TotalDamage() >= 60 && !combo.ExceedsSingleAttackDamageRatioThreshold(0.25f);
}

std::vector<Crunch::Combo> find_combos_from_analysis(const slip::Analysis& analysis) {
	std::vector<Crunch::Combo> combos;

	int port_to_use = analysis.ap[0].tag_code == "YOYO#278" ? 0 : 1;
	const slip::AnalysisPlayer& player_analysis = analysis.ap[port_to_use];

	Crunch::Combo curr_combo;
	for (size_t iAttack = 0; player_analysis.attacks[iAttack].frame > 0; ++iAttack) {
		const slip::Attack& curr_attack = player_analysis.attacks[iAttack];
		const slip::Punish& curr_punish = player_analysis.punishes[curr_attack.punish_id];

		if (!curr_combo.attacks.empty() && curr_attack.punish_id != curr_combo.attacks.back().punish_id) {
			curr_combo.punish = player_analysis.punishes[curr_combo.attacks.back().punish_id];
			if (is_combo_valid(curr_combo)) {
				combos.push_back(curr_combo);
			}
			curr_combo = {};
		}

		curr_combo.attacks.push_back(curr_attack);
	}

	return combos;
}

std::vector<Crunch::Combo> find_combos_from_parser(std::unique_ptr<slip::Parser> parser) {
	std::unique_ptr<slip::Analysis> analysis(parser->analyze());
	return find_combos_from_analysis(*analysis);
}

std::vector<Crunch::Combo> find_combos_from_replay_filename(std::string replay_filename) {
	std::unique_ptr<slip::Parser> parser = std::make_unique<slip::Parser>(0);
	parser->load(replay_filename.c_str());
	return find_combos_from_parser(std::move(parser));
}

int main() {
	Crunch::CruncherDesc<std::vector<Crunch::Combo>> a;
	a.crunch_func = find_combos_from_parser;
	Crunch::Cruncher<std::vector<Crunch::Combo>> b(a);
	std::vector<std::vector<Crunch::Combo>> c = b.Crunch();
}
