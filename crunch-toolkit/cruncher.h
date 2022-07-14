#pragma once

#include "pch.h"

namespace slippcrunch {

	bool is_file_entry_queue_empty(std::mutex* file_entry_queue_mutex, std::queue<std::filesystem::directory_entry>* file_entry_queue) {
		std::lock_guard file_entry_queue_lock(*file_entry_queue_mutex);
		return file_entry_queue->empty();
	}

	std::optional<std::filesystem::directory_entry> pop_file_entry(std::mutex* file_entry_queue_mutex, std::queue<std::filesystem::directory_entry>* file_entry_queue) {
		std::lock_guard file_entry_queue_lock(*file_entry_queue_mutex);
		if (!file_entry_queue->empty()) {
			auto file_entry = file_entry_queue->front();
			file_entry_queue->pop();
			return file_entry;
		}
		return std::nullopt;
	}

	template<typename R>
	std::vector<R> crunch_files
	(
		R(*crunch_func)(std::unique_ptr<slip::Parser>),
		std::mutex* file_entry_queue_mutex,
		std::queue<std::filesystem::directory_entry>* file_entry_queue,
		std::atomic_size_t* processed_file_count,
		std::atomic_bool* file_traversal_done
	)
	{
		std::vector<R> results;
		while (!is_file_entry_queue_empty(file_entry_queue_mutex, file_entry_queue) || !file_traversal_done->load()) {
			auto opt_file_entry = pop_file_entry(file_entry_queue_mutex, file_entry_queue);
			if (opt_file_entry.has_value()) {
				std::unique_ptr<slip::Parser> parser = std::make_unique<slip::Parser>(0);
				bool did_parse = parser->load(opt_file_entry.value().path().string().c_str());
				if (did_parse) {
					R crunch_func_result = crunch_func(std::move(parser));
					results.push_back(crunch_func_result);
					processed_file_count->store(processed_file_count->load() + 1);
				}
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

		// Setup the arguments to be passed to the workers
		std::vector<std::mutex> file_entry_queue_mutexes(worker_count);
		std::vector<std::queue<std::filesystem::directory_entry>> file_entry_queues(worker_count);
		std::vector<std::atomic_size_t> processed_file_counts(worker_count);
		for (auto& element : processed_file_counts) { element.store(0); }
		std::vector<std::atomic_bool> file_traversal_done_flags(worker_count);
		for (auto& element : file_traversal_done_flags) { element.store(false); }
		
		// Start the tasks
		std::cout << "Starting " << worker_count << " workers to parse " /* << total_file_count << " files" */ << std::endl;
		std::vector<std::future<std::vector<R>>> futures;
		std::chrono::steady_clock::time_point crunch_begin_time = std::chrono::steady_clock::now();
		for (size_t iWorker = 0; iWorker < worker_count; ++iWorker) {
			futures.push_back(std::async(
				std::launch::async,
				crunch_files<R>,
				crunch_func,
				&(file_entry_queue_mutexes[iWorker]),
				&(file_entry_queues[iWorker]),
				&(processed_file_counts[iWorker]),
				&(file_traversal_done_flags[iWorker])
			));
		}

		// Fill the file entry queues
		size_t total_file_count = 0;
		std::chrono::steady_clock::time_point file_traversal_begin_time = std::chrono::steady_clock::now();
		for (const auto& file_entry : std::filesystem::recursive_directory_iterator(path)) {
			bool is_file = !file_entry.is_directory() && (file_entry.is_regular_file() || file_entry.is_symlink());
			bool is_slp_file = is_file && file_entry.path().has_extension() && file_entry.path().extension() == ".slp";
			if (is_slp_file) {
				size_t iWorker = total_file_count % worker_count;
				std::lock_guard file_entry_queue_lock(file_entry_queue_mutexes[iWorker]);
				file_entry_queues[iWorker].push(file_entry);
				total_file_count++;
			}
		}
		std::chrono::steady_clock::time_point file_traversal_end_time = std::chrono::steady_clock::now();
		for (auto& element : file_traversal_done_flags) { element.store(true); }

		// Log the tasks' progress
		while (!are_futures_ready<std::vector<R>>(futures)) {
			for (size_t iWorker = 0; iWorker < worker_count; ++iWorker) {
				size_t worker_processed_file_count = processed_file_counts[iWorker].load();
				std::lock_guard file_entry_queue_lock(file_entry_queue_mutexes[iWorker]);
				size_t worker_file_queue_size = file_entry_queues[iWorker].size();
				std::cout << "T" << iWorker << ":" << worker_processed_file_count << "/" << worker_processed_file_count + worker_file_queue_size << " # ";
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
