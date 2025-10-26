#include "stdafx.h"
#include "curl_easy_handler.h"
#include "curl_multi_handler.h"
#include "curl_context.h"
#include "utils.h"
#include "core.h"

#ifndef STRIP_CRASH_HANDLER
#include "../common.shared/crash_report/crash_reporter.h"
#endif // !STRIP_CRASH_HANDLER

namespace
{
    constexpr int TOP_PRIORITY_THREADS_COUNT = 3;
    constexpr int MEDIUM_PRIORITY_THREADS_COUNT = 2;
    constexpr int LOW_PRIORITY_THREADS_COUNT = 1;

    std::map<std::thread::id, CURL*> curl_handlers;
    std::mutex curl_handlers_mutex_;

    CURL* get_for_this_thread()
    {
        const auto thread_id = std::this_thread::get_id();

        std::scoped_lock lock(curl_handlers_mutex_);

        if (const auto iter_handle = curl_handlers.find(thread_id); iter_handle != curl_handlers.end())
            return iter_handle->second;

        CURL* curl_handle = curl_easy_init();

        curl_handlers[thread_id] = curl_handle;

        return curl_handle;
    }
}

namespace core
{
    curl_easy::future_t curl_easy_handler::perform(const std::shared_ptr<curl_context>& _context)
    {
        auto promise = curl_easy::promise_t();
        auto future = promise.get_future();

        add_task(std::make_shared<curl_task>(_context, std::move(promise)), _context->get_priority(), _context->get_id());

        return future;
    }

    void curl_easy_handler::perform_async(const std::shared_ptr<curl_context>& _context, curl_easy::completion_function _completion_func)
    {
        add_task(std::make_shared<curl_task>(_context, std::move(_completion_func)), _context->get_priority(), _context->get_id());
    }

    void curl_easy_handler::raise_task(int64_t _id)
    {
        if (_id == -1)
            return;

        {
            std::lock_guard<std::mutex> guard(top_priority_tasks_mutex_);
            top_priority_tasks_->raise_task(_id);
        }
        {
            std::lock_guard<std::mutex> guard(medium_priority_tasks_mutex_);
            medium_priority_tasks_->raise_task(_id);
        }
        {
            std::lock_guard<std::mutex> guard(low_priority_tasks_mutext_);
            low_priority_tasks_->raise_task(_id);
        }
    }

    void core::curl_easy_handler::init()
    {
        if constexpr (platform::is_windows())
            curl_global_init(CURL_GLOBAL_ALL);
        else
            curl_global_init(CURL_GLOBAL_SSL);
    }

    void core::curl_easy_handler::cleanup()
    {
        stop_ = true;
        {
            std::lock_guard<std::mutex> guard(top_priority_tasks_mutex_);
            top_priority_tasks_.reset();
        }
        {
            std::lock_guard<std::mutex> guard(medium_priority_tasks_mutex_);
            medium_priority_tasks_.reset();
        }
        {
            std::lock_guard<std::mutex> guard(low_priority_tasks_mutext_);
            low_priority_tasks_.reset();
        }

        condition_fetch_.notify_all();
        condition_protocol_.notify_all();

        fetch_thread_.join();
        protocol_thread_.join();

        {
            std::lock_guard<std::mutex> lock(curl_handlers_mutex_);
            for (const auto& h : curl_handlers)
            {
                curl_easy_cleanup(h.second);
            }
        }

        curl_global_cleanup();
    }

    bool core::curl_easy_handler::is_stopped() const
    {
        return stop_.load();
    }

    void curl_easy_handler::add_task(std::shared_ptr<curl_task> _task, priority_t _priority, int64_t _id)
    {
        if (is_stopped())
        {
            _task->cancel();
            g_core->write_string_to_network_log("curl_easy_handler: cancel task in add_task");
            return;
        }

        auto execute = [_task = std::move(_task)] { _task->execute(get_for_this_thread()); };

        if (_priority == core::priority_fetch())
        {
            std::lock_guard<std::mutex> lock(fetch_tasks_mutex_);
            fetch_tasks_.emplace_back(std::move(execute));
            condition_fetch_.notify_one();
        }
        else if (_priority <= core::priority_protocol())
        {
            std::lock_guard<std::mutex> lock(protocol_tasks_mutex_);
            protocol_tasks_.emplace_back(std::move(execute));
            condition_protocol_.notify_one();
        }
        else if (_priority <= core::highest_priority())
        {
            std::lock_guard<std::mutex> guard(top_priority_tasks_mutex_);
            top_priority_tasks_->push_back({ std::move(execute) }, _id);
        }
        else if (_priority <= default_priority())
        {
            std::lock_guard<std::mutex> guard(medium_priority_tasks_mutex_);
            medium_priority_tasks_->push_back({ std::move(execute) }, _id);
        }
        else
        {
            std::lock_guard<std::mutex> guard(low_priority_tasks_mutext_);
            low_priority_tasks_->push_back({ std::move(execute) }, _id);
        }
    }

    void curl_easy_handler::run_task(const task& _task)
    {
        if constexpr (!core::dump::is_crash_handle_enabled() || !platform::is_windows())
            return _task();
#ifndef STRIP_CRASH_HANDLER
#ifdef _WIN32
        __try
        {
            return _task();
        }
        __except (crash_system::reporter::seh_handler(GetExceptionInformation()))
        {
        }
#endif // _WIN32
#else
        return _task();
#endif // !STRIP_CRASH_HANDLER
    }

    curl_easy_handler::curl_easy_handler()
        : stop_(false)
    {
        top_priority_tasks_ = std::make_unique<tools::threadpool>("curl top prio", TOP_PRIORITY_THREADS_COUNT);
        medium_priority_tasks_ = std::make_unique<tools::threadpool>("curl med prio", MEDIUM_PRIORITY_THREADS_COUNT);
        low_priority_tasks_ = std::make_unique<tools::threadpool>("curl low prio", LOW_PRIORITY_THREADS_COUNT);

        fetch_thread_ = std::thread([this]()
        {
            utils::set_this_thread_name("curl easy fetch");
#ifndef STRIP_CRASH_HANDLER
#ifdef _WIN32
            crash_system::reporter::instance().set_thread_exception_handlers();
#endif
#endif // !STRIP_CRASH_HANDLER
            for (;;)
            {
                task nextTask;
                {
                    std::unique_lock<std::mutex> lock(fetch_tasks_mutex_);
                    while (!(stop_ || !fetch_tasks_.empty()))
                        condition_fetch_.wait(lock);

                    if (stop_ && fetch_tasks_.empty())
                        break;

                    nextTask = std::move(fetch_tasks_.front());
                    fetch_tasks_.pop_front();
                }
                if (nextTask)
                    run_task(nextTask);
            }
        });

        protocol_thread_ = std::thread([this]()
        {
            utils::set_this_thread_name("curl easy proto");
#ifndef STRIP_CRASH_HANDLER
#ifdef _WIN32
            crash_system::reporter::instance().set_thread_exception_handlers();
#endif
#endif // !STRIP_CRASH_HANDLER
            for (;;)
            {
                task nextTask;
                {
                    std::unique_lock<std::mutex> lock(protocol_tasks_mutex_);
                    while (!(stop_ || !protocol_tasks_.empty()))
                        condition_protocol_.wait(lock);

                    if (stop_ && protocol_tasks_.empty())
                        break;

                    nextTask = std::move(protocol_tasks_.front());
                    protocol_tasks_.pop_front();
                }
                if (nextTask)
                    run_task(nextTask);
            }
        });
    }
}
