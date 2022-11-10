#include<iostream>
#include <thread>
using namespace std;
void test(int x){
    cout<<"Thread"<<endl;
    cout<<"Arg: "<<x<<endl;
}
int main(){
    std::thread myThread(&test,100);
    myThread.join();
    cout<<"Main Thread "<<endl;
    return 0;
}