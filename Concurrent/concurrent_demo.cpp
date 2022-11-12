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
    vector<int> res;
    for(int i=0;i<num_threads+1;i++){
        res.push_back(0);
        // threads[i].join();
    }
     char buffer[10];
    auto lambda=[&res, &buffer](int tid){
    cout<<"Thread"<<&this_thread::get_id<<endl;
    cout<<"Arg: "<<tid<<endl;
    //unsigned long long num_th=  std::hash<std::thread::id>{}(std::this_thread::get_id());
    buffer[1]='s';
    res[tid]=tid;
    };  
    vector<thread> threads;
    // std::thread myThread(&test,100);
   
    for(int tid=0;tid<num_threads;tid++){
        threads.push_back( thread  (lambda,tid));
        // threads[i].join();
    }
    for(int tid=0;tid<num_threads;tid++){
        threads[tid].join();
    }
      for(int i=0;i<num_threads+1;i++){
        cout<<res[i];
        // threads[i].join();
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