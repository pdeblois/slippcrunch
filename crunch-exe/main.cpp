// crunch-exe.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>

#include "parser.h"
#include "analysis.h"

#include "cruncher.h"
#include "combo.h"

bool is_combo_valid(const Crunch::Combo& combo) {
	return combo.DidKill() && combo.TotalMoveCount() >= 7 && combo.TotalDamage() >= 60 && combo.IsBelowMaxSingleAttackDamageRatioThreshold(0.25f);
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
	try {
		Crunch::CruncherDesc<std::vector<Crunch::Combo>> a;
		a.crunch_func = find_combos_from_parser;
		std::cout << "Enter path : ";
		std::cin >> a.path;
		a.path = std::filesystem::current_path();
		std::cout << std::filesystem::is_directory(a.path) << std::endl;
		Crunch::Cruncher<std::vector<Crunch::Combo>> b(a);
		std::vector<std::vector<Crunch::Combo>> c = b.Crunch();
		std::cout << "Found " << c.size() << " combos" << std::endl;
	}
	catch (std::filesystem::filesystem_error& error) {
		std::cout << error.what() << std::endl;
	}
	int i;
	std::cin >> i;
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
