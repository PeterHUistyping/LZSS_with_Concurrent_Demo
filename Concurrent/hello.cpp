#include<iostream>
#include <thread>
using namespace std;
// void test(int x){
//     cout<<"Thread"<<endl;
//     cout<<"Arg: "<<x<<endl;
// }
int main(){
    
    auto lambda=[](int x){
    cout<<"Thread"<<endl;
    cout<<"Arg: "<<x<<endl;
    };
    // std::thread myThread(&test,100);
    std::thread myThread(lambda,100);
    myThread.join();
    // The function returns when the thread execution has completed.

    // This synchronizes the moment this function returns with the completion of 
    // all the operations in the thread: This blocks the execution of the thread
    //  that calls this function until the function called on construction 
    //  returns (if it hasn't yet).
    cout<<"Main Thread "<<endl;
    return 0;
}