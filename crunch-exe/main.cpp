// crunch-exe.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>

#include "parser.h"
#include "analysis.h"

#include "crunch.h"
#include "combo.h"

bool is_combo_valid(const slippcrunch::Combo& combo) {
	return combo.DidKill() && combo.TotalMoveCount() >= 7 && combo.TotalDamage() >= 60 && !combo.ExceedsSingleAttackDamageRatioThreshold(0.25f);
}

std::vector<slippcrunch::Combo> find_combos_from_analysis(const slip::Analysis& analysis) {
	std::vector<slippcrunch::Combo> combos;

	int port_to_use = analysis.ap[0].tag_code == "YOYO#278" ? 0 : 1;
	const slip::AnalysisPlayer& player_analysis = analysis.ap[port_to_use];

	slippcrunch::Combo curr_combo;
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

std::vector<slippcrunch::Combo> find_combos_from_parser(std::unique_ptr<slip::Parser> parser) {
	std::unique_ptr<slip::Analysis> analysis(parser->analyze());
	return find_combos_from_analysis(*analysis);
}

std::vector<slippcrunch::Combo> find_combos_from_replay_filename(std::string replay_filename) {
	std::unique_ptr<slip::Parser> parser = std::make_unique<slip::Parser>(0);
	parser->load(replay_filename.c_str());
	return find_combos_from_parser(std::move(parser));
}

int main() {
	try {
		std::cout << "Press enter to start the crunch : ";
		std::cin.get();
		std::vector<std::vector<slippcrunch::Combo>> crunch_results = slippcrunch::crunch<std::vector<slippcrunch::Combo>>::execute(find_combos_from_parser, std::filesystem::current_path());
		size_t combo_count = 0;
		for (const auto& crunch_result : crunch_results) {
			combo_count += crunch_result.size();
		}
		std::cout << "Found " << combo_count << " combos" << std::endl;
	}
	catch (std::exception& error) {
		std::cout << error.what() << std::endl;
	}
	std::cin.get();
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
