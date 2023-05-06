#pragma once
#include <boost/optional.hpp>
#include <boost/asio.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/bind/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "TypeTasks.h"

template<typename MainFn>
class BaseTask {
public:
    BaseTask(boost::asio::io_service& io,
        const uint64_t interval,
        MainFn mainFn) : _io(io),
        _timer(io),
        _mainFn(mainFn),
        _interval(interval),
        _strand(io),
        _cancelled(false) {}

    virtual ~BaseTask() = default;
    virtual void start() = 0;
    void cancel() {
        this->_cancelled.store(true);
        this->_timer.cancel();
    }
    virtual void execute(boost::system::error_code const& e) {}

protected:
    void invokeFn() { this->_mainFn(); }
    void startInvoke() {
        this->_timer.async_wait(this->_strand.wrap(boost::bind(&BaseTask::invokeFn, this)));
    }
    void startWait() {
        this->_timer.async_wait(this->_strand.wrap(boost::bind(&BaseTask::execute, this, boost::asio::placeholders::error)));
    }
    boost::asio::io_service& _io;
    boost::asio::deadline_timer _timer;
    MainFn _mainFn;
    boost::posix_time::milliseconds _interval;
    boost::asio::io_service::strand _strand;
    std::atomic<bool> _cancelled;
};

template<typename MainFn>
class TriggerTask : public BaseTask<MainFn> {
public:
    TriggerTask(boost::asio::io_service& io,
        const uint64_t interval,
        MainFn mainFn) : BaseTask<MainFn>(io, interval, mainFn) {}
    ~TriggerTask() = default;
    void start() override {
        this->_timer.expires_from_now(this->_interval);
        BaseTask<MainFn>::startWait();
    }
    void execute(boost::system::error_code const& e) override {
        if (e != boost::asio::error::operation_aborted) {
            BaseTask<MainFn>::startInvoke();
        }
    }
};

template<typename MainFn>
class PeriodicTask : public BaseTask<MainFn> {
public:
    PeriodicTask(boost::asio::io_service& io,
        const uint64_t interval,
        MainFn mainFn) : BaseTask<MainFn>(io, interval, mainFn) {
        /* Schedule start to be ran by the io_service */
        this->_io.post(boost::bind(&PeriodicTask::start, this));
    }
    void start() override {
        /* Calls handler on startup (i.e. at time 0) and for interrupts. */
        BaseTask<MainFn>::startInvoke();
        this->_timer.expires_from_now(this->_interval);
        BaseTask<MainFn>::startWait();
    }
    ~PeriodicTask() = default;

private:
    void execute(boost::system::error_code const& e) override {
        if (e != boost::asio::error::operation_aborted) {
            if (!this->_cancelled) {
                this->_timer.expires_at(this->_timer.expires_at() + this->_interval);
                BaseTask<MainFn>::startInvoke();
                BaseTask<MainFn>::startWait();
            }
        }
    }
};

template<typename MainFn>
class OnceTask : public BaseTask<MainFn> {
public:
    OnceTask(boost::asio::io_service& io,
        const uint64_t interval,
        MainFn mainFn) : BaseTask<MainFn>(io, interval, mainFn) {
        /* Schedule start to be ran by the io_service */
        this->_io.post(boost::bind(&OnceTask::start, this));
    }
    ~OnceTask() = default;
    void start() override {
        this->_timer.expires_from_now(this->_interval);
        BaseTask<MainFn>::startWait();
    }
    void execute(boost::system::error_code const& e) override {
        if (e != boost::asio::error::operation_aborted) {
            if (!this->_cancelled) {
                this->_timer.expires_at(this->_timer.expires_at() + this->_interval);
                BaseTask<MainFn>::startInvoke();
                BaseTask<MainFn>::startWait();
                BaseTask<MainFn>::cancel();
            }
        }
    }
};
