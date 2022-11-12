#include <iostream>
#include <chrono>
#include <unistd.h>//mac sleep
using namespace std;
using chrono::high_resolution_clock;
using chrono::milliseconds;

void timeTest()
{
	for(int i =0;i<1000;i++)
		cout<<i<<endl; 
}

int main()
{
    
   
    chrono::time_point<std::chrono::system_clock> begin_time=     
                        std::chrono::system_clock::now();
    sleep(10);
    auto end_time = std::chrono::system_clock::now();
    chrono::duration<double, std::milli> duration_mili = end_time - 
                begin_time;
    
    printf("PrintDuration : duration_mili duration = %ld ms", (long)duration_mili.count());
 
}
 