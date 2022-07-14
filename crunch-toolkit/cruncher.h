#pragma once

#include "pch.h"

namespace slippcrunch {

	template<typename R>
	std::vector<R> crunch_files
	(
		R(*crunch_func)(std::unique_ptr<slip::Parser>),
		std::queue<std::filesystem::directory_entry> file_entry_queue,
		std::atomic_size_t* processed_file_count
	)
	{
		std::vector<R> results;
		while (!file_entry_queue.empty()) {
			std::unique_ptr<slip::Parser> parser = std::make_unique<slip::Parser>(0);
			bool did_parse = parser->load(file_entry_queue.front().path().string().c_str());
			if (did_parse) {
				R crunch_func_result = crunch_func(std::move(parser));
				results.push_back(crunch_func_result);
				processed_file_count->store(processed_file_count->load() + 1);
				file_entry_queue.pop();
			}
		}
		return results;
	}

	template<typename F>
	bool are_futures_ready(const std::vector<std::future<F>>& futures) {
		for (const auto& future : futures) {
			auto future_status = future.wait_for(std::chrono::milliseconds::zero());
			if (future_status != std::future_status::ready) {
				return false;
			}
		}
		return true;
	}

	template<typename R>
	std::vector<R> crunch
	(
		R(*crunch_func)(std::unique_ptr<slip::Parser>),
		std::filesystem::path path,
		bool is_recursive = false,
		size_t max_worker_count = std::thread::hardware_concurrency() - 1
	)
	{
		const auto worker_count = std::clamp<size_t>(std::thread::hardware_concurrency() - 1, 1, max_worker_count); // leave 1 processor free for main thread if possible

		// Setup the file queues for each thread
		std::vector<std::queue<std::filesystem::directory_entry>> file_entry_queues(worker_count);
		std::vector<std::atomic_size_t> processed_file_counts(worker_count);
		size_t total_file_count = 0;
		std::chrono::steady_clock::time_point file_traversal_begin_time = std::chrono::steady_clock::now();
		for (const auto& file_entry : std::filesystem::recursive_directory_iterator(path)) {
			bool is_file = !file_entry.is_directory() && (file_entry.is_regular_file() || file_entry.is_symlink());
			bool is_slp_file = is_file && file_entry.path().has_extension() && file_entry.path().extension() == ".slp";
			if (is_slp_file) {
				file_entry_queues[total_file_count % worker_count].push(file_entry);
				total_file_count++;
			}
		}
		std::chrono::steady_clock::time_point file_traversal_end_time = std::chrono::steady_clock::now();
		std::cout << "Found " << total_file_count << " files in " << std::chrono::duration_cast<std::chrono::seconds>(file_traversal_end_time - file_traversal_begin_time).count() << " seconds" << std::endl;
		std::cin.get();

		// Start the tasks
		std::cout << "Starting " << worker_count << " workers to parse " << total_file_count << " files" << std::endl;
		std::vector<std::future<std::vector<R>>> futures;
		std::chrono::steady_clock::time_point crunch_begin_time = std::chrono::steady_clock::now();
		for (size_t iWorker = 0; iWorker < worker_count; ++iWorker) {
			futures.push_back(std::async(
				std::launch::async,
				crunch_files<R>,
				crunch_func,
				file_entry_queues[iWorker],
				&(processed_file_counts[iWorker])
			));
		}

		// Log the tasks' progress
		while (!are_futures_ready<std::vector<R>>(futures)) {
			for (size_t iWorker = 0; iWorker < worker_count; ++iWorker) {
				size_t worker_processed_file_count = processed_file_counts[iWorker].load();
				size_t worker_file_queue_size = file_entry_queues[iWorker].size();
				std::cout << "T" << iWorker << ":" << worker_processed_file_count << "/" << worker_file_queue_size << " # ";
			}
			std::cout << std::endl;
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
		}

		// Wait for all tasks to complete their workload
		for (const auto& future : futures) {
			future.wait();
		}
		std::chrono::steady_clock::time_point crunch_end_time = std::chrono::steady_clock::now();
		std::cout << "Crunched " << total_file_count << " files in " << std::chrono::duration_cast<std::chrono::seconds>(crunch_end_time - crunch_begin_time).count() << " seconds" << std::endl;

		// Aggregate the results of each task into a single vector of results
		// Each element of the returned results vector is the result of a call to crunch_func,
		// i.e. one element of crunch_results = the returned value of crunch_func'ing one slip::Parser/.slp replay file
		// The results are in order of how the files were iterated by the std::filesystem::recursive_directory_iterator
		std::vector<std::vector<R>> future_results;
		for (auto& future : futures) {
			future_results.push_back(std::move(future.get()));
		}
		std::vector<R> crunch_results;
		for (size_t iFileEntry = 0; iFileEntry < total_file_count; ++iFileEntry) {
			size_t worker_index = iFileEntry % worker_count;
			size_t worker_offset = iFileEntry / worker_count;
			crunch_results.push_back(future_results[worker_index][worker_offset]);
		}
		return crunch_results;
	}
}
