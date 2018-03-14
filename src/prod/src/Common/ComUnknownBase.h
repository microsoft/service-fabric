// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class ComUnknownBase
    {
        DENY_COPY(ComUnknownBase);

    protected:
        ComUnknownBase();
        virtual ~ComUnknownBase();

        ULONG STDMETHODCALLTYPE BaseAddRef(void);
        ULONG STDMETHODCALLTYPE BaseRelease(void);

        // Adds a reference if the current ref-count is >0 and is no-op if the ref-count is 0.
        // This can be safely called concurrently with the COM object destructor.
        //
        // Returns the current ref-count, which will be 0 iff the ref-count was already 0.
        // If this returns 0 the caller MUST NOT call Release, and if this returns non-zero
        // the caller MUST call Release.
        //
        // This enables a COM object to fire events safely where the event state includes a reference
        // to the COM object.  The COM object needs to be allowed to go to 0 ref count, but at the
        // same time when a new event is getting ready to fire, it needs to add a ref count.  To do
        // this safely, two guarantees must be met:
        // 1) The destructor of the COM object must coordinate to ensure that no more events attempt
        //    to use the COM object after the destructor has run.
        // 2) Events must avoid adding a reference if the ref count is already 0.
        // This method helps guarantee #2.  It is up to the COM object implementation to guarantee #1.
        ULONG STDMETHODCALLTYPE BaseTryAddRef(void);

    private:
        bool isShared_;
        LONG refCount_;
    };

    //
    // Overloads of make_com to create a ComPointer to a new COM object.
    //
    template <class TComImplementation>
    ComPointer<TComImplementation> make_com()
    {
        ComPointer<TComImplementation> p;
        p.SetNoAddRef(new TComImplementation());
        return p;
    }

    template <class TComImplementation,class T0>
    ComPointer<TComImplementation> make_com(T0 && a0)
    {
        ComPointer<TComImplementation> p;
        p.SetNoAddRef(new TComImplementation(std::forward<T0>(a0)));
        return p;
    }

    template <class TComImplementation,class T0,class T1>
    ComPointer<TComImplementation> make_com(T0 && a0, T1 && a1)
    {
        ComPointer<TComImplementation> p;
        p.SetNoAddRef(new TComImplementation(
            std::forward<T0>(a0), std::forward<T1>(a1)));
        return p;
    }

    template <class TComImplementation,class T0,class T1,class T2>
    ComPointer<TComImplementation> make_com(T0 && a0, T1 && a1, T2 && a2)
    {
        ComPointer<TComImplementation> p;
        p.SetNoAddRef(new TComImplementation(
            std::forward<T0>(a0), std::forward<T1>(a1), std::forward<T2>(a2)));
        return p;
    }

    template <class TComImplementation,class T0,class T1,class T2,class T3>
    ComPointer<TComImplementation> make_com(T0 && a0, T1 && a1, T2 && a2, T3 && a3)
    {
        ComPointer<TComImplementation> p;
        p.SetNoAddRef(new TComImplementation(
            std::forward<T0>(a0), std::forward<T1>(a1), std::forward<T2>(a2), std::forward<T3>(a3)));
        return p;
    }

    template <class TComImplementation,class T0,class T1,class T2,class T3,class T4>
    ComPointer<TComImplementation> make_com(T0 && a0, T1 && a1, T2 && a2, T3 && a3, T4 && a4)
    {
        ComPointer<TComImplementation> p;
        p.SetNoAddRef(new TComImplementation(
            std::forward<T0>(a0), std::forward<T1>(a1), std::forward<T2>(a2), std::forward<T3>(a3),
            std::forward<T4>(a4)));
        return p;
    }

    template <class TComImplementation,class T0,class T1,class T2,class T3,class T4,class T5>
    ComPointer<TComImplementation> make_com(T0 && a0, T1 && a1, T2 && a2, T3 && a3, T4 && a4, T5 && a5)
    {
        ComPointer<TComImplementation> p;
        p.SetNoAddRef(new TComImplementation(
            std::forward<T0>(a0), std::forward<T1>(a1), std::forward<T2>(a2), std::forward<T3>(a3),
            std::forward<T4>(a4), std::forward<T5>(a5)));
        return p;
    }

    template <class TComImplementation,class T0,class T1,class T2,class T3,class T4,class T5,class T6>
    ComPointer<TComImplementation> make_com(T0 && a0, T1 && a1, T2 && a2, T3 && a3, T4 && a4, T5 && a5, T6 && a6)
    {
        ComPointer<TComImplementation> p;
        p.SetNoAddRef(new TComImplementation(
            std::forward<T0>(a0), std::forward<T1>(a1), std::forward<T2>(a2), std::forward<T3>(a3),
            std::forward<T4>(a4), std::forward<T5>(a5), std::forward<T6>(a6)));
        return p;
    }

    //
    // Overloads of make_com to create a ComPointer to a new COM object, but
    // these return a ComPointer of an interface of the object.
    //
    template <class TComImplementation,class ComInterface>
    ComPointer<ComInterface> make_com()
    {
        ComPointer<ComInterface> p;
        p.SetNoAddRef(new TComImplementation());
        return p;
    }

    template <class TComImplementation,class ComInterface,class T0>
    ComPointer<ComInterface> make_com(T0 && a0)
    {
        ComPointer<ComInterface> p;
        p.SetNoAddRef(new TComImplementation(std::forward<T0>(a0)));
        return p;
    }

    template <class TComImplementation,class ComInterface,class T0,class T1>
    ComPointer<ComInterface> make_com(T0 && a0, T1 && a1)
    {
        ComPointer<ComInterface> p;
        p.SetNoAddRef(new TComImplementation(
            std::forward<T0>(a0), std::forward<T1>(a1)));
        return p;
    }

    template <class TComImplementation,class ComInterface,class T0,class T1,class T2>
    ComPointer<ComInterface> make_com(T0 && a0, T1 && a1, T2 && a2)
    {
        ComPointer<ComInterface> p;
        p.SetNoAddRef(new TComImplementation(
            std::forward<T0>(a0), std::forward<T1>(a1), std::forward<T2>(a2)));
        return p;
    }

    template <class TComImplementation,class ComInterface,class T0,class T1,class T2,class T3>
    ComPointer<ComInterface> make_com(T0 && a0, T1 && a1, T2 && a2, T3 && a3)
    {
        ComPointer<ComInterface> p;
        p.SetNoAddRef(new TComImplementation(
            std::forward<T0>(a0), std::forward<T1>(a1), std::forward<T2>(a2), std::forward<T3>(a3)));
        return p;
    }

    template <class TComImplementation,class ComInterface,class T0,class T1,class T2,class T3,class T4>
    ComPointer<ComInterface> make_com(T0 && a0, T1 && a1, T2 && a2, T3 && a3, T4 && a4)
    {
        ComPointer<ComInterface> p;
        p.SetNoAddRef(new TComImplementation(
            std::forward<T0>(a0), std::forward<T1>(a1), std::forward<T2>(a2), std::forward<T3>(a3),
            std::forward<T4>(a4)));
        return p;
    }

    template <class TComImplementation,class ComInterface,class T0,class T1,class T2,class T3,class T4, class T5>
    ComPointer<ComInterface> make_com(T0 && a0, T1 && a1, T2 && a2, T3 && a3, T4 && a4, T5 && a5)
    {
        ComPointer<ComInterface> p;
        p.SetNoAddRef(new TComImplementation(
            std::forward<T0>(a0), std::forward<T1>(a1), std::forward<T2>(a2), std::forward<T3>(a3),
            std::forward<T4>(a4), std::forward<T5>(a5)));
        return p;
    }

    template <class TComImplementation, class ComInterface, class T0, class T1, class T2, class T3, class T4, class T5, class T6>
    ComPointer<ComInterface> make_com(T0 && a0, T1 && a1, T2 && a2, T3 && a3, T4 && a4, T5 && a5, T6 && a6)
    {
        ComPointer<ComInterface> p;
        p.SetNoAddRef(new TComImplementation(
            std::forward<T0>(a0), std::forward<T1>(a1), std::forward<T2>(a2), std::forward<T3>(a3),
            std::forward<T4>(a4), std::forward<T5>(a5), std::forward<T6>(a6)));
        return p;
    }

}

//
// A set of macros to define the list of interfaces supported by QueryInterface.  Typical usage
// goes at the beginning of a class (before the first access specifier) and lists out all the
// interfaces.  IUnknown should typically be first.
//
// BEGIN_COM_INTERFACE_LIST must come first, and specifies the implementation class.
//
// Each COM_INTERFACE_ITEM specifies an IID and a type that either is or uniquely derives from the
// corresponding interface type.  For example, in cases where a class implements multiple
// interfaces, each of which extends IUnknown, the IID_IUnknown entry needs to specify exactly one
// of the interfaces to choose which IUnknown pointer is returned for IID_IUnknown.
//
// COM_BREAK_IN_DEBUGGER_INTERFACE_ITEM behaves similarly to COM_INTERFACE_ITEM, except it calls
// DebugBreak when someone queries for the specified interface before returning it.
//
// END_COM_INTERFACE_LIST must come after the list of items.
//
// Here is an example:
//
//    class ComOrange : public IOrange, public IFruit, public IColor, private ComUnknownBase
//    {
//        DENY_COPY(ComOrange)
//
//        BEGIN_COM_INTERFACE_LIST(ComOrange)
//        COM_INTERFACE_ITEM(IID_IUnknown, IOrange)
//        COM_INTERFACE_ITEM(IID_IOrange, IOrange)
//        COM_INTERFACE_ITEM(IID_IFruit, IFruit)
//        COM_INTERFACE_ITEM(IID_IColor, IColor)
//        END_COM_INTERFACE_LIST()
//
//    public:
//        ...
//
#define BEGIN_COM_INTERFACE_LIST(TClass)                       \
public:                                                        \
    STDMETHOD_(ULONG, AddRef)(void) { return BaseAddRef(); }   \
    STDMETHOD_(ULONG, Release)(void) { return BaseRelease(); } \
    STDMETHOD(QueryInterface)(REFIID riid, void ** result)     \
    {                                                          \
        if (!result) { return Common::ComUtility::OnPublicApiReturn(E_POINTER); }

#define COM_INTERFACE_ITEM(iid, TInterface) \
        if (riid == (iid)) { *result = static_cast<TInterface*>(this); } else

#define COM_BREAK_IN_DEBUGGER_INTERFACE_ITEM(iid, TInterface) \
        if (riid == (iid)) { *result = static_cast<TInterface*>(this); DebugBreak(); } else

#define COM_DELEGATE_TO_BASE_ITEM(TBase) \
        if (SUCCEEDED(this->TBase::QueryInterface(riid, result))) { return Common::ComUtility::OnPublicApiReturn(S_OK); } else

#define COM_DELEGATE_TO_METHOD(TMethod) \
        if (SUCCEEDED(TMethod(riid, result))) { return Common::ComUtility::OnPublicApiReturn(S_OK); } else

#define END_COM_INTERFACE_LIST()                                         \
        { *result = NULL; }                                              \
                                                                         \
        if (*result)                                                     \
        {                                                                \
            BaseAddRef();                                                \
            return Common::ComUtility::OnPublicApiReturn(S_OK);          \
        }                                                                \
        else                                                             \
        {                                                                \
            return Common::ComUtility::OnPublicApiReturn(E_NOINTERFACE); \
        }                                                                \
    }

//
// Shortcut macro to define an implementation of IUnknown with only one specialized interface.
//
//   TClass     - name of the class in which this macro is used
//   iid        - IID of TInterface
//   TInterface - name of the COM interface this class exposes
//
#define COM_INTERFACE_LIST1(TClass,iid,TInterface) \
    BEGIN_COM_INTERFACE_LIST(TClass)               \
    COM_INTERFACE_ITEM(IID_IUnknown,IUnknown)      \
    COM_INTERFACE_ITEM((iid),TInterface)           \
    END_COM_INTERFACE_LIST()

//
// Shortcut macro to define an implementation of IUnknown with one specialized interface and
// delegate the rest to TBase's QueryInterface.
//
//   TClass     - name of the class in which this macro is used
//   iid        - IID of TInterface
//   TInterface - name of the COM interface this class exposes
//   TBase      - name of the base class to delegate remaining interfaces
//
#define COM_INTERFACE_AND_DELEGATE_LIST(TClass,iid,TInterface,TBase) \
    BEGIN_COM_INTERFACE_LIST(TClass)                                 \
    COM_INTERFACE_ITEM(IID_IUnknown,TInterface)                      \
    COM_INTERFACE_ITEM((iid),TInterface)                             \
    COM_DELEGATE_TO_BASE_ITEM(TBase)                                 \
    END_COM_INTERFACE_LIST()

//
// Shortcut macro to define an implemenation of IUnknown with two specialized interfaces.  If
// ambiguous, the object's IUnknown comes from the last interface provided.
//
//   TClass      - name of the class in which this macro is used
//   iid0        - IID of TInterface0
//   TInterface0 - name of the first COM interface this class exposes
//   iid1        - IID of TInterface1
//   TInterface1 - name of the second COM interface this class exposes,
//                 and the type to cast to to disambiguate IUnknown if
//                 TClass derives from more than one IUnknown.
//
#define COM_INTERFACE_LIST2(TClass,iid0,TInterface0,iid1,TInterface1) \
    BEGIN_COM_INTERFACE_LIST(TClass)                                  \
    COM_INTERFACE_ITEM(IID_IUnknown,IUnknown)                         \
    COM_INTERFACE_ITEM((iid0),TInterface0)                            \
    COM_INTERFACE_ITEM((iid1),TInterface1)                            \
    END_COM_INTERFACE_LIST()
