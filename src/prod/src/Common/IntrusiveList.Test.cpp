// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

Common::StringLiteral const TraceType("IntrusiveListTest");

namespace Common
{
    class IntrusiveListTest
    {
    protected:
        class Item : public LIST_ENTRY
        {
        public:
            Item(char value) : value_(value) {}

            char value() const { return value_; }

            void WriteTo(TextWriter& w, FormatOptions const&) const { w << value(); }

            intrusive::list_entry hook1;
            intrusive::list_entry hook2;

        private:
            char value_;
        };

        typedef intrusive::member_hook<Item, &Item::hook1> Item_Hook1;
        typedef intrusive::member_hook<Item, &Item::hook2> Item_Hook2;

    };

    BOOST_FIXTURE_TEST_SUITE(IntrusiveListTestSuite,IntrusiveListTest)

    BOOST_AUTO_TEST_CASE(SmokeTest)
    {
        intrusive::list<Item> x;
        intrusive::list<Item, Item_Hook1 > y;
        intrusive::list<Item, Item_Hook2 > z;

        Item a('a');
        Item b('b');
        Item c('c');

        x.push_back(&a);
        x.push_back(&b);
        x.push_back(&c);

        y.push_back(&c);
        y.push_back(&b);
        y.push_back(&a);

        z.push_back(&b);
        z.push_back(&a);
        z.push_back(&c);

        Trace.WriteInfo(TraceType, "{0}", x);
        Trace.WriteInfo(TraceType, "{0}", y);
        Trace.WriteInfo(TraceType, "{0}", z);
    }

    BOOST_AUTO_TEST_CASE(SwapTest)
    {
        {
            intrusive::list<Item> x;
            intrusive::list<Item> y;

            swap(x, y);
            Trace.WriteInfo(TraceType, "{0}", x);
            Trace.WriteInfo(TraceType, "{0}", y);
            VERIFY_IS_TRUE(x.empty());
            VERIFY_IS_TRUE(y.empty());
        }

        Item a('a');
        Item b('b');
        Item c('c');

        {
            intrusive::list<Item> x;
            intrusive::list<Item> y;

            x.push_back(&a);
            x.push_back(&b);
            x.push_back(&c);

            swap(x, y);
            Trace.WriteInfo(TraceType, "{0}", x);
            Trace.WriteInfo(TraceType, "{0}", y);
            VERIFY_IS_TRUE(x.empty());
            VERIFY_IS_TRUE(y.pop_front()->value() == 'a');
            VERIFY_IS_TRUE(y.pop_front()->value() == 'b');
            VERIFY_IS_TRUE(y.pop_front()->value() == 'c');
        }

        {
            intrusive::list<Item> x;
            intrusive::list<Item> y;

            y.push_back(&a);
            y.push_back(&b);
            y.push_back(&c);

            swap(x, y);
            Trace.WriteInfo(TraceType, "{0}", x);
            Trace.WriteInfo(TraceType, "{0}", y);
            VERIFY_IS_TRUE(y.empty());
            VERIFY_IS_TRUE(x.pop_front()->value() == 'a');
            VERIFY_IS_TRUE(x.pop_front()->value() == 'b');
            VERIFY_IS_TRUE(x.pop_front()->value() == 'c');
        }

        {
            intrusive::list<Item> x;
            intrusive::list<Item> y;

            x.push_back(&a);
            x.push_back(&b);
            y.push_back(&c);

            swap(x, y);
            Trace.WriteInfo(TraceType, "{0}", x);
            Trace.WriteInfo(TraceType, "{0}", y);
            VERIFY_IS_TRUE(y.pop_front()->value() == 'a');
            VERIFY_IS_TRUE(y.pop_front()->value() == 'b');
            VERIFY_IS_TRUE(x.pop_front()->value() == 'c');
        }
    }

    BOOST_AUTO_TEST_SUITE_END()
}
