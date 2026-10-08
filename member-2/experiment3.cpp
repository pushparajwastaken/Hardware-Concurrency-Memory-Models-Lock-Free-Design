#include<iostream>
using namespace std;
int branch_test(int x) {
    if (x > 100)
        return x + 10;
    else
        return x - 10;
}