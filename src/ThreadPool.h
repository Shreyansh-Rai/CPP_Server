#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <iostream>
#include <thread>
#include <mutex>
#include <functional>
#include <vector>
#include <queue>
#include <condition_variable>
#include <future>

using namespace std;

class ThreadPool {
private:
    int m_threads;
    bool stop; // Signal to stop the thread pool
    vector<thread> threads; // All threads in the pool
    queue<function<void()>> tasks; // All tasks in the queue
    mutex mtx;
    condition_variable cv;

public:
    explicit ThreadPool(int num_threads);
    ~ThreadPool();

    //Since function could return any type and accept any params make it a template.
    //Since async exec takes place it returns a future. This btw is called a trailing return
    //decltype works out the return type of f(args...) on it's  own. && is for perfect forwarding in bind.
    template<class F, class... Args> 
    auto ExecuteTask(F&& f, Args&&... args) -> future<decltype(f(args...))> {
        using return_type = decltype(f(args...));
        //he class template std::packaged_task wraps any Callable target 
        //(function, lambda expression, bind expression, or another function object) 
        //so that it can be invoked asynchronously. Its return value or exception thrown 
        //is stored in a shared state which can be accessed through std::future objects.
        //Since our end goal is to make a task as a function<void()> it takes in no inputs.
        //instead we bind everything. g = bind(f,args...) => g() = f(args...) forward preserves input type
        //call by value/ref remains the same.
        //Using forwarding references takes advantage of C++'s reference collapsing rules:
        //If the argument is an lvalue, T&& deduces as T& .
        //If the argument is an rvalue, T&& remains T&& .
        //This behavior allows the arguments to be forwarded with std::forward, 
        //preserving their original value category. this needs to be a shared pointer so any thread can
        //access the task packaged_task<return_type()> should be wrapped in make_shared<> 
        //make_shared<T>(args) = make a shared pointer out of T(args) hence task is a shared ptr of
        //packaged_task<return_type()>(bind(forward<F>(f), forward<Args>(args)...))
        auto task = make_shared<packaged_task<return_type()>>(bind(forward<F>(f), forward<Args>(args)...));
        future<return_type> res = task->get_future();
        unique_lock<mutex> tasks_lock(mtx);
        //Since the tasks is expecting a function<void()> we make it so that the void function calls the task
        tasks.emplace([task]() {
            (*task)();
            });
        tasks_lock.unlock();
        //any one waiting task is notified.
        cv.notify_one();
        return res;
    }
};

#endif // THREADPOOL_H
