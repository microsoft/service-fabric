// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#ifdef PLATFORM_UNIX

#define copy_n_checked(input, inputCount, output, outputCount) \
    std::copy_n(input, inputCount, output)

#define uninitialized_copy_n_checked(input, inputCount, output, outputCount) \
    std::uninitialized_copy_n(input, inputCount, output)

#else

#define copy_n_checked(input, inputCount, output, outputCount) \
    std::copy_n(input, inputCount, stdext::checked_array_iterator<std::iterator_traits<decltype(output)>::pointer>(output, outputCount));

#define uninitialized_copy_n_checked(input, inputCount, output, outputCount) \
    std::uninitialized_copy_n(input, inputCount, stdext::checked_array_iterator<std::iterator_traits<decltype(output)>::pointer>(output, outputCount))

#endif

namespace Common
{
    namespace detail
    {
        template <class T> class rref_holder
        {
            rref_holder & operator = (rref_holder const&);
        public:
            rref_holder (rref_holder const & r) : value_(std::move(r.value_)) {}
            rref_holder (T && value) : value_(std::move(value)) {}
            operator T () { return std::move(value_); }
            T move_out() { return std::move(value_); }
        private:
            mutable T value_;
        };
    }

    template <class T> detail::rref_holder<T> rref_move(T & value) { return detail::rref_holder<T>(std::move(value)); }
}

namespace Common
{
    template<typename Iter> struct range : std::pair<Iter, Iter>
    {
        range(Iter && a, Iter && b) : std::pair<Iter, Iter> (std::forward<Iter>(a), std::forward<Iter>(b)) {}

        typedef Iter iterator;

        iterator begin() const { return first; }
        iterator end() const { return second; }
        bool empty() const { return first == second; }
        size_t size() const { return second - first; }

        using std::pair<Iter,Iter>::first;
        using std::pair<Iter,Iter>::second;
    };

    template<typename Iter> Iter begin(range<Iter> const & p ) { return p.first; }
    template<typename Iter> Iter end(range<Iter> const & p ) { return p.second; }

    template<typename T> range<typename T::const_iterator> make_range(T const & t)
    {
        return range<typename T::const_iterator>(t.cbegin(), t.cend());
    }

    template<typename T> range<typename T::iterator> make_range(T & t)
    {
        return range<typename T::iterator>(t.begin(), t.end());
    }

    template<typename Iter> range<Iter> make_range(Iter const & a, Iter const & b)
    {
        return range<Iter, Iter>(a, b);
    }
}

namespace Common
{
    template <class T>
    std::unique_ptr<T> make_unique()
    {
        return std::unique_ptr<T>(new T);
    }

    template <class T>
    void reset_unique(std::unique_ptr<T> & ptr)
    {
        ptr.reset(new T);
    }

    template <class T, class D>
    std::unique_ptr<T,D> make_unique()
    {
        return std::unique_ptr<T,D>(new T);
    }

    template <class T, class t0>
    std::unique_ptr<T> make_unique(t0 && a0) {
        return std::unique_ptr<T>(new T(std::forward<t0>(a0)));
    }

    template <class T, class t0>
    void reset_unique(std::unique_ptr<T> & ptr, t0 && a0)
    {
        ptr.reset(new T(std::forward<t0>(a0)));;
    }

    template <class T, class D, class t0>
    std::unique_ptr<T,D> make_unique(t0 && a0) {
        return std::unique_ptr<T,D>(new T(std::forward<t0>(a0)));
    }

    template <class T, class t0, class t1>
    std::unique_ptr<T> make_unique(t0 && a0, t1 && a1) {
        return std::unique_ptr<T>(new T(std::forward<t0>(a0), std::forward<t1>(a1)));
    }

    template <class T, class t0, class t1, class t2>
    std::unique_ptr<T> make_unique(t0 && a0, t1 && a1, t2 && a2) {
        return std::unique_ptr<T>(new T(
            std::forward<t0>(a0), std::forward<t1>(a1), std::forward<t2>(a2)));
    }

    template <class T, class t0, class t1, class t2, class t3>
    std::unique_ptr<T> make_unique(t0 && a0, t1 && a1, t2 && a2, t3 && a3) {
        return std::unique_ptr<T>(new T(
            std::forward<t0>(a0), std::forward<t1>(a1), std::forward<t2>(a2), std::forward<t3>(a3)));
    }

    template <class T, class t0, class t1, class t2, class t3, class t4>
    std::unique_ptr<T> make_unique(t0 && a0, t1 && a1, t2 && a2, t3 && a3, t4 && a4) {
        return std::unique_ptr<T>(new T(
            std::forward<t0>(a0), std::forward<t1>(a1), std::forward<t2>(a2), std::forward<t3>(a3), std::forward<t4>(a4)));
    }

    template <class T, class t0, class t1, class t2, class t3, class t4, class t5>
    std::unique_ptr<T> make_unique(t0 && a0, t1 && a1, t2 && a2, t3 && a3, t4 && a4, t5 && a5) {
        return std::unique_ptr<T>(new T(
            std::forward<t0>(a0), std::forward<t1>(a1), std::forward<t2>(a2), std::forward<t3>(a3), std::forward<t4>(a4), std::forward<t5>(a5)));
    }

    template <class T, class t0, class t1, class t2, class t3, class t4, class t5, class t6>
    std::unique_ptr<T> make_unique(t0 && a0, t1 && a1, t2 && a2, t3 && a3, t4 && a4, t5 && a5, t6 && a6) {
        return std::unique_ptr<T>(new T(
            std::forward<t0>(a0), std::forward<t1>(a1), std::forward<t2>(a2), std::forward<t3>(a3),
            std::forward<t4>(a4), std::forward<t5>(a5), std::forward<t6>(a6)));
    }

    template <class T, class t0, class t1, class t2, class t3, class t4, class t5, class t6, class t7>
    std::unique_ptr<T> make_unique(t0 && a0, t1 && a1, t2 && a2, t3 && a3, t4 && a4, t5 && a5, t6 && a6, t7 && a7) {
        return std::unique_ptr<T>(new T(
            std::forward<t0>(a0), std::forward<t1>(a1), std::forward<t2>(a2), std::forward<t3>(a3),
            std::forward<t4>(a4), std::forward<t5>(a5), std::forward<t6>(a6), std::forward<t7>(a7)));
    }

    template <class T, class t0, class t1, class t2, class t3, class t4, class t5, class t6, class t7, class t8>
    std::unique_ptr<T> make_unique(t0 && a0, t1 && a1, t2 && a2, t3 && a3, t4 && a4, t5 && a5, t6 && a6, t7 && a7, t8 && a8) {
        return std::unique_ptr<T>(new T(
            std::forward<t0>(a0), std::forward<t1>(a1), std::forward<t2>(a2), std::forward<t3>(a3),
            std::forward<t4>(a4), std::forward<t5>(a5), std::forward<t6>(a6), std::forward<t7>(a7),
            std::forward<t8>(a8)));
    }

    template <class T, class t0, class t1, class t2, class t3, class t4, class t5, class t6, class t7, class t8, class t9>
    std::unique_ptr<T> make_unique(t0 && a0, t1 && a1, t2 && a2, t3 && a3, t4 && a4, t5 && a5, t6 && a6, t7 && a7, t8 && a8, t9 && a9) {
        return std::unique_ptr<T>(new T(
            std::forward<t0>(a0), std::forward<t1>(a1), std::forward<t2>(a2), std::forward<t3>(a3),
            std::forward<t4>(a4), std::forward<t5>(a5), std::forward<t6>(a6), std::forward<t7>(a7),
            std::forward<t8>(a8), std::forward<t9>(a9)));
    }

    template <class T, class t0, class t1, class t2, class t3, class t4, class t5, class t6, class t7, class t8, class t9, class t10>
    std::unique_ptr<T> make_unique(t0 && a0, t1 && a1, t2 && a2, t3 && a3, t4 && a4, t5 && a5, t6 && a6, t7 && a7, t8 && a8, t9 && a9, t10 && a10) {
        return std::unique_ptr<T>(new T(
            std::forward<t0>(a0), std::forward<t1>(a1), std::forward<t2>(a2), std::forward<t3>(a3),
            std::forward<t4>(a4), std::forward<t5>(a5), std::forward<t6>(a6), std::forward<t7>(a7),
            std::forward<t8>(a8), std::forward<t9>(a9), std::forward<t10>(a10)));
    }

    template <class T, class t0, class t1, class t2, class t3, class t4, class t5, class t6, class t7, class t8, class t9, class t10,  class t11>
    std::unique_ptr<T> make_unique(t0 && a0, t1 && a1, t2 && a2, t3 && a3, t4 && a4, t5 && a5, t6 && a6, t7 && a7, t8 && a8, t9 && a9, t10 && a10, t11 && a11) {
        return std::unique_ptr<T>(new T(
            std::forward<t0>(a0), std::forward<t1>(a1), std::forward<t2>(a2), std::forward<t3>(a3),
            std::forward<t4>(a4), std::forward<t5>(a5), std::forward<t6>(a6), std::forward<t7>(a7),
            std::forward<t8>(a8), std::forward<t9>(a9), std::forward<t10>(a10), std::forward<t11>(a11)));
    }

    template <class T, class t0, class t1, class t2, class t3, class t4, class t5, class t6, class t7, class t8, class t9, class t10,  class t11, class t12>
    std::unique_ptr<T> make_unique(t0 && a0, t1 && a1, t2 && a2, t3 && a3, t4 && a4, t5 && a5, t6 && a6, t7 && a7, t8 && a8, t9 && a9, t10 && a10, t11 && a11, t12 && a12) {
        return std::unique_ptr<T>(new T(
            std::forward<t0>(a0), std::forward<t1>(a1), std::forward<t2>(a2), std::forward<t3>(a3),
            std::forward<t4>(a4), std::forward<t5>(a5), std::forward<t6>(a6), std::forward<t7>(a7),
            std::forward<t8>(a8), std::forward<t9>(a9), std::forward<t10>(a10), std::forward<t11>(a11), std::forward<t12>(a12)));
    }

    template <class T, class t0, class t1, class t2, class t3, class t4, class t5, class t6, class t7, class t8, class t9, class t10,  class t11, class t12, class t13>
    std::unique_ptr<T> make_unique(t0 && a0, t1 && a1, t2 && a2, t3 && a3, t4 && a4, t5 && a5, t6 && a6, t7 && a7, t8 && a8, t9 && a9, t10 && a10, t11 && a11, t12 && a12, t13 && a13) {
        return std::unique_ptr<T>(new T(
            std::forward<t0>(a0), std::forward<t1>(a1), std::forward<t2>(a2), std::forward<t3>(a3),
            std::forward<t4>(a4), std::forward<t5>(a5), std::forward<t6>(a6), std::forward<t7>(a7),
            std::forward<t8>(a8), std::forward<t9>(a9), std::forward<t10>(a10), std::forward<t11>(a11), 
            std::forward<t12>(a12), std::forward<t13>(a13)));
    }

    template <class T, class t0, class t1, class t2, class t3, class t4, class t5, class t6, class t7, class t8, class t9, class t10,  class t11, class t12, class t13, class t14>
    std::unique_ptr<T> make_unique(t0 && a0, t1 && a1, t2 && a2, t3 && a3, t4 && a4, t5 && a5, t6 && a6, t7 && a7, t8 && a8, t9 && a9, t10 && a10, t11 && a11, t12 && a12, t13 && a13, t14 && a14) {
        return std::unique_ptr<T>(new T(
            std::forward<t0>(a0), std::forward<t1>(a1), std::forward<t2>(a2), std::forward<t3>(a3),
            std::forward<t4>(a4), std::forward<t5>(a5), std::forward<t6>(a6), std::forward<t7>(a7),
            std::forward<t8>(a8), std::forward<t9>(a9), std::forward<t10>(a10), std::forward<t11>(a11), 
            std::forward<t12>(a12), std::forward<t13>(a13), std::forward<t14>(a14)));
    }

    template <class T, class t0, class t1, class t2, class t3, class t4, class t5, class t6, class t7, class t8, class t9, class t10, class t11, class t12, class t13, class t14, class t15>
    std::unique_ptr<T> make_unique(t0 && a0, t1 && a1, t2 && a2, t3 && a3, t4 && a4, t5 && a5, t6 && a6, t7 && a7, t8 && a8, t9 && a9, t10 && a10, t11 && a11, t12 && a12, t13 && a13, t14 && a14, t15 && a15) {
        return std::unique_ptr<T>(new T(
            std::forward<t0>(a0), std::forward<t1>(a1), std::forward<t2>(a2), std::forward<t3>(a3),
            std::forward<t4>(a4), std::forward<t5>(a5), std::forward<t6>(a6), std::forward<t7>(a7),
            std::forward<t8>(a8), std::forward<t9>(a9), std::forward<t10>(a10), std::forward<t11>(a11),
            std::forward<t12>(a12), std::forward<t13>(a13), std::forward<t14>(a14), std::forward<t15>(a15)));
    }

    template <class T, class t0, class t1, class t2, class t3, class t4, class t5, class t6, class t7, class t8, class t9, class t10, class t11, class t12, class t13, class t14, class t15, class t16>
    std::unique_ptr<T> make_unique(t0 && a0, t1 && a1, t2 && a2, t3 && a3, t4 && a4, t5 && a5, t6 && a6, t7 && a7, t8 && a8, t9 && a9, t10 && a10, t11 && a11, t12 && a12, t13 && a13, t14 && a14, t15 && a15, t16 && a16) {
        return std::unique_ptr<T>(new T(
            std::forward<t0>(a0), std::forward<t1>(a1), std::forward<t2>(a2), std::forward<t3>(a3),
            std::forward<t4>(a4), std::forward<t5>(a5), std::forward<t6>(a6), std::forward<t7>(a7),
            std::forward<t8>(a8), std::forward<t9>(a9), std::forward<t10>(a10), std::forward<t11>(a11),
            std::forward<t12>(a12), std::forward<t13>(a13), std::forward<t14>(a14), std::forward<t15>(a15),
            std::forward<t16>(a16)));
    }

    template <class T, class t0, class t1, class t2, class t3, class t4, class t5, class t6, class t7, class t8, class t9, class t10, class t11, class t12, class t13, class t14, class t15, class t16, class t17>
    std::unique_ptr<T> make_unique(t0 && a0, t1 && a1, t2 && a2, t3 && a3, t4 && a4, t5 && a5, t6 && a6, t7 && a7, t8 && a8, t9 && a9, t10 && a10, t11 && a11, t12 && a12, t13 && a13, t14 && a14, t15 && a15, t16 && a16, t17 && a17) {
        return std::unique_ptr<T>(new T(
            std::forward<t0>(a0), std::forward<t1>(a1), std::forward<t2>(a2), std::forward<t3>(a3),
            std::forward<t4>(a4), std::forward<t5>(a5), std::forward<t6>(a6), std::forward<t7>(a7),
            std::forward<t8>(a8), std::forward<t9>(a9), std::forward<t10>(a10), std::forward<t11>(a11),
            std::forward<t12>(a12), std::forward<t13>(a13), std::forward<t14>(a14), std::forward<t15>(a15),
            std::forward<t16>(a16), std::forward<t17>(a17)));
    }

    template <class T, class D, class t0, class t1, class t2>
    std::unique_ptr<T,D> make_unique(t0 && a0, t1 && a1, t2 && a2) {
        return std::unique_ptr<T,D>(new T(
            std::forward<t0>(a0), std::forward<t1>(a1), std::forward<t2>(a2)));
    }

    template <class T, class D, class t0, class t1, class t2, class t3>
    std::unique_ptr<T,D> make_unique(t0 && a0, t1 && a1, t2 && a2, t3 && a3) {
        return std::unique_ptr<T,D>(new T(
            std::forward<t0>(a0), std::forward<t1>(a1), std::forward<t2>(a2), std::forward<t3>(a3)));
    }
}

namespace Common
{
    namespace detail
    {
        struct tracker_base
        {
            virtual ~tracker_base() {}
        };

        template <class Handler> struct tracker_impl : public tracker_base
        {
            tracker_impl (Handler && handler) : handler_(std::move(handler)) {}
            ~tracker_impl() { handler_(); }
        private:
            Handler handler_;
        };
    }
    typedef std::unique_ptr<detail::tracker_base> tracker;
    typedef std::shared_ptr<detail::tracker_base> shared_tracker;

    template <class Handler>
    tracker make_tracker(Handler && handler)
    {
        return Common::make_unique<detail::tracker_impl<Handler>>(std::move(handler));
    }

    template <class Handler>
    tracker make_shared_tracker(Handler && handler)
    {
        return std::make_shared<detail::tracker_impl<Handler>>(std::move(handler));
    }
}

namespace Common
{
    template <class T>
    class Global
    {
    public:
        Global()
            : data_(nullptr)
        {
        }

        Global(T* data)
            : data_(data)
        {
        }

        ~Global()
        {
        }

        T const* operator->() const
        {
            return data_;
        }

        T* operator->()
        {
            return data_;
        }

        T const & operator*() const
        {
            return *data_;
        }
        
        T & operator*()
        {
            return *data_;
        }

        operator T const & ()
        {
            return *data_;
        }

        operator bool() const
        {
            return data_ != nullptr;
        }

        void WriteTo(TextWriter& w, FormatOptions const &) const
        {
            w.Write(*data_);
        }

    private:
        T* data_;
    };

    typedef Global<std::string> GlobalString;
    typedef Global<std::wstring> GlobalWString;

#ifdef PLATFORM_UNIX
    using StringT = std::string;
    using StringLiteralT = StringLiteral; 
    using GlobalStringT = GlobalString;
#else
    using StringT = std::wstring;
    using StringLiteralT = WStringLiteral; 
    using GlobalStringT = GlobalWString;
#endif

    template <class T>
    Global<T> make_global()
    {
        return Global<T>(new T());
    }

    template <class T, class t0>
    Global<T> make_global(t0 && a0)
    {
        return Global<T>(new T(std::forward<t0>(a0)));
    }

    template <class T, class t0, class t1>
    Global<T> make_global(t0 && a0, t1 && a1)
    {
        return Global<T>(new T(std::forward<t0>(a0), std::forward<t1>(a1)));
    }

    template <class T, class t0, class t1, class t2>
    Global<T> make_global(t0 && a0, t1 && a1, t2 && a2)
    {
        return Global<T>(new T(std::forward<t0>(a0), std::forward<t1>(a1), std::forward<t2>(a2)));
    }

    template <class T, class t0, class t1, class t2, class t3>
    Global<T> make_global(t0 && a0, t1 && a1, t2 && a2, t3 && a3)
    {
        return Global<T>(new T(std::forward<t0>(a0), std::forward<t1>(a1), std::forward<t2>(a2), std::forward<t3>(a3)));
    }

    template <class T, class t0, class t1, class t2, class t3, class t4>
    Global<T> make_global(t0 && a0, t1 && a1, t2 && a2, t3 && a3, t4 && a4)
    {
        return Global<T>(new T(std::forward<t0>(a0), std::forward<t1>(a1), std::forward<t2>(a2), std::forward<t3>(a3), std::forward<t4>(a4)));
    }

    template <class T, class t0, class t1, class t2, class t3, class t4, class t5>
    Global<T> make_global(t0 && a0, t1 && a1, t2 && a2, t3 && a3, t4 && a4, t5 && a5)
    {
        return Global<T>(new T(std::forward<t0>(a0), std::forward<t1>(a1), std::forward<t2>(a2), std::forward<t3>(a3), std::forward<t4>(a4), std::forward<t5>(a5)));
    }

    template <class T, class t0, class t1, class t2, class t3, class t4, class t5, class t6>
    Global<T> make_global(t0 && a0, t1 && a1, t2 && a2, t3 && a3, t4 && a4, t5 && a5, t6 && a6)
    {
        return Global<T>(new T(std::forward<t0>(a0), std::forward<t1>(a1), std::forward<t2>(a2), std::forward<t3>(a3), std::forward<t4>(a4), std::forward<t5>(a5), std::forward<t6>(a6)));
    }

    template <class T, class t0, class t1, class t2, class t3, class t4, class t5, class t6, class t7>
    Global<T> make_global(t0 && a0, t1 && a1, t2 && a2, t3 && a3, t4 && a4, t5 && a5, t6 && a6, t7 && a7)
    {
        return Global<T>(new T(std::forward<t0>(a0), std::forward<t1>(a1), std::forward<t2>(a2), std::forward<t3>(a3), std::forward<t4>(a4), std::forward<t5>(a5), std::forward<t6>(a6), std::forward<t7>(a7)));
    }

    template <class T, class t0, class t1, class t2, class t3, class t4, class t5, class t6, class t7, class t8>
    Global<T> make_global(t0 && a0, t1 && a1, t2 && a2, t3 && a3, t4 && a4, t5 && a5, t6 && a6, t7 && a7, t8 && a8)
    {
        return Global<T>(new T(std::forward<t0>(a0), std::forward<t1>(a1), std::forward<t2>(a2), std::forward<t3>(a3), std::forward<t4>(a4), std::forward<t5>(a5), std::forward<t6>(a6), std::forward<t7>(a7), std::forward<t8>(a8)));
    }

    template <class T, class t0, class t1, class t2, class t3, class t4, class t5, class t6, class t7, class t8, class t9>
    Global<T> make_global(t0 && a0, t1 && a1, t2 && a2, t3 && a3, t4 && a4, t5 && a5, t6 && a6, t7 && a7, t8 && a8, t9 && a9)
    {
        return Global<T>(new T(std::forward<t0>(a0), std::forward<t1>(a1), std::forward<t2>(a2), std::forward<t3>(a3), std::forward<t4>(a4), std::forward<t5>(a5), std::forward<t6>(a6), std::forward<t7>(a7), std::forward<t8>(a8), std::forward<t9>(a9)));
    }
}
