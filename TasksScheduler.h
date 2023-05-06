#pragma once
#include "Tasks.h"
#include <boost/noncopyable.hpp>

template<typename Key, typename MainFn>
class TasksScheduler : boost::noncopyable {
public:
    TasksScheduler() : _work{ this->_io } {}
    ~TasksScheduler() { stop(); }
    void pollOne() { this->_io.poll_one(); }
    void addTask(Key const& classKey,
        const uint64_t interval,
        ME9_TypeTask const& type,
        MainFn const& mainFn) {
        boost::mutex::scoped_lock guard(this->_mutex);
        this->_tasks.try_emplace(std::move(classKey), createSharedPtrTask(interval, type, mainFn));
    }
    void stop() {
        this->_io.post([this]() { this->_work.reset(); /* let io_service run out of work */ });
        this->_io.stop();
    }
    void interrupt(Key const& key) {
        boost::mutex::scoped_lock guard(this->_mutex);
        const auto it = this->_tasks.find(key);
        if (it != this->_tasks.end()) {
            it->second->start();
        }
    }
    void removeTask(Key const& key) {
        boost::mutex::scoped_lock guard(this->_mutex);
        const auto it = this->_tasks.find(key);
        if (it != this->_tasks.end()) {
            it->second->cancel();
            this->_tasks.erase(it);
        }
    }

private:
    inline std::shared_ptr<BaseTask<MainFn>> createSharedPtrTask(const uint64_t interval,
        ME9_TypeTask const& type,
        MainFn const& mainFn) {
        TaskFactory factory{ std::ref(this->_io), interval, mainFn };
        return factory(std::move(type));
    }
    struct TaskFactory {
        boost::asio::io_service& _io;
        const uint64_t _interval;
        MainFn _mainFn;
        std::shared_ptr<BaseTask<MainFn>> operator()(ME9_TypeTask const& type) const {
            switch (type) {
            case ME9_TypeTask::PeriodicTaskType:
                return std::make_shared<PeriodicTask<MainFn>>(std::ref(this->_io), this->_interval, this->_mainFn);
            case ME9_TypeTask::TriggerTaskType:
                return std::make_shared<TriggerTask<MainFn>>(std::ref(this->_io), this->_interval, this->_mainFn);
            default:
                return std::make_shared<OnceTask<MainFn>>(std::ref(this->_io), this->_interval, this->_mainFn);
            }
        }
    };
    boost::asio::io_service _io;
    std::unordered_map<Key, std::shared_ptr<BaseTask<MainFn>>> _tasks;
    boost::optional<boost::asio::io_service::work> _work;
    mutable boost::mutex _mutex;
};

