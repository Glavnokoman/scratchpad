#include <benchmark/benchmark.h>

#include <random>
#include <string>
#include <vector>

#include "vector_erase_indexes.hpp"

namespace {

	auto make_test_vec(size_t vec_size)-> std::vector<std::string> {
		auto r = std::vector<std::string>(vec_size);
		
		auto generator = std::mt19937(21);
		auto len_distribution = std::uniform_int_distribution<size_t>(4, 35);
		auto char_distribution = std::uniform_int_distribution<char>('a', 'z');
		for(auto& s: r){
			auto sl = len_distribution(generator);
			std::generate_n(std::back_inserter(s), sl, [&](){return char_distribution(generator);});
		}
		return r;
	}
	
	auto make_idx_vec(size_t vec_size, size_t remove_size)-> std::vector<ptrdiff_t>{
		auto generator = std::mt19937(21);
		auto id_distribution = std::uniform_int_distribution<>(0, int(vec_size));
		
		auto r = std::vector<ptrdiff_t>(remove_size);
		std::generate(begin(r), end(r), [&](){return id_distribution(generator);});
		
		std::sort(begin(r), end(r));
		return r;
	}
} // namespace 


static void bm_erase_indexes_stable_(benchmark::State& s){
	// create vector of strings
	auto test_vec = make_test_vec(size_t(s.range(0)));
	auto idx_vec = make_idx_vec(size_t(s.range(0)), size_t(s.range(1)));
	while(s.KeepRunning()){
		s.PauseTiming();
		auto t = test_vec;
		s.ResumeTiming();
		erase_indexes_stable_(t, idx_vec);
	}
}

static void bm_erase_indexes_stable(benchmark::State& s){
	// create vector of strings
	auto test_vec = make_test_vec(size_t(s.range(0)));
	auto idx_vec = make_idx_vec(size_t(s.range(0)), size_t(s.range(1)));
	while(s.KeepRunning()){
		s.PauseTiming();
		auto t = test_vec;
		s.ResumeTiming();
		t = erase_indexes_stable(move(t), idx_vec);
	}
}

static void bm_erase_sorted_indexes_stable(benchmark::State& s){
	// create vector of strings
	auto test_vec = make_test_vec(size_t(s.range(0)));
	auto idx_vec = make_idx_vec(size_t(s.range(0)), size_t(s.range(1)));
	while(s.KeepRunning()){
		s.PauseTiming();
		auto t = test_vec;
		s.ResumeTiming();
		t = erase_sorted_indexes_stable(move(t), idx_vec);
	}
}

static void bm_erase_sorted_indexes_unstable(benchmark::State& s){
	// create vector of strings
	auto test_vec = make_test_vec(size_t(s.range(0)));
	auto idx_vec = make_idx_vec(size_t(s.range(0)), size_t(s.range(1)));
	while(s.KeepRunning()){
		s.PauseTiming();
		auto t = test_vec;
		s.ResumeTiming();
		t = erase_sorted_indexes_unstable(move(t), idx_vec);
	}
}

static auto custom_args(benchmark::internal::Benchmark* b)-> void {
	for(int i = 5; i < 13; ++i){
		const auto ix = int(1u << i);
		for(int j = 4; j < ix; j *= 2){
			b->Args({ix, j});
		}
	}
}

BENCHMARK(bm_erase_indexes_stable_)->Apply(custom_args);
BENCHMARK(bm_erase_indexes_stable)->Apply(custom_args);
BENCHMARK(bm_erase_sorted_indexes_stable)->Apply(custom_args);
BENCHMARK(bm_erase_sorted_indexes_unstable)->Apply(custom_args);

BENCHMARK_MAIN();
