#pragma once

#include "pch.h"

namespace slippcrunch {
	template<typename R>
	struct crunch_params {
		// The per-file processing function (processes one replay/parser). This is the core of what crunching is about.
		R(*crunch_func)(std::unique_ptr<slip::Parser> parser) = nullptr;
		
		// The function to call to report the workers' progress
		void(*progress_report_func)(size_t processed_file_count, size_t total_file_count) = nullptr;
		// The time interval between each call to the progress report function
		std::chrono::milliseconds progress_report_interval = std::chrono::milliseconds(50);

		// How many workers to start (i.e. threads). Will be clamped between [1,std::thread::hardware_concurrency()] (inclusive).
		// By default, leaves one core free for the calling thread.
		size_t desired_worker_count = std::thread::hardware_concurrency() - 1; // by default, leave a core for the calling thread to perform its progress report routine
	};
	
	// This struct serves as a parameter list to be passed to the crunch::execute function.
	// It can be reused for multiple crunch::execute calls, as crunch::execute does not modify any members of the struct
	// (it can write to the log_output however)
	//template<typename R>
	//struct crunch_directory_params {
	//	// Callbacks
	//	R(*crunch_func)(std::unique_ptr<slip::Parser> parser) = nullptr; // the per-file processing function (processes one replay/parser)
	//	//void(*directory_traversal_done_func)(size_t total_file_count) = nullptr;
	//	void(*progress_report_func)(const std::vector<size_t>& worker_processed_file_counts, const std::vector<size_t>& file_queue_sizes) = nullptr;
	//	//void(*crunch_done_func)(size_t total_files_parsed_count, std::chrono::steady_clock::time_point start_time, std::chrono::steady_clock::time_point end_time) = nullptr;

	//	// Directory traversal settings
	//	std::filesystem::path directory_path = std::filesystem::current_path();
	//	bool is_recursive = false;

	//	// Performance settings
	//	size_t desired_worker_count = std::thread::hardware_concurrency() - 1; // by default, leave a core for the calling thread to perform its progress report routine
	//};

	// The crunch class only serves the purpose of managing the access specifiers of functions,
	// which is why all its methods are statics. This is used to determine what is exposed in the public API.
	template<typename R>
	class crunch {
	public:
		static std::vector<R> crunch_directory(const crunch_params<R> params, const std::filesystem::path directory = std::filesystem::current_path(), const bool is_recursive = false) {
			std::vector<std::filesystem::directory_entry> file_entries;
			for (auto& directory_entry : std::filesystem::recursive_directory_iterator(directory)) {
				bool is_file = !directory_entry.is_directory() && (directory_entry.is_regular_file() || directory_entry.is_symlink());
				bool is_slp_file = is_file && directory_entry.path().has_extension() && directory_entry.path().extension() == ".slp";
				if (is_slp_file) {
					file_entries.push_back(std::move(directory_entry));
				}
			}
			return crunch_files(params, file_entries);
		}

		static std::vector<R> crunch_files(const crunch_params<R> params, const std::vector<std::filesystem::directory_entry>& files) {
			const size_t worker_count = std::clamp<size_t>(params.desired_worker_count, 1, std::thread::hardware_concurrency());

			// Setup the file queues for each worker
			std::vector<std::queue<std::filesystem::directory_entry>> worker_file_entry_queues(worker_count);
			std::vector<std::atomic_size_t> worker_processed_file_counts(worker_count);
			size_t total_file_count = 0;
			for (const auto& file_entry : files) {
				bool is_file = !file_entry.is_directory() && (file_entry.is_regular_file() || file_entry.is_symlink());
				bool is_slp_file = is_file && file_entry.path().has_extension() && file_entry.path().extension() == ".slp";
				if (is_slp_file) {
					worker_file_entry_queues[total_file_count % worker_count].push(file_entry);
					total_file_count++;
				}
			}

			// Start the workers
			std::vector<std::future<std::vector<R>>> futures;
			for (size_t iWorker = 0; iWorker < worker_count; ++iWorker) {
				futures.push_back(std::async(
					std::launch::async,
					crunch<R>::worker_func,
					params.crunch_func,
					&(worker_file_entry_queues[iWorker]),
					&(worker_processed_file_counts[iWorker])
				));
			}

			// Log the workers' progress
			if (params.progress_report_func != nullptr) {
				while (!are_futures_ready(futures)) {
					size_t total_processed_filed_count = 0;
					for (const auto& worker_processed_file_count : worker_processed_file_counts) {
						total_processed_filed_count += worker_processed_file_count.load();
					}
					params.progress_report_func(total_processed_filed_count, total_file_count);
					std::this_thread::sleep_for(params.progress_report_interval);
				}
			}

			// Wait for all workers to complete their workload
			for (const auto& future : futures) {
				future.wait();
			}

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
				crunch_results.push_back(std::move(future_results[worker_index][worker_offset]));
			}
			return crunch_results;
		}

	private:
		static std::vector<R> worker_func
		(
			R(*crunch_func)(std::unique_ptr<slip::Parser>),
			std::queue<std::filesystem::directory_entry>* file_entry_queue,
			std::atomic_size_t* processed_file_count
		)
		{
			std::vector<R> results;
			while (!file_entry_queue->empty()) {
				std::unique_ptr<slip::Parser> parser = std::make_unique<slip::Parser>(0);
				bool was_parsing_successful = parser->load(file_entry_queue->front().path().string().c_str());
				if (was_parsing_successful) {
					R crunch_func_result = crunch_func(std::move(parser));
					results.push_back(crunch_func_result);
					processed_file_count->store(processed_file_count->load() + 1);
					file_entry_queue->pop();
				}
			}
			return results;
		}

		static bool are_futures_ready(const std::vector<std::future<std::vector<R>>>& futures) {
			for (const auto& future : futures) {
				auto future_status = future.wait_for(std::chrono::milliseconds::zero());
				if (future_status != std::future_status::ready) {
					return false;
				}
			}
			return true;
		}
	};
}
