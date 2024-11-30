#include"ThreadPool.h"
//Templates need to be implemented in the .h file to prevent ONE DEFINITION RULE (ODR) from
//being violated. Hence the template Execute Function that returns futures of the packaged_tasks
//can be found in the .h file.
ThreadPool::ThreadPool(int num_threads) : m_threads(num_threads), stop(false) {
		
	//To init the thread pool need to create the threads in while(1) for 100% uptime
	for (int i = 0; i < m_threads; i++) {
		//[capture](params) {} is an anon func. [capture] is for a list of all vars to be 
		//made usable in the anon func. [this] allows all variables in the class to be used
		//emplace back is to create the thread in the vector itself. 
		//alt is to push_back(move(threadname))
		//note btw that ou could very well define another function outside instead of lambda
		threads.emplace_back([this] {
			function<void()> task; //takes task from queue.
			while (1) {
				//Need to lock before using shared threads vector.
				unique_lock<mutex> thread_lock(mtx);
				cv.wait(thread_lock, [this] {
					//if Predicate is true in the function the condition variable continues
					//if False it keeps waiting asleep. so solves busy waiting.
					return (stop or tasks.size());
					});
				
				if (stop) { 
					//exit (threadpool destroyed)
					return;
				}
				//Threads cannot be copied and you must use move to move them.
				task = move(tasks.front());
				tasks.pop();
				thread_lock.unlock();
				//Unlock after reading the task and perform it.
				task();
			}
			});
	}
}
ThreadPool::~ThreadPool() {
	unique_lock<mutex> stop_lock(mtx);
	stop = true;
	stop_lock.unlock();
	//Notifies all the threads of change in cv predicate
	cv.notify_all();
	for (auto &thread : threads) {
		thread.join(); //All threads wait here until each thread is not here. wait() equivalent
	}
}



// int Func(int a) {
// 	this_thread::sleep_for(chrono::seconds(2));
// 	cout << "Executing payload" << endl;
// 	return a;
// }
// int main() {
// 	ThreadPool pool(8);
	
// 	while (1) {
// 		future<int> result = pool.ExecuteTask(Func, 10);
// 	}
// }