// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include <experimental/resumable>
#include <mutex>
#include <thread>
#include <memory>
#include <atomic>

namespace service_fabric
{
    //
    // Default policy for task/async_awaitable_t
    //
    struct default_awaitable_policy
    {
        //
        // Policy for resuming a coroutine on an async awaitable
        // Default is to resume on wherever the async callback is received
        //
        static void resume(std::experimental::coroutine_handle<> coroutine)
        {
            coroutine.resume();
        }
    };

    //
    // An awaitable wrapping over async callbacks
    // Note that awaitable != coroutine. 
    //
    template <typename T, typename POLICY = default_awaitable_policy>
    class async_awaitable_base
    {
    public:
        async_awaitable_base()
            :_ex(nullptr), _coroutine(nullptr)
        {

        }

        //
        // awaitable contracts 
        //
        bool await_ready() const noexcept
        {
            // let await_suspend make the call and decide whether to suspend
            return false;
        }

        bool await_suspend(std::experimental::coroutine_handle<> awaiter)
        {
            _coroutine = awaiter;

            bool ready = on_start();

            // if we sync completed we'll resume on this thread, otherwise we'll resume on the callback
            // thread
            return !ready;
        }

    public :
        // need to call resume manually because there are cases where resume is not needed within on_start()
        void post_resume()
        {
            // resume on the callback thread by default
            // we should allow posting this to a thread pool
            POLICY::resume(_coroutine);
        }

    protected:
        // start your async operation. 
        // return true if operation started and not yet ready (and thus require suspension). otherwise false.
        virtual bool on_start() = 0;
        virtual void throw_if_error() const { }

    private:
        std::exception_ptr _ex;
        std::experimental::coroutine_handle<> _coroutine;
    };

    template <typename T, typename POLICY = default_awaitable_policy>
    class async_awaitable : public async_awaitable_base<T, POLICY>
    {
    public :
        T await_resume()
        {
            this->throw_if_error();

            return std::move(_ret); 
        }

        void set_result(const T& ret) 
        { 
            _ret = ret; 
        }

        void set_result(T&& ret) 
        { 
            // std::move required to force move semantics since ret has a name despite it is a universal 
            // reference. this is necessary for good performance
            _ret = std::move(ret); 
        }

    private :
        T _ret;
    };

    template <typename POLICY>
    class async_awaitable<void, POLICY> : public async_awaitable_base<void, POLICY>
    {
    public :
        void await_resume()
        {
            return get_result();
        }

        void get_result() const 
        { 
            this->throw_if_error();
        }

        void set_result() { }
    };

    template <typename T>
    class task_promise;

    //
    // Shared value between task and task_promise
    //
    template <typename T>
    class task_value_base
    {
    public:
        task_value_base()
        {
            _ex = nullptr;
            _ready = false;
            _coroutine = nullptr;
        }

        void set_value()
        {
            set_ready();

            resume();
        }

        bool is_ready()
        {
            return _ready;
        }

        void set_exception(std::exception_ptr ex)
        {
            _ex = ex;
            
            set_ready();

            resume();
        }

        void wait()
        {
            std::unique_lock<std::mutex> lock(_mutex);
            while (!_ready)
            {
                _ready_cond.wait(lock);
            }
        }
       
        bool suspend(std::experimental::coroutine_handle<> coroutine)
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if (is_ready()){
                return false;
            }

            // every time when suspend called, we are supplied with the coroutine for this suspention
            // point which knows how to resume to after the co_await
            set_coroutine(coroutine);
            return true;
        }
    protected :
        void throw_if_error()
        {
            // we've captured an exception in this coroutine
            if (_ex != nullptr)
                std::rethrow_exception(_ex);
        }

    private :
        void set_coroutine(std::experimental::coroutine_handle<> coroutine)
        {
            _coroutine = coroutine;
        }

        void set_ready()
        {
            std::lock_guard<std::mutex> lock(_mutex);

            _ready = true;
            _ready_cond.notify_all();
        }
 
        void resume()
        {
            // resume on the coroutine we remembered when we suspend, and it points to the proper suspention
            // point after the await. 
            // we also need to clear the member variable so that we don't incorrectly resume on the wrong 
            // coroutine
            auto coroutine = _coroutine;
            if (coroutine != decltype(coroutine)())
            {
                _coroutine = nullptr;
                coroutine();
            }
        }

    private :
        std::mutex _mutex;
        std::condition_variable _ready_cond;
        std::atomic<bool> _ready;
        std::exception_ptr  _ex;
        std::experimental::coroutine_handle<> _coroutine;
    };

    template <typename T>
    class task_value : public task_value_base<T>
    {
    public :
        T &&get_value()
        {
            this->throw_if_error();

            return std::move(_value);
        }

        template <typename VALUE, typename = std::enable_if_t<std::is_convertible<VALUE&&, T>::value>>
        void set_value(VALUE&& value)
        {
            _value = std::move(value);
            task_value_base<T>::set_value();
        }
        
        void set_value(const T& value)
        {
            _value = value;
            task_value_base<T>::set_value();
        }

    private :
        T _value;
    };

    // specialization for 
    template <>
    class task_value<void> : public task_value_base<void>
    {
    public :
        void get_value()
        {
            throw_if_error();
        }
    };


    //
    // Coroutine type - for functions that needs to co_await and establish a coroutine frame/context
    // This is the return object and our version of future
    //
    // task_t<return_type> func() 
    // {
    //      co_await ...
    //      co_return ...
    // }
    //
    // This coroutine type itself is also awaitable. It shares the same underlying value with task_promise_t
    // so that you can construct a task_t with task_promise_t (that is equivalent with any other task_t)
    //
    template <typename T>
    class task
    {
    public:
        using value_type = T;
        using promise_type = task_promise<T>;
        using coroutine_type = std::experimental::coroutine_handle<>;

    public:
        task(std::shared_ptr<task_value<T>> value)
            :_value(value)
        {
        }

    public:

        bool await_ready()
        {
            return is_ready();
        }

        bool await_suspend(coroutine_type coroutine)
        {
            return _value->suspend(coroutine);
        }

        auto await_resume()
        {
            return _value->get_value();
        }

    public:
        auto get_value() const
        {
            if (!is_ready())
                wait();
            return _value->get_value();
        }

        bool is_ready() const
        {
            return _value->is_ready();
        }

        void wait() const
        {
            _value->wait();
        }

    private:
        std::shared_ptr<task_value<T>> _value;
    };


    //
    // Promise type used by task
    // It owns the value and can construct a task_t<T> from the value shared using shared_ptr
    //
    template <typename T>
    class task_promise_base
    {
    public:
        template <typename ...Args>
        task_promise_base(Args&& ...args)
            :_value(std::make_shared<task_value<T>>(std::forward<Args>(args)...))
        {}

        task_promise_base(const task_promise_base&) = delete;
        task_promise_base(task_promise_base&&) = default;

    public:
        //
        // Promise functions for compiler
        //

        // don't initial suspend - we don't have a way to get back
        auto initial_suspend() noexcept { return std::experimental::suspend_never{}; }

        // don't final suspend - we don't have a way to resume nor destroy the coroutine either
        auto final_suspend() noexcept { return std::experimental::suspend_never{}; }

        void set_exception(std::exception_ptr ex)
        {
            _value->set_exception(ex);
        }

    public:
        auto get_value() const
        {
            return _value->get_value();
        }

    protected:
        std::shared_ptr<task_value<T>> _value;
    };

    template<typename T>
    class task_promise : public task_promise_base<T>
    {
    public :
        // set the return value of the coroutine
        template<
            typename VALUE,
            typename = std::enable_if_t<std::is_convertible<VALUE&&, T>::value>>
        void return_value(VALUE&& value)
            noexcept(std::is_nothrow_constructible<T, VALUE&&>::value)
        {
            this->_value->set_value(std::move(value));
        }

        // construct the return object (task_t)
        auto get_return_object()
        {
            return task<T>(this->_value);
        }
    };

    template<>
    class task_promise<void> : public task_promise_base<void>
    {
    public :
        // set the return value of the coroutine
        void return_void() noexcept
        {
            _value->set_value();
        }

        // construct the return object (task_t)
        auto get_return_object()
        {
            return task<void>(this->_value);
        }
    };
}
