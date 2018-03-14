// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace Common
{
    using namespace std;

    class TestMovePointer
    {
    };

    BOOST_FIXTURE_TEST_SUITE(TestMovePointerSuite,TestMovePointer)

    BOOST_AUTO_TEST_CASE(UPtr)
    {
        auto ptr = make_unique<int>(5);

        VERIFY_IS_TRUE(ptr);
        MoveUPtr<int> mover(std::move(ptr));
        VERIFY_IS_FALSE(ptr);
        VERIFY_IS_TRUE(mover.HasPtr());

        ManualResetEvent waitHandle(false);

        Threadpool::Post(
            [mover, &waitHandle]() mutable
            {
                VERIFY_IS_TRUE(mover.HasPtr());
                auto movedPtr = mover.TakeUPtr();
                VERIFY_IS_FALSE(mover.HasPtr());

                VERIFY_IS_TRUE(*movedPtr == 5);

                waitHandle.Set();
            });

        VERIFY_IS_FALSE(mover.HasPtr());
        waitHandle.WaitOne();
    }

    namespace 
    {
        class ComComponent : public IUnknown
        {
            DENY_COPY(ComComponent);
        public:
            explicit ComComponent(int count) : refCount_(0), count_(count) { }
            ~ComComponent() { CODING_ERROR_ASSERT(refCount_ == 0); }

            __declspec(property(get=get_Count)) int Count;
            int get_Count() const { return count_; }

            STDMETHOD_(ULONG, AddRef)() { return ++refCount_; }
            STDMETHOD_(ULONG, Release)() { return --refCount_; }
            STDMETHOD(QueryInterface)(__in REFIID, __out void ** p) 
            {
                AddRef();
                *p = static_cast<void*>(this);
                return S_OK; 
            }

        private:
            ULONG refCount_;
            int count_;
        };
    }

    BOOST_AUTO_TEST_CASE(CPtr)
    {
        auto ptr = make_com<ComComponent>(23);

        VERIFY_IS_TRUE(ptr);
        MoveCPtr<ComComponent> mover(std::move(ptr));
        VERIFY_IS_FALSE(ptr);
        VERIFY_IS_TRUE(mover.HasPtr());

        ManualResetEvent waitHandle(false);

        Threadpool::Post(
            [mover, &waitHandle]() mutable
            {
                VERIFY_IS_TRUE(mover.HasPtr());
                auto movedPtr = mover.TakeSPtr();
                VERIFY_IS_FALSE(mover.HasPtr());

                VERIFY_IS_TRUE(movedPtr->Count == 23);

                waitHandle.Set();
            });

        VERIFY_IS_FALSE(mover.HasPtr());
        waitHandle.WaitOne();
    }

    BOOST_AUTO_TEST_SUITE_END()
}
