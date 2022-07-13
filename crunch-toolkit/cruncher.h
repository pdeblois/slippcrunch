#pragma once

#include "pch.h"

//template<typename R>
//void worker_func(std::promise<R>&& promise) {
//	std::vector<R> result;
//
//	promise.set_value(result);
//}

namespace Crunch {
	/*using ComboScoreFunc = std::add_pointer_t<long long(const slip::SlippiReplay& combo)>;
	class ComboCruncher {
		std::vector<Combo> Crunch(const ComboScoreFunc combo_score_func);
	};
	using CrunchFuncPtr = std::add_pointer_t<void*(const slip::SlippiReplay& combo)>;*/
	template<typename R>
	struct CruncherDesc {
		R(*crunch_func)(std::unique_ptr<slip::Parser>) = nullptr;
		std::filesystem::path path;
		bool is_recursive = false;
	};
	template<typename R>
	class Cruncher {
		//typedef RETURN_TYPE(*FUNC_PTR)(const slip::SlippiReplay& replay);
		//FUNC_PTR func;
	public:
		Cruncher(CruncherDesc<R> cruncher_desc) : m_cruncher_desc(cruncher_desc) {
			// empty ctor, nothing to do here (m_cruncher_desc already assigned through initializer list)
		}
		std::vector<R> Crunch() {
			const size_t processor_count = std::thread::hardware_concurrency();
			const size_t worker_thread_count = processor_count > 2 ? processor_count - 1 : 1; // leave 1 processor free for main thread if possible
			// for processor_count, make it std::max(1, processor_count - 1)
			// that way we can have a main thread free for printing info, if not we'll just have to live
			// with the CPU being hogged and having slow info printing

			std::vector<std::queue<std::filesystem::directory_entry>> file_entry_queues(worker_thread_count);
			std::vector<std::atomic_size_t> processed_file_counts(worker_thread_count);
			
			size_t file_count = 0;
			for (const auto& file_entry : std::filesystem::recursive_directory_iterator(m_cruncher_desc.path)) {
				bool is_file = !file_entry.is_directory() && (file_entry.is_regular_file() || file_entry.is_symlink());
				bool is_slp_file = is_file && file_entry.path().has_extension() && file_entry.path().extension() == ".slp";
				if (is_slp_file) {
					std::cout << "Adding " << file_entry.path() << " to the parse queue" << std::endl;
					file_entry_queues[file_count % worker_thread_count].push(file_entry);
					file_count++;
				}
			}

			std::vector<std::thread> threads;
			std::vector<std::future<std::vector<R>>> futures;
			
			std::cout << "Starting " << worker_thread_count << " worker threads to parse " << file_count << " files" << std::endl;
			std::chrono::steady_clock::time_point crunch_begin_time = std::chrono::steady_clock::now();
			for (size_t iThread = 0; iThread < worker_thread_count; ++iThread) {
				std::promise<std::vector<R>> promise;
				futures.emplace_back(std::move(promise.get_future()));
				threads.emplace_back(&Cruncher<R>::worker_func, this, file_entry_queues[iThread], &(processed_file_counts[iThread]), std::move(promise));
			}

			while (running_thread_count(&futures) > 0) {
				for (size_t iThread = 0; iThread < worker_thread_count; ++iThread) {
					size_t thread_processed_file_count = (processed_file_counts[iThread]).load();
					//int thread_processed_file_count = 0;
					size_t thread_file_queue_size = file_entry_queues[iThread].size();
					//float thread_progress = static_cast<float>(thread_processed_file_count) / static_cast<float>(thread_file_queue_size);
					std::cout << "T" << iThread << ":" << thread_processed_file_count << "/" << thread_file_queue_size << " # ";
				}
				std::cout << std::endl;
				std::this_thread::sleep_for(std::chrono::milliseconds(500));
			}

			for (auto& thread : threads) {
				thread.join();
			}
			std::chrono::steady_clock::time_point crunch_end_time = std::chrono::steady_clock::now();
			std::cout << "Crunched " << file_count << " files in " << std::chrono::duration_cast<std::chrono::seconds>(crunch_end_time - crunch_begin_time).count() << " seconds" << std::endl;

			std::vector<R> results;
			for (auto& future : futures) {
				auto future_result = future.get();
				results.insert(results.end(), future_result.begin(), future_result.end());
			}
			return results;
		}
	private:
		CruncherDesc<R> m_cruncher_desc;

		// should be floating function or not?
		// should templated stuff be marked inline or not?
		void worker_func(std::queue<std::filesystem::directory_entry> file_entry_queue, std::atomic_size_t* processed_file_count, std::promise<std::vector<R>>&& promise) {
			std::vector<R> results;

			auto curr_file_entry = pop_file_entry(&file_entry_queue);
			while (curr_file_entry.has_value()) {

				//std::cout << "Parsing " << curr_file_entry.value().path() << std::endl;
				std::unique_ptr<slip::Parser> parser = std::make_unique<slip::Parser>(0);
				bool did_parse = parser->load(curr_file_entry.value().path().string().c_str());
				if (did_parse) {
					//std::cout << "Crunching " << curr_file_entry.value().path() << std::endl;
					R func_result = m_cruncher_desc.crunch_func(std::move(parser));
					results.push_back(func_result);
					processed_file_count->store(processed_file_count->load() + 1);
				}

				curr_file_entry = pop_file_entry(&file_entry_queue);
			}

			promise.set_value(results);
		}

		std::optional<std::filesystem::directory_entry> pop_file_entry(std::queue<std::filesystem::directory_entry>* file_entry_queue) {

			if (!file_entry_queue->empty()) {
				auto file_entry = file_entry_queue->front();
				file_entry_queue->pop();
				return file_entry;
			}

			return std::nullopt;
		}

		size_t running_thread_count(std::vector<std::future<std::vector<R>>>* futures) {
			size_t running_thread_count = 0;
			for (auto& future : (*futures)) {
				auto status = future.wait_for(std::chrono::milliseconds::zero());
				if (status != std::future_status::ready) {
					running_thread_count++;
				}
			}
			return running_thread_count;
		}
	};
}