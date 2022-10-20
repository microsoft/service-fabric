// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include <vector>
#include <map>
#include <exception>
#include <string>
#include <map>
#include <memory>
#include <mutex>
#include <list>
#include <thread>
#include <type_traits>
#include "libpal.h"
#include "servicefabric.h"
#include "mock.h"
#include "task.h"
#include "binding.h"

#include "reliable_exception.h"

#include "external/SfStatus.h"

using namespace std;
using namespace std::chrono;

namespace service_fabric
{
    // a tick is a 100 nano-second
    // typedef ratio<1, 1000000000> nano;
    typedef ratio<1, 10000000> tick;

    using ticks = duration<int64_t, tick>;

    //
    // Immutable const buffer
    //
    class const_slice
    {
    public :
        const_slice()
        {
            _buf = nullptr; _size = 0;
        }

        const_slice(const char *buf, size_t size)
            :_buf(buf), _size(size)
        {}

        const char *data() const { return _buf; }
        size_t size() const { return _size; }

        bool is_null() const { return _buf == nullptr; }

        const_slice &operator =(const const_slice &other) = default;

        void clear() 
        { 
            _buf = nullptr; _size = 0; 
        }
    private :
        const char *_buf;
        size_t _size;
    };

    //
    // Immutable buffer
    //
    class slice
    {
    public :
        slice(char *buf, size_t size)
            :_buf(buf), _size(size)
        {}

        char *data() const { return _buf; }
        size_t size() const { return _size; }

        slice &operator =(const slice &other)
        {
            _buf = other._buf;
            _size = other._size;

            return *this;        
        }

    private :
        char *_buf;
        size_t _size;
    };

    class reliable_context;

    //
    // simplified unique_ptr with pointer type as type argument
    //
    template <typename HANDLE, typename RELEASE_HELPER>
    class native_wrapper
    {
    public :
        native_wrapper(native_wrapper &&other)
            :native_wrapper(other.detach())
        {
        }

        native_wrapper(HANDLE hnd)
        {
            _handle = hnd;
        }

        native_wrapper()
        {
            _handle = nullptr;
        }

        ~native_wrapper()
        {
            reset();
        }

        native_wrapper &operator=(native_wrapper &&right) noexcept
        {
            attach(right.detach());
            return *this;
        }

        bool operator==(const native_wrapper &right) const noexcept 
        {
            return _handle == right._handle;
        }

        native_wrapper(const native_wrapper &other) = delete;
        HANDLE operator=(const native_wrapper &right) = delete;

        HANDLE detach()
        {
            HANDLE hnd = _handle;
            _handle = nullptr;

            return hnd;
        }

        void attach(HANDLE hnd)
        {
            if (hnd != _handle)
            {
                reset();
                _handle = hnd;
            }
        }

        void reset()
        {
            if (_handle)
            {
                RELEASE_HELPER::release(_handle);
            }

            _handle = nullptr;
        }

        HANDLE native_handle() const { return _handle; }

        explicit operator bool() const { return (_handle != nullptr); }

        bool is_null() const { return _handle == nullptr; }

    private :
        HANDLE _handle;
    };

    //
    // awaitable policy for service fabric async operations
    // To override this for the entire service fabric async awaitable operations, call set_resume_func
    //
    struct ktl_awaitable_policy
    {
        static void resume(std::experimental::coroutine_handle<> coroutine)
        {
            resume_func _resume_func = get_resume_func();
            if (_resume_func){
                _resume_func(coroutine);
            }
            else{
                coroutine.resume();
            }
        }

        using resume_func = std::function<void(std::experimental::coroutine_handle<>)>;

        static void set_resume_func(resume_func func)
        {
            get_resume_func() = func;
        }

    private :
        static resume_func &get_resume_func() 
        { 
            // Use local static to support embedding static in a header file
            static resume_func s_resume_func;
            return s_resume_func; 
        }
    };

    //
    // Common awaitable implementation for KTL based async thread callback-based C APIs
    // By default we'll resume on KTL threads but you can customize that to post to your own thread pool
    // if needed (such as to boost::asio::io_service worker threads)
    //
    template <typename T>
    class ktl_awaitable : public async_awaitable<T, ktl_awaitable_policy>
    {
    public :
        ktl_awaitable()
            :_status(S_OK)
        {}

    protected :
        void set_error(HRESULT status, string &&msg)
        {
            _status = status;
            _msg = msg;
        }

        virtual void throw_if_error()
        {
            if (!SUCCEEDED(_status))
            {
                reliable_exception::throw_error(_status, _msg.c_str());
            }
        }

        HRESULT get_error()
        {
            return _status;
        }

    private :
        HRESULT _status;
        std::string _msg;
    };

    struct reliable_transaction_release_helper
    {
        static void release(TransactionHandle hnd)
        {
            binding::get()._pfnTransaction_Dispose(hnd);
            binding::get()._pfnTransaction_Release(hnd);
        }
    };

    class reliable_transaction : public native_wrapper<TransactionHandle, reliable_transaction_release_helper>
    {
    public:
        using base_type = native_wrapper<TransactionHandle, reliable_transaction_release_helper>;

        reliable_transaction(reliable_transaction &&txn)
            :base_type(std::move(txn))
        {
            _processed = txn._processed;
        }

        reliable_transaction(TransactionHandle handle)
            : base_type(handle), _processed(false)
        {
        }

        ~reliable_transaction()
        {
            // automatically abort transaction if not yet committed
            if (!is_null())
            {
                if (!is_processed())
                    abort();
            }

            // releasing transaction will happen in base dtor
        }


    public:
        // true if the transaction has been either committed or aborted - dtor will do nothing
        // false otherwise - we'll abort the transaction in dtor.
        bool is_processed() { return _processed; }

        auto commit()
        {
            class reliable_transaction_commit_operation : public ktl_awaitable<void>
            {
            public:
                reliable_transaction_commit_operation(reliable_transaction *txn)
                    :_txn(txn)
                {

                }

            protected:
                virtual bool on_start()
                {
                    BOOL sync_completed;
                    HRESULT status = binding::get()._pfnTransaction_CommitAsync(
                        _txn->native_handle(),
                        [](void *context, HRESULT status) {
                            auto op = reinterpret_cast<reliable_transaction_commit_operation *>(context);
                            if (SUCCEEDED(status))
                            {
                                op->set_result();
                                op->set_committed();
                            }
                            else
                            {
                                op->set_error(status, std::string("Transaction_Commit"));
                            }

                            op->post_resume();
                        },
                        this,
                        &sync_completed);
                    if (FAILED(status))
                    {
                        set_error(status, std::string("Transaction_Commit"));
                        return true;
                    }

                    if (sync_completed)
                    {
                        set_result();
                        set_committed();
                        return true;
                    }

                    return false;
                }

                void set_committed()
                {
                    _txn->set_processed();
                }

            private:
                reliable_transaction *_txn;
            };

            return reliable_transaction_commit_operation(this);
        }

        void abort()
        {
            HRESULT status = binding::get()._pfnTransaction_Abort(native_handle());
            if (FAILED(status))
            {
                reliable_exception::throw_error(status, "Transaction_Abort");
                return;
            }

            set_processed();
        }   

    private :
        void set_processed() { _processed = true; }

    private :
        // true if the transaction has been either committed or aborted - dtor will do nothing
        // false otherwise - we'll abort the transaction in dtor.
        bool _processed;
    };

    //
    // Default serialization implementation for key 
    //
    template <typename TYPE, typename ENABLE = void>
    struct reliable_key_serializer_traits
    {
        static void serialize(const TYPE &obj, u16string &str)
        {
            throw nullptr; 
        }

        static TYPE deserialize(const char16_t *chars)
        {
            throw nullptr;    
        }
    };

    //
    // Default serialization implementation for u16string 
    //
    template<>
    struct reliable_key_serializer_traits<u16string>
    {
        static void serialize(const u16string &obj, u16string &str)
        {
            str = obj;
        }

        static u16string deserialize(const char16_t *chars)
        {
            return u16string(chars);
        }
    };

    //
    // Default serialization implementation for u16string 
    //
    template<>
    struct reliable_key_serializer_traits<const char16_t *>
    {
        static void serialize(const char16_t *obj, u16string &str)
        {
            // this is a bit unfortunate but for all other cases we don't necessarily control the lifetime
            // of the returned u16string - so making a copy is a safe bet for now
            str = obj;
        }

        static const char16_t *deserialize(const char16_t *chars)
        {
            return chars;
        }
    };

    // 
    // @TODO - Key serialization for string
    // Unfortunately std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> is broken in VS 2017
    // 

    //
    // Default serialization implementation that throws
    //
    template <typename TYPE, typename ENABLE = void>
    struct reliable_value_serializer_traits
    {
        // value_holder is there to ensure lifetime of the serialization is kept alive for the duration of 
        // the async call. reliable_map doesn't care about what's in there
        using value_holder = TYPE;

        //
        // this returns a char buffer that is alive as long as holder is alive
        // if there are no additional lifetime semantics required, holder can be the same as TYPE itself
        // otherwise, save it into your holder
        // reliable_map use the returned value, not the holder
        //
        static const_slice serialize(const TYPE &obj, value_holder &holder)
        {
            UNREFERENCED_PARAMETER(obj);
            UNREFERENCED_PARAMETER(holder);
            throw nullptr;    
        }

        static void deserialize(TYPE &obj, const void *bytes, size_t len)
        {
            UNREFERENCED_PARAMETER(obj);
            UNREFERENCED_PARAMETER(bytes);
            UNREFERENCED_PARAMETER(len);
            throw nullptr;
        }
    };

    //
    // Default serializer for integers/floats on the stack - blittable 
    //
    template <typename TYPE>
    struct reliable_value_serializer_traits<TYPE, typename std::enable_if<(std::is_integral<TYPE>::value || std::is_floating_point<TYPE>::value)>::type>
    {
        using value_holder = TYPE;

        static const_slice serialize(const TYPE &obj, TYPE &holder)
        {
            // we assume obj is going to be on the stack
            return { (const char *)&obj, sizeof(TYPE) };
        }

        static void deserialize(TYPE &obj, const void *bytes, size_t len)
        {
            memcpy(&obj, bytes, len);
        }
    };

    //
    // Default serializer for string 
    //
    template <> 
    struct reliable_value_serializer_traits<string>
    {
        using value_holder = string;

        static const_slice serialize(const string &obj, string &holder)
        {
            UNREFERENCED_PARAMETER(holder);
            return { obj.c_str(), obj.length() };
        }

        static void deserialize(string &obj, const void *bytes, size_t len)
        {
            obj.assign((const char *)bytes, len);
        }
    };

    //
    // Default serializer for vector<char> 
    //
    template <> 
    struct reliable_value_serializer_traits<vector<char>>
    {
        using value_holder = const char *;

        static const_slice serialize(const vector<char> &obj, value_holder &holder)
        {
            UNREFERENCED_PARAMETER(holder);
            return { obj.data(), obj.size() };
        }

        static void deserialize(vector<char> &obj, const void *bytes, size_t len)
        {
            // @PERF - If we can guarantee the lifetime of returned buffer, we should be able to pass 
            // pointer directly 
            obj.assign((const char *)bytes, (const char *)bytes + len);
        }
    };

    template <typename KEY>
    struct reliable_map_key_enumerator
    {
        using key_serializer = reliable_key_serializer_traits<KEY>;

        enum enumerator_status
        {
            first,
            in_progress,
            end
        };

        // we only need it for coroutines
        reliable_map_key_enumerator()
            :reliable_map_key_enumerator(nullptr)
        {

        }

        reliable_map_key_enumerator(StoreKeyEnumeratorHandle handle)
        {
            _handle = handle;
            _status = enumerator_status::first;
        }

        reliable_map_key_enumerator(const reliable_map_key_enumerator &) = delete;
        reliable_map_key_enumerator(reliable_map_key_enumerator &&other)
            :reliable_map_key_enumerator(other._handle)
        {
            other._handle = nullptr;
        }

        reliable_map_key_enumerator &operator =(reliable_map_key_enumerator &&other)
        {
            _handle = other._handle;
            _status = enumerator_status::first;
            _key.clear();

            other._handle = nullptr;

            return *this;
        }

        reliable_map_key_enumerator &operator =(const reliable_map_key_enumerator &) = delete;

        ~reliable_map_key_enumerator()
        {
            if (_handle != nullptr)
            {
                binding::get()._pfnStoreKeyEnumerator_Release(_handle);
                _handle = nullptr;
            }
        }

        bool move_next()
        {
            if (_status == enumerator_status::end)
                return false;

            const char16_t *key = nullptr;
            BOOL advanced;

            HRESULT status = binding::get()._pfnStoreKeyEnumerator_MoveNext(_handle, &advanced, &key);
            if (FAILED(status))
                reliable_exception::throw_error(status, "StateProviderEnumerator::MoveNext");

            if (advanced)
            {
                _key = key;
                _status = enumerator_status::in_progress;

                return true;
            }
            else
            {
                _status = enumerator_status::end;

                return false;
            }
        }

        KEY get_key()
        {
            validate();

            return key_serializer::deserialize(_key.c_str());
        }

    private:
        void validate()
        {
            if (_handle == nullptr ||                           // moved
                _status != enumerator_status::in_progress)      // no value available yet or past end
                throw out_of_range("reliable_map_key_enumerator");
        }

    private:
        enumerator_status           _status;
        StoreKeyEnumeratorHandle _handle;
        u16string                   _key;
    };

    struct reliable_map_release_helper
    {
        static void release(StateProviderHandle hnd)
        {
            binding::get()._pfnStore_Release(hnd);
        }
    };
    
    template <typename KEY, typename VALUE>
    class reliable_map_holder;

    struct buffer_release_helper
    {
        static void release(BufferHandle handle)
        {
            if (handle != NULL)
                binding::get()._pfnBuffer_Release(handle);
        }
    };

    typedef native_wrapper<BufferHandle, buffer_release_helper> buffer_holder;

    template <typename KEY, typename VALUE>
    class reliable_map : public native_wrapper<StateProviderHandle, reliable_map_release_helper>
    {
    public :
        using base_type = native_wrapper<StateProviderHandle, reliable_map_release_helper>;
        using key_serializer = reliable_key_serializer_traits<KEY>;
        using value_serializer = reliable_value_serializer_traits<VALUE>;

        reliable_map(StateProviderHandle hnd)
            :base_type(hnd)
        {}

        reliable_map()
            :base_type()
        {}

        reliable_map(reliable_map &&other)
            :base_type(std::move(other))
        {

        }

        reliable_map &operator =(reliable_map &&other)
        {
            base_type::operator =(std::move(other));
            return *this;
        }

        bool operator ==(const reliable_map &other) const
        {
            return base_type::operator ==(other);
        }

    public :
        using get_completion_callback = std::function<void(const_slice, bool, int64_t)>;

    private :
        class reliable_map_get_operation : public ktl_awaitable<bool>
        {
        public:
            reliable_map_get_operation(reliable_transaction &txn, StateProviderHandle handle, const KEY *key_ptr, const_slice key, ticks timeout, Store_LockMode lockmode, get_completion_callback callback)
                :_handle(handle), _txn(&txn), _key(key), _key_ptr(key_ptr), _timeout(timeout), _lockmode(lockmode), _callback(callback)
            {
                _cts = NULL;
            }

        protected:
            virtual bool on_start()
            {
                BOOL sync_completed;
                BOOL exists;
                int64_t version;
                size_t cookie;

                // If pass a real key (instead of const_slice)
                if (_key_ptr)
                {                        
                    key_serializer::serialize(*_key_ptr, _key_str);

                    // Update const_slice as needed. Size is not needed today
                    _key = const_slice((const char *)_key_str.c_str(), 0);
                }

                Buffer buffer;
                HRESULT status = binding::get()._pfnStore_ConditionalGetAsync(
                    _handle,
                    _txn->native_handle(),
                    (const char16_t *)_key.data(),
                    _timeout.count(),
                    _lockmode,
                    &cookie,        // pCookie
                    &buffer,        // native buffer
                    &version,       // version
                    NULL,           // CancellationTokenSourceHandle
                    &exists,
                    [](void *context, HRESULT status, BOOL exists, size_t cookie, void *bytes, uint32_t bytes_size, int64_t version) {
                        UNREFERENCED_PARAMETER(cookie);
                        UNREFERENCED_PARAMETER(version);
                        auto op = reinterpret_cast<reliable_map_get_operation *>(context);
                        if (SUCCEEDED(status))
                        {
                            op->set_result(exists);

                            op->_callback(const_slice((const char *)bytes, bytes_size), exists, version);
                        }
                        else
                        {
                            op->set_error(status, std::string("Store_ConditionalGetAsync"));
                        }

                        op->post_resume();
                    },
                    this,
                    &sync_completed);
                if (FAILED(status))
                {
                    this->set_error(status, std::string("Store_ConditionalGetAsync"));
                    return true;
                }

                if (sync_completed)
                {
                    this->set_result(exists);                   

                    // make sure buffer.handle is always released
                    if (exists)
                    {
                        buffer_holder holder(buffer.Handle);
                        _callback(const_slice(buffer.Bytes, buffer.Length), exists, version);
                    }
                    else
                    {
                        _callback(const_slice(), exists, version);
                    }

                    return true;
                }

                return false;
            }

        private:
            reliable_transaction *_txn;
            StateProviderHandle _handle;
            ticks _timeout;
            const_slice _key;
            const KEY *_key_ptr;
            u16string _key_str;
            CancellationTokenSourceHandle _cts;
            bool *_exists;
            Store_LockMode _lockmode;
            get_completion_callback _callback;
        };

    public :
        // Returning value through coroutine is quite expensive even if you optimize using move semantics
        // Stick to copying to pass by reference for now
        auto get(reliable_transaction &txn, const KEY &key, ticks timeout, VALUE *value = nullptr)
        {
            return reliable_map_get_operation(txn, native_handle(), &key, const_slice(), timeout, Store_LockMode::Store_LockMode_Free, [value](const_slice bytes, BOOL exists, int64_t version) -> void {
                UNREFERENCED_PARAMETER(version);
                if (exists)
                {
                    value_serializer::deserialize(*value, bytes.data(), bytes.size());
                }
            });
        }

        auto get(reliable_transaction &txn, const KEY &key, ticks timeout, std::pair<int64_t, VALUE> *version_value = nullptr, Store_LockMode lockmode = Store_LockMode::Store_LockMode_Free)
        {
            return reliable_map_get_operation(txn, native_handle(), &key, const_slice(), timeout, lockmode, [version_value](const_slice bytes, BOOL exists, int64_t version) -> void {
                if (exists)
                {
                    version_value->first = version;
                    value_serializer::deserialize(version_value->second, bytes.data(), bytes.size());
                }
            });
        }
        
        // Version that take raw key buffer gives you the value buffer directly. Great if you need best perfgormance 
        auto get_bytes(reliable_transaction &txn, const_slice key, ticks timeout, get_completion_callback completed)
        {
            return reliable_map_get_operation(txn, native_handle(), nullptr, key, timeout, Store_LockMode::Store_LockMode_Free, completed);
        }

        // Version that take key and gives you the value buffer directly. Best for big buffer
        auto get_bytes(reliable_transaction &txn, const KEY &key, ticks timeout, get_completion_callback completed)
        {
            return reliable_map_get_operation(txn, native_handle(), &key, {}, timeout, Store_LockMode::Store_LockMode_Free, completed);
        }

        auto update(reliable_transaction &txn, const KEY &key, const VALUE &value, ticks timeout)
        {
            class reliable_map_update_operation : public ktl_awaitable<bool>
            {
            public:
                reliable_map_update_operation(reliable_transaction &txn, StateProviderHandle handle, const KEY &key, const VALUE &value, ticks timeout)
                    :_handle(handle), _txn(&txn), _key(key), _value(value), _timeout(timeout)
                {
                    _cts = NULL;
                }

            protected:
                virtual bool on_start()
                {
                    BOOL sync_completed;
                    BOOL updated;

                    auto slice = value_serializer::serialize(_value, _value_holder);
                    key_serializer::serialize(_key, _key_str);
                    
                    HRESULT status = binding::get()._pfnStore_ConditionalUpdateAsync(
                        _handle,
                        _txn->native_handle(),
                        _key_str.c_str(),
                        NULL,           // cookie
                        (void *)slice.data(),
                        (uint32_t) slice.size(),
                        _timeout.count(),                        
                        NULL,           // CancellationTokenSourceHandle 
                        -1,             // version
                        &updated,
                        [](void *context, HRESULT status, BOOL updated) 
                        {
                            auto op = reinterpret_cast<reliable_map_update_operation *>(context);
                            if (SUCCEEDED(status))
                            {
                                op->set_result(updated);
                            }
                            else
                            {
                                op->set_error(status, std::string("Store_ConditionalUpdateAsync"));
                            }

                            op->post_resume();
                        },
                        this,
                        &sync_completed);
                    if (FAILED(status))
                    {
                        this->set_error(status, std::string("Store_ConditionalUpdateAsync"));
                        return true;
                    }

                    if (sync_completed)
                    {
                        this->set_result(updated);

                        return true;
                    }

                    return false;
                }

            private:
                reliable_transaction *_txn;
                StateProviderHandle _handle;
                ticks _timeout;
                const KEY &_key;
                const VALUE &_value;
                u16string _key_str;
                typename value_serializer::value_holder _value_holder;       // see reliable_map_value_serializer_traits
                CancellationTokenSourceHandle _cts;
            };

            return reliable_map_update_operation(txn, native_handle(), key, value, timeout);
        }

        auto update(reliable_transaction &txn, const KEY &key, const pair<int64_t, VALUE> &version_value, ticks timeout)
        {
            class reliable_map_update_operation : public ktl_awaitable<bool>
            {
            public:
                reliable_map_update_operation(reliable_transaction &txn, StateProviderHandle handle, const KEY &key, const pair<int64_t, VALUE> &version_value, ticks timeout)
                    :_handle(handle), _txn(&txn), _key(key), _version_value(version_value), _timeout(timeout)
                {
                    _cts = NULL;
                }

            protected:
                virtual bool on_start()
                {
                    BOOL sync_completed;
                    BOOL updated;

                    auto slice = value_serializer::serialize(_version_value.second, _value_holder);
                    key_serializer::serialize(_key, _key_str);

                    HRESULT status = binding::get()._pfnStore_ConditionalUpdateAsync(
                        _handle,
                        _txn->native_handle(),
                        _key_str.c_str(),
                        NULL,           // cookie
                        (void *)slice.data(),
                        (uint32_t)slice.size(),
                        _timeout.count(),
                        NULL,           // CancellationTokenSourceHandle 
                        _version_value.first,             // version
                        &updated,
                        [](void *context, HRESULT status, BOOL updated)
                    {
                        auto op = reinterpret_cast<reliable_map_update_operation *>(context);
                        if (SUCCEEDED(status))
                        {
                            op->set_result(updated);
                        }
                        else
                        {
                            op->set_error(status, std::string("Store_ConditionalUpdateAsync"));
                        }

                        op->post_resume();
                    },
                        this,
                        &sync_completed);
                    if (FAILED(status))
                    {
                        this->set_error(status, std::string("Store_ConditionalUpdateAsync"));
                        return true;
                    }

                    if (sync_completed)
                    {
                        this->set_result(updated);

                        return true;
                    }

                    return false;
                }

            private:
                reliable_transaction * _txn;
                StateProviderHandle _handle;
                ticks _timeout;
                const KEY &_key;
                const pair<int64_t, VALUE> &_version_value;
                u16string _key_str;
                typename value_serializer::value_holder _value_holder;       // see reliable_map_value_serializer_traits
                CancellationTokenSourceHandle _cts;
            };

            return reliable_map_update_operation(txn, native_handle(), key, version_value, timeout);
        }



        auto remove(reliable_transaction &txn, const KEY &key, ticks timeout)
        {
            class reliable_map_remove_operation : public ktl_awaitable<bool>
            {
            public:
                reliable_map_remove_operation(reliable_transaction &txn, StateProviderHandle handle, const KEY &key, ticks timeout)
                    :_handle(handle), _txn(&txn), _key(key), _timeout(timeout)
                {
                    _cts = NULL;
                }

            protected:
                virtual bool on_start()
                {
                    BOOL sync_completed;
                    BOOL removed;

                    key_serializer::serialize(_key, _key_str);

                    HRESULT status = binding::get()._pfnStore_ConditionalRemoveAsync(
                        _handle,
                        _txn->native_handle(),
                        _key_str.c_str(),
                        _timeout.count(),
                        NULL,           // CancellationTokenSource
                        -1,             // version
                        &removed,
                        [](void *context, HRESULT status, BOOL removed)
                        {
                            auto op = reinterpret_cast<reliable_map_remove_operation *>(context);
                            if (SUCCEEDED(status))
                            {
                                op->set_result(removed);
                            }
                            else
                            {
                                op->set_error(status, std::string("Store_ConditionalRemoveAsync"));
                            }

                            op->post_resume();
                        },
                        this,
                        &sync_completed);
                    if (FAILED(status))
                    {
                        this->set_error(status, std::string("Store_ConditionalRemoveAsync"));
                        return true;
                    }

                    if (sync_completed)
                    {
                        this->set_result(removed);

                        return true;
                    }

                    return false;
                }

            private:
                reliable_transaction *_txn;
                StateProviderHandle _handle;
                ticks _timeout;
                const KEY &_key;
                u16string _key_str;
                CancellationTokenSourceHandle _cts;
            };

            return reliable_map_remove_operation(txn, native_handle(), key, timeout);
        }

        auto add(reliable_transaction &txn, const KEY &key, const VALUE &value, ticks timeout)
        {
            class reliable_map_add_operation : public ktl_awaitable<void>
            {
            public:
                reliable_map_add_operation(reliable_transaction &txn, StateProviderHandle handle, const KEY &key, const VALUE &value, ticks timeout)
                    :_handle(handle), _txn(&txn), _key(key), _timeout(timeout), _value(value)
                {
                    _cts = NULL;
                }

            protected:
                virtual bool on_start()
                {
                    BOOL sync_completed;
                    
                    auto slice = value_serializer::serialize(_value, _value_holder);
                    key_serializer::serialize(_key, _key_str);

                    HRESULT status = binding::get()._pfnStore_AddAsync(
                        _handle,
                        _txn->native_handle(),
                        _key_str.c_str(),
                        0,           // pCookie
                        (void *)slice.data(),
                        (uint32_t) slice.size(),
                        _timeout.count(),
                        NULL,           // CancellationTokenSourceHandle
                        [](void *context, HRESULT status) {
                            auto op = reinterpret_cast<reliable_map_add_operation *>(context);
                            if (SUCCEEDED(status))
                            {
                                op->set_result();
                            }
                            else
                            {
                                op->set_error(status, std::string("Store_AddAsync"));
                            }

                            op->post_resume();
                        },
                        this,
                        &sync_completed);
                    if (FAILED(status))
                    {
                        set_error(status, std::string("Store_AddAsync"));
                        return true;
                    }

                    if (sync_completed)
                    {
                        set_result();

                        return true;
                    }

                    return false;
                }

            private:
                reliable_transaction *_txn;
                StateProviderHandle _handle;
                ticks _timeout;
                const KEY &_key;
                const VALUE &_value;
                u16string _key_str;
                typename value_serializer::value_holder _value_holder;
                CancellationTokenSourceHandle _cts;
            };

            return reliable_map_add_operation(txn, native_handle(), key, value, timeout);
        }

        auto get_keys(reliable_transaction &txn, const char16_t *first_key = nullptr, const char16_t *last_key = nullptr)
        {
            class reliable_map_get_keys_operation : public ktl_awaitable<reliable_map_key_enumerator<KEY>>
            {
            public:
                reliable_map_get_keys_operation(reliable_transaction &txn, StateProviderHandle handle, const char16_t *first_key, const char16_t *last_key)
                    :_handle(handle), _txn(&txn), _first_key(first_key), _last_key(last_key)
                {

                }

            protected:
                virtual bool on_start()
                {
                    BOOL sync_completed;

                    StoreKeyEnumeratorHandle handle;
                    HRESULT status = binding::get()._pfnStore_CreateKeyEnumeratorAsync(
                        _handle,
                        _txn->native_handle(),
                        _first_key, 
                        _last_key,
                        &handle,
                        [](void *context, HRESULT status, StoreKeyEnumeratorHandle handle)
                        {
                            auto op = reinterpret_cast<reliable_map_get_keys_operation *>(context);
                            if (SUCCEEDED(status))
                            {
                                op->set_result(reliable_map_key_enumerator<KEY>(handle));
                            }
                            else
                            {
                                op->set_error(status, std::string("Store_CreateKeysEnumeratorAsync"));
                            }

                            op->post_resume();
                        },
                        this,
                        &sync_completed);

                    if (FAILED(status))
                    {
                        this->set_error(status, std::string("Store_CreateKeysEnumeratorAsync"));
                        return true;
                    }

                    if (sync_completed)
                    {
                        this->set_result(reliable_map_key_enumerator<KEY>(handle));

                        return true;
                    }

                    return false;
                }

            private:
                reliable_transaction *_txn;
                StateProviderHandle _handle;
                const char16_t *_first_key;
                const char16_t *_last_key;
            };
            return reliable_map_get_keys_operation(txn, native_handle(), first_key, last_key);
        }

        auto bind(reliable_transaction &txn, ticks timeout)
        {
            return reliable_map_holder<KEY, VALUE>(*this, txn, timeout);
        }
    };

    // reliable_map_holder that is bound to a particular transaction and time out, simplifing the code
    template <typename KEY, typename VALUE>
    class reliable_map_holder
    {
    public :
        reliable_map_holder(reliable_map<KEY, VALUE> &map, reliable_transaction &txn, ticks timeout)
            :_map(map), _txn(txn), _timeout(timeout)
        {

        }

        task<VALUE> get(const KEY &key, bool *exists = nullptr)
        {
            VALUE value;
            bool has_value = co_await _map.get(_txn, key, _timeout, &value);
            if (exists)
                *exists = has_value;

            co_return value;
        }

        // To avoid ambiguity, value doesn't have default value here
        // People would use this for better perf anyway
        auto get(const KEY &key, VALUE *value)
        {
            return _map.get(_txn, key, _timeout, value);
        }

        auto get(const KEY &key, pair<int64_t, VALUE> *version_value, Store_LockMode lockmode)
        {
            return _map.get(_txn, key, _timeout, version_value, lockmode);
        }


        auto has(const KEY &key)
        {
            return _map.get(_txn, key, _timeout, nullptr);
        }

        auto update(const KEY &key, const VALUE &value)
        {
            return _map.update(_txn, key, value, _timeout);
        }

        auto update(const KEY &key, pair<int64_t, VALUE> &version_value)
        {
            return _map.update(_txn, key, version_value, _timeout);
        }

        auto remove(const KEY &key)
        {
            return _map.remove(_txn, key, _timeout);
        }

        auto add(const KEY& key, const VALUE &value)
        {
            // return the awaitable directly
            // it's faster, and avoids a clang bug when you co_return co_await awaitable<void>
            return _map.add(_txn, key, value, _timeout);
        }

        auto get_keys(const char16_t *first_key = nullptr, const char16_t *second_key = nullptr)
        {
            // return the awaitable directly
            // it's faster, and avoids a clang bug when you co_return co_await 
            return _map.get_keys(_txn, first_key, second_key);
        }

    private :
        reliable_map<KEY, VALUE>    &_map;
        reliable_transaction        &_txn;
        ticks               _timeout;
    };

    /*
    // result of each iteration
    class reliable_map_container
    {
    public :
        reliable_map_container(u16string name, StateProviderHandle handle)
            :_name(name), _handle(handle)
        {
        }

        const u16string& get_name()
        {
            return _name;
        }

        template <typename KEY, typename VALUE>
        auto get_map()
        {
            // @TODO - need to AddRef
            return reliable_map<KEY, VALUE>(_handle);
        }

        ~reliable_map_container()
        {
            // @TODO - need to release when we have proper AddRef
        }

    private :
        u16string _name;
        StateProviderHandle _handle;
    };
    
    //
    // Because all the reliable_map may have different KEY/VALUE, it returns reliable_map_ptr which you
    // can then call .get_name() and .get_map<KEY, VALUE>()
    //
    struct reliable_map_iterator : std::iterator<input_iterator_tag, reliable_map_container>
    {                
        enum iterator_status
        {
            in_progress,        // in-progress 
            past_end            // iterator is in a "past-end" state - calling operator * will throw
        };

        reliable_map_iterator()
            :reliable_map_iterator(nullptr, false, false)
        {
        }

        reliable_map_iterator(const reliable_map_iterator &other)
            :reliable_map_iterator(
                other._replicator, 
                other._parents_only, 
                other._status == iterator_status::past_end)
        {
        }



        ~reliable_map_iterator()
        {
            reset();
        }

        void reset()
        {
            if (_handle != nullptr)
            {
                binding::get()._pfnStateProviderEnumerator_Release(_handle);
                _handle = nullptr;
            }

            _status = iterator_status::in_progress;
        }

        void end()
        {
            _status = iterator_status::past_end;
        }

        reliable_map_iterator &operator =(const reliable_map_iterator &other)
        {
            reset();

            _replicator = other._replicator;
            if (other._status == iterator_status::past_end)
            {
                // copy the past_end status as well
                _status = iterator_status::past_end;
            }

            return *this;
        }

        // l-value ++
        reliable_map_iterator &operator ++()
        {
            move_next();

            return *this;
        }

        // r-value ++
        reliable_map_iterator operator ++(int)
        {
            move_next();

            return *this;
        }

        auto operator *()
        {
            validate();

            return reliable_map_container(_provider_name, _store);
        }

        bool operator == (const reliable_map_iterator &other) const
        {
            // is it the same iterator?
            if (this == &other)
                return true;

            // must be on the same replicator / context
            if (this->_replicator != other._replicator)
                return false;

            // must have same setting
            if (this->_parents_only != other._parents_only)
                return false;

            // first/past_end iterator are the same
            // this is required for sane begin()/end() comparisons
            // also we optimize begin() to avoid the enumerator construction
            if (this->_status == other._status)
            {
                // two past-end iterators are the same, good for 
                // 1. end() == end()
                // 2. begin() == end() for empty
                if (this->_status == iterator_status::past_end)
                    return true;

                // otherwise we resort to compare the StateProviderHandle. not super precise but good enough in
                // most cases, such as begin() == begin() check
                return this->_store == other._store;
            }

            return false;
        }

        bool operator != (const reliable_map_iterator &other) const
        {
            return !(operator ==(other));
        }

        bool is_end() const { return _status == iterator_status::past_end; }

    private:
        reliable_map_iterator(TxnReplicatorHandle replicator, bool parents_only, bool is_end)
        {
            _replicator = replicator;
            _parents_only = false;
            _handle = nullptr;
            _store = nullptr;

            if (_replicator != nullptr && !is_end)
            {
                // Unfortunately we need to always try retriving the first value - otherwise we won't know
                // begin() == end()
                StateProviderEnumeratorHandle enumerator_handle;
                HRESULT status = binding::get()._pfnTxnReplicator_CreateEnumerator(_replicator, _parents_only, &enumerator_handle);
                if (IF_FAILED(status))
                    reliable_exception::throw_error(status, "TxnReplicator_CreateEnumerator");

                _handle = enumerator_handle;
                _status = iterator_status::in_progress;

                move_next();
            }
            else
            {
                if (is_end)
                    _status = iterator_status::past_end;
                else
                    _status = iterator_status::in_progress;
            }
        }

        friend reliable_context;

        void validate()
        {
            if (_status == iterator_status::past_end)
                throw out_of_range("reliable_map_iterator");
        }

        void move_next()
        {
            // @TODO - need to release StateProviderHandle when we move to next StateProviderHandle, but right now we don't
            // have Store_AddRef, so calling release here isn't safe when people have already created a map
            validate();

            const char16_t *provider_name = nullptr;
            StateProviderHandle store;
            BOOL advanced;

            HRESULT status = binding::get()._pfnStateProviderEnumerator_MoveNext(_handle, &advanced, &provider_name, &store);
            if (IF_FAILED(status))
                reliable_exception::throw_error(status, "StateProviderEnumerator::MoveNext");

            if (advanced)
            {
                _provider_name = provider_name;
                _store = store;
            }
            else
            {
                _status = iterator_status::past_end;
            }
        }

    private:
        iterator_status         _status;
        bool                    _parents_only;
        TxnReplicatorHandle       _replicator;
        StateProviderEnumeratorHandle _handle;
        StateProviderHandle            _store;
        std::u16string          _provider_name;
    };
    */

    #define STORE_PREFIX u"fabric:/tstoresp/"

    static u16string add_store_prefix(const char16_t *name)
    {
        return u16string(STORE_PREFIX) + name;
    }

    static u16string remove_store_prefix(const char16_t *name)
    {
        return u16string(name).substr(sizeof(STORE_PREFIX) / sizeof(char16_t) - 1);
    }

    struct reliable_map_enumerator
    {
        enum enumerator_status
        {
            first, 
            in_progress,
            end
        };

        reliable_map_enumerator(StateProviderEnumeratorHandle handle)
        {
            _handle = handle;
            _status = enumerator_status::first;
            _store = nullptr;
        }

        reliable_map_enumerator(const reliable_map_enumerator &) = delete;

        reliable_map_enumerator(reliable_map_enumerator &&other)
        {
            _handle = other._handle;
            _status = other._status;
            _store = other._store;

            other._handle = nullptr;                        
        }

        reliable_map_enumerator &operator =(reliable_map_enumerator &&other)
        {
            _handle = other._handle;
            _status = enumerator_status::first;
            _store = nullptr;
            _name.clear();

            other._handle = nullptr;
            return *this;
        }

        reliable_map_enumerator &operator =(const reliable_map_enumerator &) = delete;

        ~reliable_map_enumerator()
        {
            if (_handle != nullptr)
            {
                binding::get()._pfnStateProviderEnumerator_Release(_handle);
                _handle = nullptr;
            }
        }

        bool move_next()
        {
            // @TODO - need to release StateProviderHandle when we move to next StateProviderHandle, but right now we don't
            // have Store_AddRef, so calling release here isn't safe when people have already created a map
            if (_status == enumerator_status::end)
                return false;

            const char16_t *provider_name = nullptr;
            StateProviderHandle store;
            BOOL advanced;

            HRESULT status = binding::get()._pfnStateProviderEnumerator_MoveNext(_handle, &advanced, &provider_name, &store);
            if (FAILED(status))
                reliable_exception::throw_error(status, "StateProviderEnumerator::MoveNext");

            if (advanced)
            {
                _name = remove_store_prefix(provider_name);
                _store = store;
                _status = enumerator_status::in_progress;

                return true;
            }
            else
            {
                _status = enumerator_status::end;

                return false;
            }
        }

        template <typename KEY, typename VALUE>
        reliable_map<KEY, VALUE> get_map()
        {
            validate();

            // @TODO - AddRef
            return reliable_map<KEY, VALUE>(_store);
        }

        const u16string &get_name()
        {
            validate();

            return _name;
        }

    private :
        void validate()
        {
            if (_handle == nullptr ||                           // moved
                _status != enumerator_status::in_progress)      // data not available or past end
                throw out_of_range("reliable_map_enumerator");
        }
 
    private :
        enumerator_status       _status;
        u16string               _name;
        StateProviderEnumeratorHandle _handle;
        StateProviderHandle            _store;
    };

    class reliable_context
    {
    public:
        reliable_context(KEY_TYPE key, TxnReplicatorHandle replicator, PartitionHandle partition)
            :_key(key), _replicator(replicator), _partition(partition)
        {

        }

    public:        
        /*
        auto begin(bool parents_only = false)
        {
            return reliable_map_iterator(_replicator, parents_only, false);            
        }

        auto end(bool parents_only = false)
        {
            return reliable_map_iterator(_replicator, parents_only, true);
        }
        */

        reliable_map_enumerator get_maps(bool parents_only = false)
        {
            StateProviderEnumeratorHandle handle;
            HRESULT status = binding::get()._pfnTxnReplicator_CreateEnumerator(_replicator, parents_only, &handle);
            if (FAILED(status))
                reliable_exception::throw_error(status, "TxnReplicator_CreateEnumerator");

            return reliable_map_enumerator(handle);
        }


        template <typename KEY, typename VALUE>
        class get_reliable_map_operation : public ktl_awaitable<reliable_map<KEY, VALUE>>
        {
            public:
                get_reliable_map_operation(TxnReplicatorHandle replicator, const char16_t *map_name, ticks timeout, bool *exists)
                    :_replicator(replicator), _map_name(add_store_prefix(map_name)), _timeout(timeout), _exists(exists)
                {
                    _cts = NULL;
                }

            protected:
                virtual bool on_start()
                {
                    BOOL sync_completed;
                    StateProviderHandle store;
                    BOOL does_exist;
                    HRESULT status = binding::get()._pfnTxnReplicator_GetOrAddStateProviderAsync(
                        _replicator,
                        nullptr,                    // force GetOrAddStateProviderAsync always create a new transaction  
                        _map_name.c_str(),
                        u"cpp",                     // cpp = C++
                        nullptr,                    // no additional StateProviderInfo required (C++ doesn't need language metadata)
                        _timeout.count(),           // timeout in ticks
                        NULL,                       // CancellationTokenSourceHandle
                        &store,
                        &does_exist,
                        [](void *context, HRESULT status, StateProviderHandle hnd, BOOL exists) {
                            auto op = reinterpret_cast<get_reliable_map_operation*>(context);
                            if (SUCCEEDED(status))
                            {
                                op->set_result(reliable_map<KEY, VALUE>(hnd));
                            }
                            else
                            {
                                op->set_error(status, std::string("TxnReplicator_GetOrAddStateProviderAsync"));
                            }

                            if (op->_exists)
                                *op->_exists = exists;

                            op->post_resume();
                        },
                        this,
                        &sync_completed);

                    // @BUG - should not return HRESULT_NAME_ALREADY_EXISTS if the name is already there
                    // and also the returned store is NULL...
                    if (FAILED(status))
                    {
                        this->set_error(status, std::string("TxnReplicator_GetOrAddStateProviderAsync"));
                        return true;
                    }

                    if (sync_completed)
                    {
                        if (_exists)
                            *_exists = does_exist;
                        this->set_result(reliable_map<KEY, VALUE>(store));
                        return true;
                    }

                    return false;
                }

            private:
                TxnReplicatorHandle _replicator;
                std::u16string _map_name;
                ticks _timeout;
                bool *_exists;
                CancellationTokenSourceHandle _cts;
        };

        template <typename KEY, typename VALUE>
        auto get_reliable_map(const char16_t *map_name, ticks timeout, bool *exists = nullptr)
        {
            return get_reliable_map_operation <KEY, VALUE> (_replicator, map_name, timeout, exists);        
        }

        template <typename KEY, typename VALUE>
        auto get_reliable_map_if_exists(const char16_t *map_name, bool *exists = nullptr, HRESULT* pStatus = nullptr)
        {
            StateProviderHandle store;
            u16string store_name(add_store_prefix(map_name));
            HRESULT status = binding::get()._pfnTxnReplicator_GetStateProvider(
                _replicator,
                store_name.c_str(),
                &store);

            if (pStatus)
            {
                *pStatus = status;
            }

            if (FAILED(status))
            {
                if (exists)
                    *exists = false;
                return reliable_map<KEY, VALUE>();
            }

            if (exists)
                *exists = TRUE;

            return reliable_map<KEY, VALUE>(store);
        }

        auto remove_reliable_map(const char16_t *map_name, ticks timeout)
        {
            class remove_reliable_map_operation : public ktl_awaitable<void>
            {
            public:
                remove_reliable_map_operation(TxnReplicatorHandle replicator, const char16_t *map_name, ticks timeout)
                    :_replicator(replicator), _map_name(add_store_prefix(map_name)), _timeout(timeout)
                {
                    _cts = NULL;
                }

            protected:
                virtual bool on_start()
                {
                    BOOL sync_completed;
                    HRESULT status = binding::get()._pfnTxnReplicator_RemoveStateProviderAsync(
                        _replicator,
                        nullptr,                    // force creating a new transaction
                        _map_name.c_str(),
                        _timeout.count(),           // timeout in ticks
                        NULL,                       // CancellationTokenSourceHandle
                        [](void *context, HRESULT status) 
                        {
                            auto op = reinterpret_cast<remove_reliable_map_operation*>(context);
                            if (SUCCEEDED(status))
                            {
                                op->set_result();
                            }
                            else
                            {
                                op->set_error(status, std::string("TxnReplicator_RemoveStateProviderAsync"));
                            }

                            op->post_resume();
                        },
                        this,
                        &sync_completed);
                    if (FAILED(status))
                    {
                        set_error(status, std::string("TxnReplicator_RemoveStateProviderAsync"));
                        return true;
                    }

                    if (sync_completed)
                    {
                        set_result();
                        return true;
                    }

                    return false;
                }

            private:
                TxnReplicatorHandle _replicator;
                std::u16string _map_name;
                ticks _timeout;
                CancellationTokenSourceHandle _cts;
            };

            return remove_reliable_map_operation(_replicator, map_name, timeout);
        }
        
        reliable_transaction create_transaction()
        {
            TransactionHandle transaction;
            HRESULT status = binding::get()._pfnTxnReplicator_CreateTransaction(_replicator, &transaction);
            if (FAILED(status))
                reliable_exception::throw_error(status, "TxnReplicator_CreateTransaction");
            
            return reliable_transaction(transaction);
        }

        task<void> execute(const std::function <task<void> (reliable_transaction &txn)> func)
        {
            {
                auto transaction = create_transaction();
                co_await func(transaction);
                co_await transaction.commit();
            }
            
            co_return;
        }

        KEY_TYPE get_key()
        {
            return _key;
        }

        TxnReplicatorHandle get_replicator()
        {
            return _replicator;
        }

        PartitionHandle get_partition()
        {
            return _partition;
        }

    private:
        KEY_TYPE _key;
        TxnReplicatorHandle _replicator;
        PartitionHandle _partition;
    };


    class rw_spin_lock
    {
    public:
        rw_spin_lock()
        {
            _readers = 0;
        }

    public:
        void acquire_reader()
        {
            int retry = 0;
            while (true)
            {
                uint32_t prev_readers = _readers;
                if (prev_readers != HAS_WRITER)
                {
                    uint32_t new_readers = prev_readers + 1;
                    if (_readers.compare_exchange_weak(prev_readers, new_readers))
                    {
                        // we've won the race
                        return;
                    }
                }

                retry++;
                if (retry > RETRY_THRESHOLD)
                {
                    retry = 0;
                    this_thread::yield();
                }
            }
        }

        void release_reader()
        {
            int retry = 0;
            while (true)
            {
                uint32_t prev_readers = _readers;
                if (prev_readers != HAS_WRITER && prev_readers > 0)
                {
                    uint32_t new_readers = prev_readers - 1;
                    if (_readers.compare_exchange_weak(prev_readers, new_readers))
                    {
                        // we've won the race
                        return;
                    }
                }

                retry++;
                if (retry > RETRY_THRESHOLD)
                {
                    retry = 0;
                    this_thread::yield();
                }
            }
        }

        void acquire_writer()
        {
            int retry = 0;
            while (true)
            {
                uint32_t prev_readers = _readers;
                if (prev_readers == 0)
                {
                    if (_readers.compare_exchange_weak(prev_readers, HAS_WRITER))
                    {
                        // we've won the race
                        return;
                    }
                }

                retry++;
                if (retry > RETRY_THRESHOLD)
                {
                    // save some cpu cycles
                    retry = 0;
                    this_thread::yield();
                }
            }
        }

        void release_writer()
        {
            int retry = 0;
            while (true)
            {
                uint32_t prev_readers = _readers;
                if (prev_readers == HAS_WRITER)
                {
                    if (_readers.compare_exchange_weak(prev_readers, 0))
                    {
                        // we've won the race
                        return;
                    }
                }

                retry++;
                if (retry > RETRY_THRESHOLD)
                {
                    // save some cpu cycles
                    retry = 0;
                    this_thread::yield();
                }
            }
        }

    private:
        const uint32_t HAS_WRITER = 0xffffffff;
        const int RETRY_THRESHOLD = 100;
        std::atomic<uint32_t> _readers;
    };

    class reader_lock
    {
    public:
        reader_lock(rw_spin_lock &lock) : _lock(lock)
        {
            _lock.acquire_reader();
        }

        ~reader_lock()
        {
            _lock.release_reader();
        }

    private:
        rw_spin_lock &_lock;
    };

    class writer_lock
    {
    public:
        writer_lock(rw_spin_lock &lock) : _lock(lock)
        {
            _lock.acquire_writer();
        }

        ~writer_lock()
        {
            _lock.release_writer();
        }

    private:
        rw_spin_lock &_lock;
    };

    // Define the prototype for ChangeRole Callback
    typedef void (*FuncChangeRoleCallback)(KEY_TYPE key, int32_t newRole);

    // Define the prototype for Remove Partition Callback
    typedef void(*FuncRemovePartitionCallback)(KEY_TYPE key);

    // Define the prototype for abort Partition Callback
    typedef void(*FuncAbortPartitionCallback)(KEY_TYPE key);

    // Define the prototype for add Partition Callback
    typedef void(*FuncAddPartitionCallback)(KEY_TYPE key, TxnReplicatorHandle replicator, PartitionHandle partition);

    class reliable_service
    {
    public:
        reliable_service()
        {
            binding::get().init();
            
            // this is a bit awkward but we need to hide the static inside get() for this to remain a header
            get() = this;

            pfChangeRoleCallback_ = nullptr;
            pfRemovePartitionCallback_ = nullptr;
            pfAbortPartitionCallback_ = nullptr;
            pfAddPartitionCallback_ = nullptr;
        }

        ~reliable_service()
        {
            // Perform cleanup to release resources used by ReliableCollectionService
            binding::get()._pfnReliableCollectionService_Cleanup();
        }

        static reliable_service *&get()
        {
            // use a local static to support having everything in headers
            static reliable_service *s_reliable_service;

            return s_reliable_service;
        }

        reliable_context get_context(KEY_TYPE key)
        {
            reader_lock lock(_rw_lock);
            partition_info info;
            if (find_partition_info_no_lock(key, info))
            {
                return reliable_context(info.key, info.replicator, info.partition);
            }

            // @TODO - What's the proper error code here
            throw reliable_exception(-1, "get_context");
        }

        reliable_context get_context()
        {
            // @TODO - we need an API
            throw nullptr;
        }

        HRESULT get_replicator_info(TxnReplicatorHandle txnReplicator, TxnReplicator_Info* info)
        {
            return binding::get()._pfnReliableCollectionTxnReplicator_GetInfo(txnReplicator, info);
        }
        
        void set_ChangeRoleCallback(FuncChangeRoleCallback changeRoleCallback)
        {
            pfChangeRoleCallback_ = changeRoleCallback;
        }

        void set_RemovePartitionCallback(FuncRemovePartitionCallback removePartitionCallback)
        {
            pfRemovePartitionCallback_ = removePartitionCallback;
        }

        void set_AbortPartitionCallback(FuncAbortPartitionCallback abortPartitionCallback)
        {
            pfAbortPartitionCallback_ = abortPartitionCallback;
        }

        void set_AddPartitionCallback(FuncAddPartitionCallback addPartitionCallback)
        {
            pfAddPartitionCallback_ = addPartitionCallback;
        }

        void initialize(const char16_t *service_name, int port, BOOL registerManifestEndpoints, 
            BOOL skipUserPortEndpointRegistration,
            BOOL reportEndpointsOnlyOnPrimaryReplica)
        {
            HRESULT hrInit = binding::get()._pfnReliableCollectionRuntime_Initialize(RELIABLECOLLECTION_API_VERSION);
            if (FAILED(hrInit))
            {
                throw reliable_exception(hrInit, "ReliableCollectionRuntime initialization failed.");
            }

            // @TODO - context should support cookies
            hrInit = binding::get()._pfnReliableCollectionService_InitializeEx(
                service_name,
                port,
                [](auto key, auto replicator_hnd, auto partition_hnd, auto partitionId, auto replicaId) {
                    reliable_service::get()->add_context_callback(key, replicator_hnd, partition_hnd, partitionId, replicaId);
                },
                [](auto key) {
                    reliable_service::get()->remove_context_callback(key);
                },
                [](auto key, auto newRole) {
                    reliable_service::get()->change_role_callback(key, newRole);
                },
                [](auto key) {
                    reliable_service::get()->abort_callback(key);
                },
                registerManifestEndpoints,
                skipUserPortEndpointRegistration,
                reportEndpointsOnlyOnPrimaryReplica);

                if (FAILED(hrInit))
                {
                    throw reliable_exception(hrInit, "ReliableCollectionService initialization failed.");
                }
        }

#ifdef RELIABLE_COLLECTION_TEST
    public :
#else
    private :
#endif
        void add_context_callback(KEY_TYPE key, TxnReplicatorHandle replicator, PartitionHandle partition, GUID partitionId, int64_t replicaId)
        {
            {// @TODO - should we move this logic into the C API layer
                writer_lock lock(_rw_lock);
                add_partition_info_no_lock({ key, replicator, partition, partitionId, replicaId });
            }
            if (pfAddPartitionCallback_ != nullptr)
            {
                pfAddPartitionCallback_(key, replicator, partition);
            }
        }

        void remove_context_callback(KEY_TYPE key)
        {
            if (pfRemovePartitionCallback_ != nullptr)
            {
                pfRemovePartitionCallback_(key);
            }
            writer_lock lock(_rw_lock);
            remove_partition_info_no_lock(key);
        }

        void change_role_callback(KEY_TYPE key, int32_t newRole)
        {
            if (pfChangeRoleCallback_ != nullptr)
            {
                (*pfChangeRoleCallback_)(key, newRole);
            }
        }

        void abort_callback(KEY_TYPE key)
        {
            if (pfAbortPartitionCallback_ != nullptr)
            {
                (*pfAbortPartitionCallback_)(key);
            }
        }

        struct partition_info
        {
            KEY_TYPE            key;
            TxnReplicatorHandle   replicator;
            PartitionHandle    partition;
            GUID partitionId;
            int64_t replicaId;
        };

        bool find_partition_info_no_lock(KEY_TYPE key, partition_info &info)
        {
            // @TODO - is this correct for partitions with secondaries?
            for (auto it = _partitions.begin(); it != _partitions.end(); ++it)
            {
                if (key < it->key)
                {
                    if (it == _partitions.begin())
                        return false;

                    it--;
                    info = *it;
                    return true;
                }
            }

            if (_partitions.size() > 0)
            {
                // must be the last one
                info = _partitions.back();
                return true;
            }

            return false;
        }

        void add_partition_info_no_lock(partition_info info)
        {
            auto it = _partitions.begin();
            for (; it != _partitions.end(); ++it)
            {
                if (info.key < it->key)
                {
                    // insert before
                    break;
                }
                else if (info.key == it->key)
                {
                    // update - but is this a bug?
                    *it = info;
                    return;
                }
            }

            _partitions.insert(it, info);
        }

        void remove_partition_info_no_lock(KEY_TYPE key)
        {
            for (auto it = _partitions.begin(); it != _partitions.end(); ++it)
            {
                if (it->key == key)
                {
                    _partitions.erase(it);
                    return;
                }
            }

            return;
        }

    private:
        atomic<int> _writers;
        list<partition_info> _partitions;
        rw_spin_lock _rw_lock;
        FuncChangeRoleCallback pfChangeRoleCallback_;
        FuncRemovePartitionCallback pfRemovePartitionCallback_;
        FuncAbortPartitionCallback pfAbortPartitionCallback_;
        FuncAddPartitionCallback pfAddPartitionCallback_;      
    };
}
