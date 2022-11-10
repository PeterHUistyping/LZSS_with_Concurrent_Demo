#include<iostream>
#include <thread>
#include <vector>
using namespace std;
// void test(int x){
//     cout<<"Thread"<<endl;
//     cout<<"Arg: "<<x<<endl;
// }
int main(){
    const int num_threads=10;
    auto lambda=[](int x){
    cout<<"Thread"<<&this_thread::get_id<<endl;
    cout<<"Arg: "<<x<<endl;
    };  vector<thread> threads;
    // std::thread myThread(&test,100);
    for(int i=0;i<num_threads;i++){
        threads.push_back( thread  (lambda,i));
        // threads[i].join();
    }
    for(int i=0;i<num_threads;i++){
        threads[i].join();
    }
  
    // myThread.join();

    // The function returns when the thread execution has completed.

    // This synchronizes the moment this function returns with the completion of 
    // all the operations in the thread: This blocks the execution of the thread
    //  that calls this function until the function called on construction 
    //  returns (if it hasn't yet).
    cout<<"Main Thread "<<endl;
    return 0;
}