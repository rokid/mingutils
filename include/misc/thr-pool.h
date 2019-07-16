#pragma once

#include <functional>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <chrono>
#include <list>
#include <vector>
#include <algorithm>

#define TASKTHREAD_FLAG_CREATED 1
#define TASKTHREAD_FLAG_SHOULD_EXIT 2
#define TASK_OP_QUEUE 0
#define TASK_OP_DOING 1
#define TASK_OP_DONE 2
#define TASK_OP_DISCARD 3

class ThreadPool {
private:
  typedef std::function<void()> TaskFunc;
  /// \param op  0: queue  1: doing  2: done  3: discard
  typedef std::function<void(int32_t op)> TaskCallback;

  class TaskInfo {
  public:
    TaskInfo() {
    }

    TaskInfo(TaskFunc f, TaskCallback c) : func(f), cb(c) {
    }

    TaskFunc func;
    TaskCallback cb;
  };

  class TaskThread {
  public:
    TaskThread() {
    }

    TaskThread(const TaskThread &&o) {
    }

    void set_thr_pool(ThreadPool *pool) {
      the_pool = pool;
    }

    void awake() {
      std::unique_lock<std::mutex> locker(thr_mutex);
      if ((flags & TASKTHREAD_FLAG_CREATED) == 0
          && (flags & TASKTHREAD_FLAG_SHOULD_EXIT) == 0) {
        // init
        thr = std::thread([this]() {
            this->run();
            });
        flags |= TASKTHREAD_FLAG_CREATED;
      } else {
        // launch task
        thr_cond.notify_one();
      }
    }

    void exit() {
      std::unique_lock<std::mutex> locker(thr_mutex);
      flags |= TASKTHREAD_FLAG_SHOULD_EXIT;
      if (thr.joinable()) {
        thr_cond.notify_one();
        locker.unlock();
        thr.join();
        locker.lock();
        flags = 0;
      }
    }

  private:
    void run() {
      std::unique_lock<std::mutex> locker(thr_mutex);
      thr_cond.notify_one();

      while ((flags & TASKTHREAD_FLAG_SHOULD_EXIT) == 0) {
        if (the_pool->get_pending_task(task)) {
          if (task.cb)
            task.cb(TASK_OP_DOING);
          if (task.func)
            task.func();
          if (task.cb)
            task.cb(TASK_OP_DONE);
        } else {
          the_pool->push_idle_thread(this);
          thr_cond.wait(locker);
        }
      }
    }

  private:
    ThreadPool *the_pool = nullptr;
    std::thread thr;
    std::mutex thr_mutex;
    std::condition_variable thr_cond;
    TaskInfo task;
    uint32_t flags = 0;
  };

public:
  explicit ThreadPool(uint32_t max) {
    thread_array.resize(max);

    uint32_t i;
    for (i = 0; i < max; ++i) {
      thread_array[i].set_thr_pool(this);
    }
    init_idle_threads();
  }

  ~ThreadPool() {
    finish();
  }

  void push(TaskFunc task, TaskCallback cb = nullptr) {
    std::unique_lock<std::mutex> locker(task_mutex);
    pending_tasks.push_back({ task, cb });
    if (cb)
      cb(TASK_OP_QUEUE);
    if (idle_threads.empty())
      return;
    auto thr = idle_threads.front();
    idle_threads.pop_front();
    locker.unlock();
    thr->awake();
  }

  void finish() {
    size_t sz = thread_array.size();
    size_t i;
    for (i = 0; i < sz; ++i) {
      thread_array[i].exit();
    }
    std::lock_guard<std::mutex> locker(task_mutex);
    for_each(pending_tasks.begin(), pending_tasks.end(), [](TaskInfo& task) {
      if (task.cb)
        task.cb(TASK_OP_DISCARD);
    });
    pending_tasks.clear();
  }

private:
  void init_idle_threads() {
    std::lock_guard<std::mutex> locker(task_mutex);
    size_t sz = thread_array.size();
    size_t i;
    for (i = 0; i < sz; ++i) {
      idle_threads.push_back(thread_array.data() + i);
    }
  }

  bool get_pending_task(TaskInfo& task) {
    std::lock_guard<std::mutex> locker(task_mutex);
    if (pending_tasks.empty()) {
      task.func = nullptr;
      task.cb = nullptr;
      return false;
    }
    task = pending_tasks.front();
    pending_tasks.pop_front();
    return true;
  }

  void push_idle_thread(TaskThread *thr) {
    std::lock_guard<std::mutex> locker(task_mutex);
    idle_threads.push_back(thr);
  }

private:
  std::list<TaskInfo> pending_tasks;
  std::list<TaskThread*> idle_threads;
  std::mutex task_mutex;
  std::vector<TaskThread> thread_array;

  friend TaskThread;
};
