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
			const auto processor_count = std::thread::hardware_concurrency();
			const auto worker_thread_count = processor_count > 2 ? processor_count - 1 : 1; // leave 1 processor free for main thread if possible
			// for processor_count, make it std::max(1, processor_count - 1)
			// that way we can have a main thread free for printing info, if not we'll just have to live
			// with the CPU being hogged and having slow info printing

			std::vector<std::thread> threads;
			std::vector<std::future<std::vector<R>>> futures;
			
			for (auto file_entry : std::filesystem::recursive_directory_iterator(m_cruncher_desc.path)) {
				bool is_file = !file_entry.is_directory() && (file_entry.is_regular_file() || file_entry.is_symlink());
				bool is_slp_file = is_file && file_entry.path().has_extension() && file_entry.path().extension() == ".slp";
				if (is_slp_file) {
					std::cout << "Adding " << file_entry.path() << " to the parse queue" << std::endl;
					// TODO : Have one queue per thread to get rid of mutex/sync shared access performance hit
					// and distribute the files across thread file queues to balance byte size and even out performance
					m_file_entries.push(file_entry);
				}
			}
			size_t file_count = m_file_entries.size();

			std::cout << "Starting " << worker_thread_count << " worker threads to parse " << file_count << " files" << std::endl;
			std::chrono::steady_clock::time_point crunch_begin_time = std::chrono::steady_clock::now();
			for (size_t iThread = 0; iThread < worker_thread_count; ++iThread) {
				std::promise<std::vector<R>> promise;
				futures.push_back(std::move(promise.get_future()));
				std::thread thread(&Cruncher<R>::worker_func, this, std::move(promise));
				threads.push_back(std::move(thread));
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

		std::mutex m_file_entries_mutex;
		std::queue<std::filesystem::directory_entry> m_file_entries;

		// should be floating function or not?
		// should templated stuff be marked inline or not?
		void worker_func(std::promise<std::vector<R>>&& promise) {
			std::vector<R> results;

			auto curr_file_entry = pop_file_entry();
			while (curr_file_entry.has_value()) {

				std::cout << "Parsing " << curr_file_entry.value().path() << std::endl;
				std::unique_ptr<slip::Parser> parser = std::make_unique<slip::Parser>(0);
				bool did_parse = parser->load(curr_file_entry.value().path().string().c_str());
				if (did_parse) {
					std::cout << "Crunching " << curr_file_entry.value().path() << std::endl;
					R func_result = m_cruncher_desc.crunch_func(std::move(parser));
					results.push_back(func_result);
				}

				curr_file_entry = pop_file_entry();
			}

			promise.set_value(results);
		}

		std::optional<std::filesystem::directory_entry> pop_file_entry() {
			std::lock_guard lock(m_file_entries_mutex);

			if (!m_file_entries.empty()) {
				auto file_entry = m_file_entries.front();
				m_file_entries.pop();
				return file_entry;
			}

			return std::nullopt;
		}
	};
}