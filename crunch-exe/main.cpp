// crunch-exe.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>

#include "parser.h"
#include "analysis.h"

#include "crunch.h"
#include "combo.h"

bool is_combo_valid(const slippcrunch::Combo& combo) {
	return combo.DidKill() && combo.TotalMoveCount() >= 7 && combo.TotalDamage() >= 60 && combo.HighestSingleAttackDamageRatio() <= 0.25f;
}

std::vector<slippcrunch::Combo> find_combos_from_parser(std::unique_ptr<slip::Parser> parser) {
	std::unique_ptr<slip::Analysis> analysis_ptr(parser->analyze());
	const slip::Analysis& analysis = *analysis_ptr;
	
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
				slippcrunch::Combo::ReplayData replay_data;
				replay_data.absolute_replay_file_path = parser->replay()->original_file;
				replay_data.timestamp = parser->replay()->start_time;
				replay_data.first_game_frame = parser->replay()->first_frame;
				replay_data.last_game_frame = parser->replay()->last_frame;
				curr_combo.replay_data = replay_data;
				combos.push_back(std::move(curr_combo));
			}
			curr_combo = {};
		}

		curr_combo.attacks.push_back(curr_attack);
	}

	return combos;
}

std::vector<slippcrunch::Combo> find_combos_from_replay_filename(std::string replay_filename) {
	std::unique_ptr<slip::Parser> parser = std::make_unique<slip::Parser>(0);
	parser->load(replay_filename.c_str());
	return find_combos_from_parser(std::move(parser));
}

size_t do_nothing_with_parser(std::unique_ptr<slip::Parser> parser) {
	return 1;
}

void log_progress(size_t processed_file_count, size_t total_file_count) {
	if (total_file_count == 0) {
		return;
	}

	static const std::string loading_symbols[] = { "-", "\\", "|", "/" };
	constexpr uint8_t loading_symbols_length = 4;
	static uint8_t iLoadingSymbol = 0;

	float progress = static_cast<float>(processed_file_count) / static_cast<float>(total_file_count);
	
	using indicator_count_t = uint8_t;
	static const indicator_count_t total_indicator_count = 50;
	indicator_count_t filled_indicator_count = static_cast<indicator_count_t>(std::floor(progress * total_indicator_count));
	indicator_count_t hollow_indicator_count = total_indicator_count - filled_indicator_count;
	
	std::string filled_indicators(filled_indicator_count, '-');
	std::string arrow_indicator(hollow_indicator_count > 0 ? 1 : 0, '>');
	std::string hollow_indicators(hollow_indicator_count > 0 ? hollow_indicator_count - 1 : 0, ' ');
	
	std::stringstream progress_report;
	progress_report << loading_symbols[++iLoadingSymbol % loading_symbols_length] << " Crunching...";
	progress_report << " [" << filled_indicators << arrow_indicator << hollow_indicators << "] ";
	progress_report << std::floor(progress * 100) << "% (" << processed_file_count << "/" << total_file_count << " files)";
	progress_report << std::endl;
	std::cout << progress_report.str();
}

std::vector<slippcrunch::Combo> filter_combo_crunch_results(const std::vector<std::optional<std::vector<slippcrunch::Combo>>>& file_results) {
	std::vector<slippcrunch::Combo> filtered_combos;
	for (const auto& file_result : file_results) {
		if (file_result.has_value()) {
			filtered_combos.insert(filtered_combos.end(), file_result.value().begin(), file_result.value().end());
		}
	}
	return filtered_combos;
}

void output_json_combo_queue(const std::vector<slippcrunch::Combo>& combos, std::string json_filename) {
	constexpr size_t indentation_size = 4;
	static const std::string single_indent(indentation_size, ' ');

	std::stringstream json_output;
	json_output << "{" << std::endl;
	json_output << single_indent << "\"mode\": \"queue\"," << std::endl;
	json_output << single_indent << "\"replay\": \"\"," << std::endl;
	json_output << single_indent << "\"isRealTimeMode\": false," << std::endl;
	json_output << single_indent << "\"outputOverlayFiles\": true," << std::endl;
	json_output << single_indent << "\"queue\": [" << std::endl;

	for (size_t iCombo = 0; iCombo < combos.size(); ++iCombo) {
		json_output << combos[iCombo].ToJson(2, indentation_size);
		if (iCombo != combos.size() - 1) {
			json_output << ",";
		}
		json_output << std::endl;
	}

	json_output << single_indent << "]" << std::endl;
	json_output << "}" << std::endl;

	std::ofstream json_file(json_filename, std::ios_base::out);
	json_file << json_output.str();
	json_file.close();
}

int main() {
	try {
		std::string json_filename;
		std::cout << "Enter the combo queue filename : ";
		std::cin >> json_filename;
		
		slippcrunch::crunch_params<std::vector<slippcrunch::Combo>> crunch_args;
		crunch_args.crunch_func = find_combos_from_parser;
		crunch_args.progress_report_func = log_progress;
		
		std::chrono::steady_clock::time_point crunch_start_time = std::chrono::steady_clock::now();
		std::vector<std::optional<std::vector<slippcrunch::Combo>>> crunch_results = slippcrunch::crunch<std::vector<slippcrunch::Combo>>::crunch_directory(crunch_args, std::filesystem::current_path(), true);
		std::chrono::steady_clock::time_point crunch_end_time = std::chrono::steady_clock::now();
		
		auto crunch_duration = std::chrono::duration_cast<std::chrono::seconds>(crunch_end_time - crunch_start_time);
		std::cout << "Crunched " << crunch_results.size() << " files in " << crunch_duration.count() << " seconds" << std::endl;
		
		size_t combo_count = 0;
		for (const auto& crunch_result : crunch_results) {
			combo_count += crunch_result.has_value() ? crunch_result.value().size() : 0;
		}
		std::cout << "Found " << combo_count << " combos" << std::endl;
		output_json_combo_queue(filter_combo_crunch_results(crunch_results), json_filename);
	}
	catch (const std::exception& error) {
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
