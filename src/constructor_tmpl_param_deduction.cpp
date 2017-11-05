#include <iostream>

template<class T>
struct test{};

template<class T>
struct test<T*> {
	explicit test(T*){ std::cout << "non-const version chosen" << "\n";}
};

template<class T>
struct test<const T*> {
	explicit test(const T*){ std::cout << "const version chosen" << "\n";}
};

// non-const or both of next two are required to run properly
template<class T> test(T*)-> test<T*>;             // this alone works OK alone or combined with const T*,
template<class T> test(const T*)-> test<const T*>; // this alone results in false version chosen, thing compiles but runs with a surprise

int main(int argc, char const *argv[]) {
	const char* cbuf = nullptr;
	auto t1 = test{cbuf};
	
	char* buf = nullptr;
	auto t2 = test{buf};
	
	return 0;
}
