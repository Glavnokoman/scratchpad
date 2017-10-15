#define CATCH_CONFIG_RUNNER
#include <catch/catch.hpp>

#include "bufbuf.hpp"

#include <algorithm>
#include <chrono>
#include <numeric>
#include <vector>
#include <thread>

using namespace std::chrono;

namespace {
	using value_t = uint8_t;
	using buf_t = BufBuf<value_t[]>;
	static const size_t PRODUCERS = 3;
	static const size_t SLOTSIZE = 16;
	static const auto TESTIME = 300ms; // time to run reader

	struct Writer {
		buf_t& buf;
		value_t* writeslot;
		size_t counter;
		bool stop; // dirty stopper
		
		explicit Writer(buf_t& buf) : buf(buf), writeslot(buf.getWriteSlot()), stop(false) {}
		
		template<class Rep, class Period>
		auto operator()(const duration<Rep, Period>& delay, value_t val)-> void {
			for(counter = 0; !stop; counter++ ){
				std::fill_n(writeslot, SLOTSIZE, val + counter % 16);
				writeslot = buf.publish(writeslot);
				std::this_thread::sleep_for(delay);
			}
		}
	}; // struct Producer
	
	struct Reader
	{
		buf_t& buf;
		std::vector<size_t> occs; // data occurences, maybe test for some statistics
		bool datafail = false;
		
		explicit Reader(buf_t& buf): buf(buf), occs(1u << 8*sizeof(value_t), 0){}
		
		template<class Rep, class Period>
		auto operator()(const duration<Rep, Period>& delay)-> void {
			auto then = high_resolution_clock::now();
			for(;duration_cast<milliseconds>(high_resolution_clock::now() - then) < TESTIME;){
				auto readslot = buf.read();
				std::this_thread::sleep_for(delay);
				datafail |= (readslot[0] != readslot[SLOTSIZE - 1]);
				if(readslot[0] != readslot[SLOTSIZE -1]){
					std::cerr << "datafail: " << readslot[0] << ": " << readslot[SLOTSIZE - 1] << "\n"; 
				}
				occs[readslot[0]] += 1;
			}
		}
	};
	
	std::vector<size_t> counters_all_in(PRODUCERS+1);
	
} // namespace 

TEST_CASE(){
	buf_t buf(PRODUCERS, SLOTSIZE);
	auto writers = std::vector<Writer>{};
	for(size_t i = 0; i < PRODUCERS; ++i){ writers.emplace_back(buf); }
	Reader rd(buf);
	const auto values = std::vector<value_t>{30, 50, 70}; // start of a 16-range of values written by each writer
	std::vector<std::thread> threads;
	std::vector<size_t> counters(PRODUCERS+1, 0);
	
	SECTION("burf on too many producers"){
		CHECK_THROWS([&](){
			writers.emplace_back(buf);
		}());
	}
	SECTION("all full speed"){
		auto delays = std::vector<nanoseconds>{0ns, 0ns, 0ns, 0ns};
		for(size_t i = 0; i < writers.size(); ++i){
			threads.emplace_back(std::ref<Writer>(writers[i]), delays[i], values[i]);
		}
		std::thread tr(std::ref<Reader>(rd), delays.back());
		tr.join();
		for(auto& w: writers){ w.stop = true; }
		for(auto& t: threads){ t.join(); }
		
		for(size_t i = 0; i < PRODUCERS; ++i){ counters[i] = writers[i].counter; }
		counters.back() = std::accumulate(begin(rd.occs), end(rd.occs), 0u);
		
		counters_all_in = counters;
		
		CHECK(rd.datafail == false); // all reads and writes are kinda atomic
	}
	SECTION("fast producer slow consumer"){
		auto delays = std::vector<milliseconds>{0ms, 0ms, 0ms, 1ms};
		for(size_t i = 0; i < writers.size(); ++i){
			threads.emplace_back(std::ref<Writer>(writers[i]), delays[i], values[i]);
		}
		std::thread tr(std::ref<Reader>(rd), delays.back());
		tr.join();
		for(auto& w: writers){ w.stop = true; }
		for(auto& t: threads){ t.join(); }

		for(size_t i = 0; i < PRODUCERS; ++i){ counters[i] = writers[i].counter; }
		counters.back() = std::accumulate(begin(rd.occs), end(rd.occs), 0u);

		const auto total_prod = std::accumulate(begin(counters), begin(counters) + PRODUCERS, 0u);
		const auto total_prod_ai = std::accumulate(begin(counters_all_in), begin(counters_all_in) + PRODUCERS, 0u);		

		CHECK(rd.datafail == false);       // all reads and writes are kinda atomic
		CHECK(total_prod > total_prod_ai); // producers are not blocked by slow consumer
	}
	SECTION("slow producers, fast consumer"){
		auto delays = std::vector<milliseconds>{1ms, 3ms, 2ms, 0ms};
		for(size_t i = 0; i < writers.size(); ++i){
			threads.emplace_back(std::ref<Writer>(writers[i]), delays[i], values[i]);
		}
		std::thread tr(std::ref<Reader>(rd), delays.back());
		tr.join();
		for(auto& w: writers){ w.stop = true; }
		for(auto& t: threads){ t.join(); }
		
		for(size_t i = 0; i < PRODUCERS; ++i){ counters[i] = writers[i].counter; }
		counters.back() = std::accumulate(begin(rd.occs), end(rd.occs), 0u);

		const auto total_prod = std::accumulate(begin(counters), begin(counters) + PRODUCERS, 0u);
		
		CHECK(rd.datafail == false);          // all reads and writes are kinda atomic
		CHECK(total_prod >= counters.back()); // reader is blocked by publishers
	}
}

int main( int argc, char* argv[] )
{
	// global setup...
	int result = Catch::Session().run( argc, argv );
	// global clean-up...
	return ( result < 0xff ? result : 0xff );
}
