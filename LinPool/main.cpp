#include <atomic>
#include <cassert>
#include <thread>
#include <iostream>
#include <chrono>
#include <vector>

std::atomic<std::string*> ptr{ nullptr };

// 生产者
void producer() {
    auto* p = new std::string("Hello");
    ptr.store(p, std::memory_order_release); // 发布指针
}

// 消费者
void consumer() {
    std::string* p;
    while (!(p = ptr.load(std::memory_order_acquire))); // 等待指针发布
    assert(*p == "Hello"); // 正确断言
	std::cout << *p << '\n';
}

int main() {

    std::thread f(producer);
    std::thread f2(consumer);
    f2.join();

    f.join();

    return 0;
}
