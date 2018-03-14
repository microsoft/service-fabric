// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "Throw.h"
#include <memory>

Common::StringLiteral const TraceType("IntrusivePtrTest");

namespace Common
{
    class IntrusivePtrTest
    {
    protected:
        class simple
        {
            DENY_COPY(simple);
        public:
            simple () : ref_count_(1) {}
            virtual ~simple () 
            {
                //FN_ENTER();
            }

            friend void intrusive_ptr_add_ref( simple * p )
            {
                coding_error_assert(p);
                LONG result = InterlockedIncrement( &p->ref_count_ );
                //FN_ENTER(result);
                result;coding_error_assert(result > 1);
            }

            friend void intrusive_ptr_release( simple * p )
            {
                coding_error_assert(p);
                LONG result = InterlockedDecrement( &p->ref_count_ );
                //FN_ENTER(result);
                result;coding_error_assert(result >= 0);
                if (result == 0)
                    delete p;
            }
        private:
            volatile LONG ref_count_;
        };
    };

    BOOST_FIXTURE_TEST_SUITE(IntrusivePtrTestSuite,IntrusivePtrTest)

    BOOST_AUTO_TEST_CASE(SmokeTest)
    {
        auto p = make_intrusive<simple>();
        auto q = make_intrusive<basic_ref_counted>();
        auto r = std::make_shared<simple>();
        auto s = Common::make_unique<simple>();

        int N = 1000000;

        {
            std::vector<decltype(p)> ptrs;
            ptrs.reserve(N);
            Common::DateTime start = DateTime::Now();

            for (int i = 0; i < N; ++i)
            {
                ptrs.push_back( make_intrusive<simple>() );
            }

            ptrs.clear();

            Common::TimeSpan elapsed = DateTime::Now() - start;

            Trace.WriteInfo(TraceType, "simple {0} in {1}", N, elapsed);
        }

        {
            std::vector<decltype(q)> ptrs;
            ptrs.reserve(N);
            Common::DateTime start = DateTime::Now();

            for (int i = 0; i < N; ++i)
            {
                ptrs.push_back( make_intrusive<basic_ref_counted>() );
            }

            ptrs.clear();

            Common::TimeSpan elapsed = DateTime::Now() - start;

            Trace.WriteInfo(TraceType, "complex {0} in {1}", N, elapsed);
        }

        {
            std::vector<decltype(r)> ptrs;
            ptrs.reserve(N);
            Common::DateTime start = DateTime::Now();

            for (int i = 0; i < N; ++i)
            {
                ptrs.push_back( std::make_shared<simple>() );
            }

            ptrs.clear();

            Common::TimeSpan elapsed = DateTime::Now() - start;

            Trace.WriteInfo(TraceType, "shared {0} in {1}", N, elapsed);
        }

        {
            std::vector<decltype(s)> ptrs;
            ptrs.reserve(N);
            Common::DateTime start = DateTime::Now();

            for (int i = 0; i < N; ++i)
            {
                ptrs.push_back( Common::make_unique<simple>() );
            }

            ptrs.clear();

            Common::TimeSpan elapsed = DateTime::Now() - start;

            Trace.WriteInfo(TraceType, "unique {0} in {1}", N, elapsed);
        }

    }

    BOOST_AUTO_TEST_CASE(Nullptr_comparison)
    {
        std::shared_ptr<int> foo = std::make_shared<int>(5);

        coding_error_assert(foo != nullptr);
        coding_error_assert(nullptr != foo);
        VERIFY_IS_FALSE(foo == nullptr);
        VERIFY_IS_FALSE(foo == nullptr);

        coding_error_assert((foo == nullptr) == (!foo));
        coding_error_assert((foo != nullptr) == (!!foo));
    }

    BOOST_AUTO_TEST_SUITE_END()
}
