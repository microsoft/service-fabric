    /*++

    (c) 2010 by Microsoft Corp. All Rights Reserved.

    KDeferredCall

      Defines general purpose mechanisms to asynchronously invoke an arbitrary
      delegates using the work item queue or jumping back into the async apartment
      from a non-async thread.


    Description:
      Kernel Tempate Library (KTL):

        KDeferredCall           Supports a single deferred call to a work item, tracks return value
                                Typically used as a member.  The object lives longer than the call-return
                                sequence.

        KDeferredJump           Supports multiple one way calls (doesn't track the return value),
                                and may go out of scope before the calls even start.  Usually a local
                                variable. Return values can be indirectly supported by using
                                out-parameters or passing in a delegate as a parameter and calling
                                it with the return value.

    History:
      raymcc          01-Jun-2012         Initial version

--*/

#pragma once

template <typename... Ts>
struct Tuple;

// Create specializations for each of the tuple sizes that are used. This is 
// because there is a lot of template magic needed to make this work generally. 
// At some point, this code should switch to use std::tuple, but that type is 
// not kernel friendly at this point in time.
template <>
struct Tuple<>
{
    void set()
    {
    }

    template <typename Delegate>
    auto apply(Delegate delegate)
    {
        return delegate();
    }
};

template <typename P0>
struct Tuple<P0>
{
    P0 _p0;

    Tuple()
    {
    }
    Tuple(const P0& p0) : _p0(p0)
    {
    }
    void set(const P0& p0)
    {
        _p0 = p0;
    }
    template <typename Delegate>
    auto apply(Delegate delegate)
    {
        return delegate(_p0);
    }
};

template <typename P0, typename P1>
struct Tuple<P0, P1>
{
    P0 _p0;
    P1 _p1;

    Tuple()
    {
    }
    Tuple(const P0& p0, const P1& p1) : _p0(p0), _p1(p1)
    {
    }
    void set(const P0& p0, const P1& p1)
    {
        _p0 = p0;
        _p1 = p1;
    }
    template <typename Delegate>
    auto apply(Delegate delegate)
    {
        return delegate(_p0, _p1);
    }
};

template <typename P0, typename P1, typename P2>
struct Tuple<P0, P1, P2>
{
    P0 _p0;
    P1 _p1;
    P2 _p2;

    Tuple()
    {
    }
    Tuple(const P0& p0, const P1& p1, const P2& p2) : _p0(p0), _p1(p1), _p2(p2)
    {
    }
    void set(const P0& p0, const P1& p1, const P2& p2)
    {
        _p0 = p0;
        _p1 = p1;
        _p2 = p2;
    }
    template <typename Delegate>
    auto apply(Delegate delegate)
    {
        return delegate(_p0, _p1, _p2);
    }
};

template <typename>
struct KCall;

template <class R, typename... Ts>
struct KCall<R(Ts...)> : public KThreadPool::WorkItem
{
    typedef KDelegate<R(Ts...)> _DelegateType;
    virtual void Execute()
    {
        _tuple.apply(_Delegate);
        _delete(this);
    }
    KCall(const _DelegateType& delegate, const Ts&... args) : _Delegate(delegate), _tuple(args...)
    {
    }
    _DelegateType _Delegate;
    Tuple<Ts...> _tuple;
};

//
//  KDeferredJump
//
//  Base template
//
template <typename>
class KDeferredJump;

template <class R, typename... Ts>
class KDeferredJump<R(Ts...)>
{
    typedef KDelegate<R(Ts...)> _DelegateType;

    K_DENY_COPY(KDeferredJump);

public:
    KDeferredJump(
        __in R(*pfn)(Ts...),
        __in KAllocator& Alloc
        )
        : _Allocator(Alloc)
    {
        _Delegate = pfn;
    }

    template <class OBJ>
    KDeferredJump(
        __in OBJ& Obj,
        __in R(OBJ::*pfn)(Ts...),
        __in KAllocator& Alloc
        )
        : _Allocator(Alloc)
    {
        _Delegate.Bind(&Obj, pfn);
    }

    NTSTATUS operator()(const Ts&... args)
    {
        auto _Tmp = _new(KTL_TAG_DEFERRED_CALL, _Allocator) KCall<R(Ts...)>(_Delegate, args...);
        if (!_Tmp)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        _Allocator.GetKtlSystem().DefaultThreadPool().QueueWorkItem(*_Tmp);
        return STATUS_SUCCESS;
    }

private:
    _DelegateType _Delegate;
    KAllocator& _Allocator;
};


//
//  KDeferredCall
//
//  Base template
//
template <typename>
class KDeferredCall;

//
//  Specialization for non-void signature R(...)
//
template <class R, typename... Ts>
class KDeferredCall<R(Ts...)> : public KThreadPool::WorkItem
{
    K_DENY_COPY(KDeferredCall);

public:
    KDeferredCall(__in KtlSystem& Sys) : _System(Sys) {}

    void Bind(__in R(*pfn)(Ts...))
    {
        _Delegate = pfn;
    }

    template <class OBJ>
    void Bind(
        __in OBJ& Obj,
        __in R(OBJ::*pfn)(Ts...)
        )
    {
        _Delegate.Bind(&Obj, pfn);
    }

    void operator()(const Ts&... args)
    {
        _tuple.set(args...);
        _System.DefaultThreadPool().QueueWorkItem(*this);
    }

    R& ReturnValue()
    {
        return _R;
    }

private:
    virtual void Execute()
    {
        _R = _tuple.apply(_Delegate);
    }

    KtlSystem& _System;
    KDelegate<R(Ts...)> _Delegate;
    Tuple<Ts...> _tuple;
    R _R;
};

//
//  Specialization for signature void(...)
//
template <typename... Ts>
class KDeferredCall<void(Ts...)> : public KThreadPool::WorkItem
{
    K_DENY_COPY(KDeferredCall);

public:
    KDeferredCall(__in KtlSystem& Sys) : _System(Sys) {}

    void Bind( __in void(*pfn)(Ts...))
    {
        _Delegate = pfn;
    }

    template <class OBJ>
    void Bind(
        __in OBJ& Obj,
        __in void(OBJ::*pfn)(Ts...)
        )
    {
        _Delegate.Bind(&Obj, pfn);
    }

    void operator()(const Ts&... args)
    {
        _tuple.set(args...);
        _System.DefaultThreadPool().QueueWorkItem(*this);
    }

private:
    virtual void Execute()
    {
        _tuple.apply(_Delegate);
    }

    KtlSystem& _System;
    KDelegate<void(Ts...)> _Delegate;
    Tuple<Ts...> _tuple;
};
