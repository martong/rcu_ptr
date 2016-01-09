#pragma once

#include <random>

namespace detail {

struct RandomWait {
	void operator()() {
		static const constexpr std::size_t MAX = 100000;
		std::uniform_int_distribution<std::size_t> dist(0, MAX);
		std::size_t N = dist(random_engine);
		for (std::size_t i = 0; i < N; ++i) {
			//dummy = ++dummy * 3 - 2;

			// http://stackoverflow.com/questions/7083482/how-to-prevent-gcc-from-optimizing-out-a-busy-wait-loop
			__asm__ __volatile__("");
		}
	}
	//int dummy = 1;

	std::mt19937 random_engine{ std::random_device{}() };
};

template <std::size_t N, typename F>
struct InLoopExecutor {
	InLoopExecutor(F f) : f(f) {}
	RandomWait random_wait{};
	void operator()() {
		for (std::size_t i = 0; i < N; ++i) {
			f();
			random_wait();
		}
	}
	F f;
};

} // namespace detail

template <std::size_t N, typename F>
auto makeInLoopExecutor(F f) {
	return detail::InLoopExecutor<N, F>{f};
}

template <std::size_t N, typename F>
void executeInLoop(F f) {
	detail::InLoopExecutor<N, F>{f} ();
}
