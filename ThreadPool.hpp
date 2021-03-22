//
// Created by 11026 on 2020/3/22.
//

#pragma once

#include <vector>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <future>
#include <functional>

namespace ThreadPool {
    class ThreadPool {
    private:
        typedef unsigned int size_t;
        static constexpr size_t INIT_THREAD_NUM = 8;//默认初始化线程数量
        using Task = std::function<void()>;//自定义任务数据类型

        //数据成员
        std::vector<std::thread> pool;//线程池
        std::queue<Task> tasks;//任务队列
        std::mutex lock;//互斥锁,用于生产者消费者程序
        std::condition_variable cond;//条件变量
        std::atomic<bool> isRunning;//线程池是否工作中
        std::atomic<int> idleNum;//空闲线程数量
        std::atomic<bool> destory;//线程池销毁
    public:
        ThreadPool() = delete;
        ThreadPool(const ThreadPool&) = delete;

        ThreadPool(size_t sz = INIT_THREAD_NUM) :isRunning(true), idleNum(sz),destory(false) {
            for (int i = 0; i < idleNum; i++) {
                //初始化线程，调用threadRun函数，由于是调用本对象的threadRun函数，需要隐参数this指针
                pool.emplace_back(&ThreadPool::threadRun, this);
            }
        }
        //关闭线程池
        void close() {
            if (isRunning.load())
                isRunning.store(false);
        }
        //重启线程池
        void open() {
            if (!isRunning.load())
                isRunning.store(true);
            cond.notify_all();
        }

        //判断线程池是否关闭
        bool isclosed() const {
            return !isRunning.load();
        }

        // 获取当前任务队列中的任务数
        size_t size() {
            std::unique_lock<std::mutex> m_lock(lock);
            return tasks.size();
        }

        // 获取当前空闲线程数
        size_t count() {
            return idleNum;
        }

        //传入运行函数及其参数，内部进行绑定
        //借鉴自https://my.oschina.net/propagator/blog/2985836
        template<typename F, typename... Args>
        auto add(F&& f, Args&& ... args)->std::future<decltype(f(args...))>
        {
            if (!isRunning.load())
            {
                throw std::runtime_error("ThreadPool is closed, can not submit task.");
            }

            using RetType = decltype(f(args...));

            //此处使用只能指针是因为下一步向队列中提交lambda表达式需要传入函数指针进行运行任务，若是普通变量则会因为线程异步运行
            //在此函数运行结束后被销毁，无法被线程运行，同理，普通指针需要在函数运行结束时进行销毁，但此函数可能先于线程执行完毕，故
            //使用只能指针
            std::shared_ptr<std::packaged_task<RetType()>> task = std::make_shared<std::packaged_task<RetType()>>(
                    std::bind(std::forward<F>(f), std::forward<Args>(args)...)
            );

            std::future<RetType> future = task->get_future();
            addTask([task]() {
                (*task)();
            });
            return future;
        }

        template<typename F, typename... Args>
        auto add(const F& f,const  Args& ... args)->std::future<decltype(f(args...))>
        {
            if (!isRunning.load())
            {
                throw std::runtime_error("ThreadPool is closed, can not submit task.");
            }

            using RetType = decltype(f(args...));

            //此处使用只能指针是因为下一步向队列中提交lambda表达式需要传入函数指针进行运行任务，若是普通变量则会因为线程异步运行
            //在此函数运行结束后被销毁，无法被线程运行，同理，普通指针需要在函数运行结束时进行销毁，但此函数可能先于线程执行完毕，故
            //使用只能指针
            std::shared_ptr<std::packaged_task<RetType()>> task = std::make_shared<std::packaged_task<RetType()>>(
                    std::bind(std::forward<F>(f), std::forward<Args>(args)...)
            );

            std::future<RetType> future = task->get_future();
            addTask([task]() {
                (*task)();
            });
            return future;
        }


        ~ThreadPool() {
            while (!tasks.empty())
            {
                tasks.pop();
            }
            destory.store(true);
            //此处是为了让所有空闲线程都能够结束线程，放入空任务
            for (int i = 0; i < idleNum; i++)
                addTask(Task([]() {}));
            cond.notify_all();
            for (auto& thread : pool)
            {
                if(thread.joinable())
                    thread.join();
            }
            pool.clear();
            isRunning.store(false);
        }
    private:
        //内部函数


        //生产者程序，
        void addTask(const Task& task)
        {
            std::unique_lock<std::mutex> m_lock(lock);
            tasks.push(task);
            cond.notify_one();
        }
        //消费者程序,当队列为空或线程池关闭时挂起
        Task getTask()
        {
            std::unique_lock<std::mutex> m_lock(lock);
            while ((tasks.empty() || !isRunning.load())) {
                cond.wait(m_lock);
            }
            Task task = std::move(tasks.front());
            tasks.pop();
            return task;
        }

        //线程所执行的函数，当任务队列为空时，挂起，当线程池关闭时,也挂起
        void threadRun()
        {
            while (!destory.load())//线程未被销毁
            {
                auto&& task = getTask();
                idleNum--;//原子变量，简单修改不需要互斥
                task();
                idleNum++;
            }
        }
    };
}

