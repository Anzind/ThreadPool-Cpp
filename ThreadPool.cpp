﻿#include "ThreadPool.h"
#include <iostream>

const int number = 2;

ThreadPool::ThreadPool(int min, int max) :minThread(min), maxThread(max),
shutdown(false), curThread(min), idleThread(min), exitThread(0) {
	/* 创建管理者线程 */
	t_manager = new thread(&ThreadPool::manager, this);
	/* 创建工作线程 */
	for (int i = 0; i < min; i++) {
		//t_workers.emplace_back(thread(&ThreadPool::worker, this));
		thread t(&ThreadPool::worker, this);
		t_workers.insert(make_pair(t.get_id(), move(t)));
	}
}

void ThreadPool::addTask(function<void(void)> task) {
	{
		lock_guard<mutex> locker(queueMutex);
		taskQ.emplace(task);
	}
	condition.notify_one();
}

void ThreadPool::manager() {
	while (!shutdown.load()) {
		this_thread::sleep_for(chrono::seconds(3));
		int idle = idleThread.load();
		int cur = curThread.load();

		if (idle > cur / 2 && cur > minThread) {
			/* 若空闲线程>当前线程/2，则释放线程 */
			exitThread.store(number);
			condition.notify_all();

			lock_guard<mutex> idsLocker(idsMutex);
			for (auto id : t_workerIds) {
				auto it = t_workers.find(id);
				if (it != t_workers.end()) {
					(*it).second.join();
					t_workers.erase(it);
				}
			}
			t_workerIds.clear();
		}
		else if (idle == 0 && cur < maxThread) {
			/* 若空闲线程为0，则添加线程 */
			//t_workers.emplace_back(thread(&ThreadPool::worker, this));
			thread t(&ThreadPool::worker, this);
			t_workers.insert(make_pair(t.get_id(), move(t)));
			curThread++;
			idleThread++;
		}
	}
}

void ThreadPool::worker() {
	while (!shutdown.load()) {
		function<void(void)> task = nullptr;
		{
			unique_lock<mutex> locker(queueMutex);
			while (taskQ.empty() && shutdown) {
				condition.wait(locker);
				/* 唤醒后需要退出 */
				if (exitThread > 0) {
					curThread--;
					exitThread--;
					cout << "-------线程退出，ID:" << this_thread::get_id() << endl;
					lock_guard<mutex> idsLocker(idsMutex);
					t_workerIds.push_back(this_thread::get_id());
					return;
				}
			}
			if (!taskQ.empty()) {
				task = move(taskQ.front());
				taskQ.pop();
			}
		}
		if (task) {
			idleThread--;
			task();
			idleThread++;
		}
	}
}
