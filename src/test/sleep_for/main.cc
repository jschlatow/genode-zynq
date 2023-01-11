#include <thread>
#include <iostream>
#include <unistd.h>

void sleeper(unsigned count, unsigned beats_per_interval, unsigned interval_ms, bool print)
{
	for (unsigned i=0; i < count; i++) {
		if (print)
			std::cout << "Napping for " << interval_ms << "ms (" << i << ")" << std::endl;

		for (unsigned b=0; b < beats_per_interval; b++)
			std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms/beats_per_interval));
	}
}

int main()
{
	std::cout << "Starting sleep_for test with 2 threads" << std::endl;

	std::thread t1(sleeper, 200, 50, 1000, true);
	std::thread t2(sleeper, 200, 50, 1000, false);

	t1.join();
	t2.join();

	return 0;
}
