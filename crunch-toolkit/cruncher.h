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
		R(*crunch_func)(slip::Parser&) = nullptr;
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
			
			std::vector<std::thread> threads(worker_thread_count);
			std::vector<std::future<std::vector<R>>> futures(worker_thread_count);
			
			for (auto file_entry : std::filesystem::recursive_directory_iterator(m_cruncher_desc.path)) {
				if (!file_entry.is_directory() && (file_entry.is_regular_file() || file_entry.is_symlink())) {
					m_file_entries.push(file_entry);
				}
			}

			// for processor_count, make it std::max(1, processor_count - 1)
			// that way we can have a main thread free for printing info, if not we'll just have to live
			// with the CPU being hogged and having slow info printing
			for (std::size_t iThread = 0; iThread < worker_thread_count; ++iThread) {
				std::promise<std::vector<R>> promise;
				futures.push_back(std::move(promise.get_future()));
				threads.emplace_back(&Cruncher<R>::worker_func, this, std::move(promise));
			}

			for (auto& thread : threads) {
				thread.join();
			}

			std::vector<R> results;
			for (auto& future : futures) {
				results.insert(results.end(), future.get().begin(), future.get().end());
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

				slip::Parser parser(0);
				bool did_parse = parser.load(curr_file_entry.value().path().string().c_str());
				if (did_parse) {
					R func_result = m_cruncher_desc.crunch_func(parser);
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