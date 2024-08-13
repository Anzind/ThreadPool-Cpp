#pragma once
#include <thread>
#include <queue>
#include <vector>
#include <map>
#include <atomic>
#include <functional>
#include <mutex>
#include <condition_variable>

using namespace std;

/*
* 构成：
* 1.管理者线程 -> 子线程，1个
*		- 控制工作线程的数量：增加或减少
* 2.若干工作线程 -> 子线程，n个
*		- 从任务队列中取任务，并处理
*		- 任务队列为空，则阻塞（条件变量处理）
*		- 线程同步（互斥锁）
*		- 当前数量，空闲的线程数量
*		- 最小，最大线程数量
* 3.任务队列 -> stl->queue
*		- 互斥锁
*		- 条件变量
* 4.线程池开关 -> shutdown(bool)
*/
class ThreadPool {
public:
	ThreadPool(int min = 3, int max = thread::hardware_concurrency());
	~ThreadPool();

	/* 添加任务 */
	void addTask(function<void(void)>);

private:
	void manager();
	void worker();

private:
	thread* t_manager;
	/* 存储已经退出了任务函数的线程ID */
	vector<thread::id> t_workerIds;
	map<thread::id, thread> t_workers;
	atomic<int> minThread;
	atomic<int> maxThread;
	atomic<int> curThread;
	atomic<int> idleThread;
	atomic<int> exitThread;
	atomic<bool> shutdown;

	queue<function<void(void)>> taskQ;
	mutex queueMutex;
	mutex idsMutex;
	condition_variable condition;
};

