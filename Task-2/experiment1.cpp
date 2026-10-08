#include <iostream>

int foo(int a, int b) {
    int x = a + 1;
    int y = b + 2;
    return x * y;
}

int main() {
    std::cout << foo(10, 20) << '\n';
}

