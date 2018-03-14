// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "Common/ComPointer.h"
#include "Common/AssertWF.h"

namespace Common
{
    GUID IID_ITest1 = { 0x523db99e, 0xa874, 0x4da2, 0x8d, 0x00, 0x2b, 0x34, 0x1f, 0x3a, 0x1d, 0x24 };
    GUID IID_ITest2 = { 0x80080ad5, 0xe765, 0x4789, 0xbc, 0x76, 0x5e, 0x18, 0x76, 0x87, 0xdc, 0xe9 };

    class ITest1 : public IUnknown { public: STDMETHOD_(int,Method1)() = 0; };
    class ITest2 : public IUnknown { public: STDMETHOD_(int,Method2)() = 0; };
    class Component2;

    class Component1 : public ITest1
    {
        DENY_COPY(Component1);
    public:
        Component1() : refCount_(0), test2_(NULL) { }
        ~Component1() { CODING_ERROR_ASSERT(refCount_ == 0); }

        STDMETHOD_(ULONG, AddRef)() { return ++refCount_; }
        STDMETHOD_(ULONG, Release)() { return --refCount_; }
        STDMETHOD(QueryInterface)(__in REFIID riid, __out void ** p);
        STDMETHOD_(int,Method1)() { return 1; }

        ULONG refCount_;
        Component2 * test2_;
    };

    class Component2 : public ITest2
    {
        DENY_COPY(Component2);
    public:
        Component2(Component1 & test1) : refCount_(0), owner_(test1)
        {
            owner_.AddRef();
            owner_.test2_ = this;
        }
        ~Component2() { owner_.Release(); CODING_ERROR_ASSERT(refCount_ == 0); }

        STDMETHOD_(ULONG, AddRef)() { return ++refCount_; }
        STDMETHOD_(ULONG, Release)() { return --refCount_; }
        STDMETHOD(QueryInterface)(__in REFIID riid, __out void ** p) { return owner_.QueryInterface(riid, p); }
        STDMETHOD_(int,Method2)() { return 2; }

        ULONG refCount_;
        Component1 & owner_;
    };

    HRESULT Component1::QueryInterface(
        __in REFIID riid,
        __out void ** p)
    {
        IUnknown * result = NULL;

        /**/ if (riid == IID_IUnknown) { result = this; }
        else if (riid == IID_ITest1)   { result = this; }
        else if (riid == IID_ITest2)   { CODING_ERROR_ASSERT(test2_ != NULL); result = test2_; }

        *p = static_cast<void*>(result);
        if (result) { result->AddRef(); return S_OK; } else { return E_NOINTERFACE; }
    }

    class TestComPointer
    {
    protected:
        template <class T>
        void CheckNull(ComPointer<T> const & p) { CODING_ERROR_ASSERT(!p); }
        void CheckValid(ComPointer<IUnknown> const & p) { CODING_ERROR_ASSERT(p); p->AddRef(); p->Release(); }
        void CheckValid(ComPointer<ITest1> const & p) { CODING_ERROR_ASSERT(p); CODING_ERROR_ASSERT(p->Method1() == 1); }
        void CheckValid(ComPointer<ITest2> const & p) { CODING_ERROR_ASSERT(p); CODING_ERROR_ASSERT(p->Method2() == 2); }
        void CheckRefCount(Component1 const & c1, ULONG expected) { CODING_ERROR_ASSERT(c1.refCount_ == expected); }
        void CheckRefCount(Component2 const & c2, ULONG expected) { CODING_ERROR_ASSERT(c2.refCount_ == expected); }
    };


    BOOST_FIXTURE_TEST_SUITE(TestComPointerSuite,TestComPointer)

    BOOST_AUTO_TEST_CASE(CheckTestComponents)
    {
        Component1 c1;
        CheckRefCount(c1, 0);
        Component2 c2(c1);
        CheckRefCount(c1, 1);
        CheckRefCount(c2, 0);
    }
    BOOST_AUTO_TEST_CASE(EmptyConstructor)
    {
        ComPointer<ITest1> x;
        CheckNull(x);
    }
    BOOST_AUTO_TEST_CASE(SetAndAddRef)
    {
        Component1 c1;
        ComPointer<ITest1> x; x.SetAndAddRef(&c1);
        CheckRefCount(c1, 1);
        CheckValid(x);
    }
    BOOST_AUTO_TEST_CASE(SetNoAddRefAndDetachNoRelease)
    {
        Component1 c1;
        ComPointer<ITest1> x; x.SetNoAddRef(&c1);
        CheckRefCount(c1, 0);
        CheckValid(x);
        x.DetachNoRelease();
        CheckNull(x);
    }
    BOOST_AUTO_TEST_CASE(InitializationAddress)
    {
        Component1 c1;
        ComPointer<ITest1> x;
        c1.QueryInterface(IID_ITest1, reinterpret_cast<void**>(x.InitializationAddress()));
        CheckRefCount(c1, 1);
        CheckValid(x);
    }
    BOOST_AUTO_TEST_CASE(CopyConstructor)
    {
        Component1 c1;
        ComPointer<ITest1> x; x.SetAndAddRef(&c1);
        ComPointer<ITest1> y(x);
        CheckRefCount(c1, 2);
        CheckValid(x);
        CheckValid(y);
    }
    BOOST_AUTO_TEST_CASE(MoveConstructor)
    {
        Component1 c1;
        ComPointer<ITest1> x; x.SetAndAddRef(&c1);
        ComPointer<ITest1> y(std::move(x));
        CheckRefCount(c1, 1);
        CheckNull(x);
        CheckValid(y);
    }
    BOOST_AUTO_TEST_CASE(UnknownQIConstructor)
    {
        Component1 c1;
        ComPointer<ITest1> x; x.SetAndAddRef(&c1);
        ComPointer<IUnknown> y(x, IID_IUnknown);
        CheckRefCount(c1, 2);
        CheckValid(x);
        CheckValid(y);
    }
    BOOST_AUTO_TEST_CASE(Test1QIConstructor)
    {
        Component1 c1;
        ComPointer<ITest1> x; x.SetAndAddRef(&c1);
        ComPointer<ITest1> y(x, IID_ITest1);
        CheckRefCount(c1, 2);
        CheckValid(x);
        CheckValid(y);
    }
    BOOST_AUTO_TEST_CASE(Test2QIConstructor)
    {
        Component1 c1;
        Component2 c2(c1);
        ComPointer<ITest1> x; x.SetAndAddRef(&c1);
        ComPointer<ITest2> y(x, IID_ITest2);
        CheckRefCount(c1, 2);
        CheckRefCount(c2, 1);
        CheckValid(x);
        CheckValid(y);
    }
    BOOST_AUTO_TEST_CASE(Equality)
    {
        Component1 c1x;
        Component1 c1y;
        ComPointer<ITest1> xa; xa.SetAndAddRef(&c1x);
        ComPointer<ITest1> xb; xb.SetAndAddRef(&c1x);
        ComPointer<ITest1> y; y.SetAndAddRef(&c1y);
        CODING_ERROR_ASSERT(xa == xa);
        CODING_ERROR_ASSERT(!(xa != xa));
        CODING_ERROR_ASSERT(xa == xb);
        CODING_ERROR_ASSERT(!(xa != xb));
        CODING_ERROR_ASSERT(xa != y);
        CODING_ERROR_ASSERT(!(xa == y));
    }
    BOOST_AUTO_TEST_CASE(CopyAssignment)
    {
        Component1 c1x;
        Component1 c1y;
        ComPointer<ITest1> x; x.SetAndAddRef(&c1x);
        ComPointer<ITest1> y; y.SetAndAddRef(&c1y);
        ComPointer<ITest1> z;
        z = x;
        CheckRefCount(c1x, 2);
        CheckRefCount(c1y, 1);
        CheckValid(x);
        CheckValid(y);
        CheckValid(z);
        z = y;
        CheckRefCount(c1x, 1);
        CheckRefCount(c1y, 2);
        CheckValid(x);
        CheckValid(y);
        CheckValid(z);
    }
    BOOST_AUTO_TEST_CASE(MoveAssignment)
    {
        Component1 c1x;
        Component1 c1y;
        ComPointer<ITest1> x; x.SetAndAddRef(&c1x);
        ComPointer<ITest1> y; y.SetAndAddRef(&c1y);
        ComPointer<ITest1> z;
        z = std::move(x);
        CheckRefCount(c1x, 1);
        CheckRefCount(c1y, 1);
        CheckNull(x);
        CheckValid(y);
        CheckValid(z);
        z = std::move(y);
        CheckRefCount(c1x, 0);
        CheckRefCount(c1y, 1);
        CheckNull(x);
        CheckNull(y);
        CheckValid(z);
    }
    BOOST_AUTO_TEST_CASE(Swap)
    {
        Component1 c1;
        ComPointer<ITest1> x; x.SetAndAddRef(&c1);
        ComPointer<ITest1> y;
        x.Swap(y);
        CheckRefCount(c1, 1);
        CheckNull(x);
        CheckValid(y);
    }
    BOOST_AUTO_TEST_CASE(Release)
    {
        Component1 c1;
        ComPointer<ITest1> x; x.SetAndAddRef(&c1);
        x.Release();
        CheckRefCount(c1, 0);
        CheckNull(x);
    }
    BOOST_AUTO_TEST_CASE(SameComObject)
    {
        Component1 c1x;
        Component2 c2x(c1x);
        Component1 c1y;
        ComPointer<ITest1> x1; x1.SetAndAddRef(&c1x);
        ComPointer<ITest2> x2; x2.SetAndAddRef(&c2x);
        ComPointer<ITest1> y;  y.SetAndAddRef(&c1y);
        ComPointer<ITest1> empty1;
        ComPointer<ITest2> empty2;
        CODING_ERROR_ASSERT(empty1.IsSameComObject(empty1));
        CODING_ERROR_ASSERT(empty1.IsSameComObject(empty2));
        CODING_ERROR_ASSERT(!empty1.IsSameComObject(x1));
        CODING_ERROR_ASSERT(!x1.IsSameComObject(empty1));
        CODING_ERROR_ASSERT(x1.IsSameComObject(x1));
        CODING_ERROR_ASSERT(x2.IsSameComObject(x2));
        CODING_ERROR_ASSERT(x1.IsSameComObject(x2));
        CODING_ERROR_ASSERT(!y.IsSameComObject(x1));
        CODING_ERROR_ASSERT(!x1.IsSameComObject(y));
        CODING_ERROR_ASSERT(!y.IsSameComObject(x2));
        CODING_ERROR_ASSERT(!x2.IsSameComObject(y));
    }

    BOOST_AUTO_TEST_SUITE_END()
}
