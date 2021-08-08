#include <iostream>
#include <semaphore>
#include <thread>
#include <vector>

/// Alternative solution to the problem 3.8.3 (exclusive queues) from the [little book of semaphores](http://greenteapress.com/semaphores/LittleBookOfSemaphores.pdf).
/// 

std::counting_semaphore can_ping(0);
std::counting_semaphore can_pong(0);
std::counting_semaphore balance(1);
std::counting_semaphore neg_balance(0);

auto do_ping()-> void { std::cout << "ping"; }
auto do_pong()-> void { std::cout << "pong"; }

auto ping()-> void {
	can_pong.release();
	can_ping.acquire();

	balance.acquire();
	do_ping();
	neg_balance.release();
}

auto pong()-> void {
	can_ping.release();
	can_pong.acquire();

	neg_balance.acquire();
	do_pong();
	balance.release();
}

auto main()-> int {

	auto ts = std::vector<std::thread>{};
	for(int i = 0; i < 7; ++i){
		ts.emplace_back([]{ for(int j = 0; j < 30000; ++j){ ping(); }});
	}
	for(int i = 0; i < 3; ++i){
		ts.emplace_back([]{for(int j = 0; j < 70000; ++j){ pong(); }});
	}

	for(auto& t: ts ){
		t.join();
	}
	return 0;
}
