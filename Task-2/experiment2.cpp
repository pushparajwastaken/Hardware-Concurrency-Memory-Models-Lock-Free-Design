#include<iostream>
using namespace std;
int independent(int a, int b, int c, int d) {
    int x = a + b;
    int y = c + d;
    return x * y;
}
int dependent(int a, int b) {
    int x = a + b;
    int y = x + 10;
    int z = y * 2;
    return z;
}
int main()
{
    cout<<independent(1,23,3,4)<<endl;
    cout<<dependent(10,20)<<endl;
    return 0;
}