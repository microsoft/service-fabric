// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "Common/Atomics.h"

namespace Common
{
    class TestAtomics
    {
    };

    BOOST_FIXTURE_TEST_SUITE(TestAtomicsSuite,TestAtomics)

    BOOST_AUTO_TEST_CASE(Atomic64)
    {
        atomic_uint64 p;

        VERIFY_IS_TRUE( (p |= 1) == 1 );
        VERIFY_IS_TRUE( (p |= 1) == 1 );

        VERIFY_IS_TRUE( (p |= 2) == 3 );
        VERIFY_IS_TRUE( (p |= 2) == 3 );

        VERIFY_IS_TRUE( (p |= 4) == 7 );
        VERIFY_IS_TRUE( (p |= 4) == 7 );

        VERIFY_IS_TRUE( (p &= ~2u) == 5 );
        VERIFY_IS_TRUE( (p &= ~2u) == 5 );

        unsigned __int64 xor2 = p ^= 2; VERIFY_IS_TRUE(xor2 == 7);
        xor2 = p ^= 2; VERIFY_IS_TRUE(xor2 == 5);
    }

    struct MyClass {};

    BOOST_AUTO_TEST_CASE(AtomicPointerTest)
    {
        MyClass a;
        MyClass b;
        atomic<MyClass*> p;
        atomic<MyClass*> q(&a);

        VERIFY_IS_TRUE(p.load() == nullptr);
        VERIFY_IS_TRUE(q.load() != nullptr);
        VERIFY_IS_TRUE(p.load() != q.load());

        p.store(&b);
        MyClass * ret = p.exchange(nullptr);
        VERIFY_IS_TRUE(ret == &b);
        VERIFY_IS_TRUE(p.load() == nullptr); 
    }

    BOOST_AUTO_TEST_CASE(AtomicFlagTest)
    {
        atomic_flag f;

        VERIFY_IS_TRUE( f.test_and_set() == false );
        VERIFY_IS_TRUE( f.test_and_set() == true );
        VERIFY_IS_TRUE( f.test_and_set() == true );

        f.clear();

        VERIFY_IS_TRUE( f.test_and_set() == false );
    }

    BOOST_AUTO_TEST_CASE(AtomicLongTest)
    {
        atomic_long p;
        atomic_long q(15);

        VERIFY_IS_TRUE(p.load() == 0);
        VERIFY_IS_TRUE(q.load() == 15);

        VERIFY_IS_TRUE(++p == 1);
        VERIFY_IS_TRUE(++p == 2);
        VERIFY_IS_TRUE(p++ == 2);
        VERIFY_IS_TRUE(p++ == 3);
        VERIFY_IS_TRUE(p.load() == 4);

        p.exchange(15);

        VERIFY_IS_TRUE(p.load() == 15);

        LONG expected = 15;

        VERIFY_IS_TRUE(p.compare_exchange_weak(expected, 30));
        VERIFY_IS_TRUE(p.load() == 30);
        VERIFY_IS_TRUE(expected == 15);

        expected = 60;

        VERIFY_IS_FALSE(p.compare_exchange_weak(expected, 20));
        VERIFY_IS_TRUE(p.load() == 30);
        VERIFY_IS_TRUE(expected == 30);

        p += 20;

        VERIFY_IS_TRUE(p.load() == 50);

        p -= 30;

        VERIFY_IS_TRUE(p.load() == 20);

        atomic_long enumFlags(0);
        VERIFY_IS_TRUE(enumFlags.load() == 0);
    }

    BOOST_AUTO_TEST_CASE(AtomicFetchTest)
    {
        // Verify fetch_add with atomic_long
        atomic_long p(0);
        long q;
        q = p.fetch_add(100);
        VERIFY_IS_TRUE(q == 0);
        VERIFY_IS_TRUE(p.load() == 100);

        // Verify fetch_add with negative atomic_long
        p.store(-1);
        q = 0;
        q = p.fetch_add(10);
        VERIFY_IS_TRUE(q == -1);
        VERIFY_IS_TRUE(p.load() == 9)

        // Verify fetch_sub with atomic_long
        atomic_long r(10);
        long s;
        s = r.fetch_sub(100);
        VERIFY_IS_TRUE(s == 10);
        VERIFY_IS_TRUE(r.load() == -90);

        // Verify fetch_sub with negative atomic long
        r.store(-1);
        s = 0;
        s = r.fetch_sub(10);
        VERIFY_IS_TRUE(s == -1);
        VERIFY_IS_TRUE(r.load() == -11);

        // Verify fetch_add with atomic_uint64
        atomic_uint64 t(0);
        uint64 u;
        u = t.fetch_add(100);
        VERIFY_IS_TRUE(u == 0);
        VERIFY_IS_TRUE(t.load() == 100);

        // Verify fetch_sub with atomic_uint64
        atomic_uint64 v(100);
        uint64 w;
        w = v.fetch_sub(100);
        VERIFY_IS_TRUE(w == 100);
        VERIFY_IS_TRUE(v.load() == 0);

        // Verify fetch_sub with negative atomic_uint64
        v.store(10);
        w = 0;
        w = v.fetch_sub(100);
        VERIFY_IS_TRUE(w == 10);

        // Confirm overflow when setting an unsigned integer to a value below zero
        VERIFY_IS_TRUE(v.load() == (std::numeric_limits<uint64>::max() - 89));
    }

    BOOST_AUTO_TEST_SUITE_END()
}
