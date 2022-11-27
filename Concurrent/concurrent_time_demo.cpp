#include<iostream>
#include <thread>
#include <vector>
#include <unistd.h>//mac sleep
#include <time.h>
using namespace std;
// void test(int x){
//     cout<<"Thread"<<endl;
//     cout<<"Arg: "<<x<<endl;
// }
int count(int now){//o(1)
    if(now){
        return now;
    }
    return count(now-1);
}
int main(){
    // const int num_threads=10;
 
    // vector<thread> threads;
    // std::thread myThread(&test,100);
         // clock_t tStart = clock();
         time_t timer1 = time(NULL);
         printf("Time taken: %ld\n", timer1);
        //printf("Time taken: %ld\n", ( tStart) );
        // count(1e9);
        sleep(10);
        time_t timer2 = time(NULL);
        printf("Time taken: %ld\n", timer2 );
        
        //printf("Time taken: %ld\n", (clock() ) );
         //printf("Time taken: %.2fs\n", (double)(clock() - tStart)/1000);
    /* auto lambda=[&](int tid){
     sleep(2);
      //assert(chunk_size2[tid]==original_size[tid]);
    };  
    for(int tid=0;tid<num_threads;tid++){
       threads.push_back(thread(lambda,tid));
    }
     for(int tid=0;tid<num_threads;tid++){
        threads[tid].join();
    }*/
        //printf("Time taken: %.2fs\n", (double)(clock() - tStart)/CLOCKS_PER_SEC);
    //  printf("Time taken: %.2fs\n", (double)(clock() - tStart)/1000);
    return 0;
}