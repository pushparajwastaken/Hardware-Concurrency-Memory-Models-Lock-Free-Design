#include <iostream>
#include <thread> //use threads
#include <vector>  //stl vector
#include <string>
#include <chrono>
using namespace std;
// 1. Global Variable / Heap Structure
// Yeh process ke data segment/heap mein rahega aur sabhi threads ke liye SAME hoga.
vector <string> shared_global_list;

void thread_worker(string thread_name) {
    // Global structure ko update karna (Implicit Communication)
    
    shared_global_list.push_back(thread_name + "_global");

    // 2. Local Variable (Allocated on this specific thread's Private Stack)
    string local_variable = "I am private to " + thread_name;

    // Dono variable ke actual memory addresses print karna
    // & operator se hume pointer yaani physical/virtual address milta hai
    cout << "[" << thread_name << "] Global list memory address: " << &shared_global_list << "\n";
    cout << "[" << thread_name << "] Local variable memory address: " << &local_variable << "\n";

    this_thread::sleep_for(std::chrono::milliseconds(100));
}

int main() {
    // Do alag threads create karna
    thread t1(thread_worker, "Thread-1");
    thread t2(thread_worker, "Thread-2");
    thread t3(thread_worker,"Thread-3");

    // Dono threads ke complete hone ka wait karna
    t1.join();
    t2.join();
    t3.join();

    // Final global list print karna
    cout << "\nFinal shared global list content:\n";
    for (const auto& item : shared_global_list) {
        std::cout << "- " << item << "\n";
    }

    return 0;
}
