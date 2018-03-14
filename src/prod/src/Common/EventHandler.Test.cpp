// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "Common/Event.h"

using namespace std;

Common::StringLiteral const TraceType("EventHandlerTest");

namespace Common
{
    class TestEventHandler
    {
    protected:
        class MyEventArgs : public EventArgs
        {
        public:
            MyEventArgs(int value) : value_(value) {}

            int value_;
        };

        class MyEventSource
        {
        public:
            Event event0_;
            EventT<MyEventArgs> event2_;

            MyEventSource() {}

            void FireEvent0()
            {
                event0_.Fire(EventArgs(), true);
            }

            void FireEvent2()
            {
                event2_.Fire(MyEventArgs(2), true);
            }
        };

        class MyEventListner
        {
        public:
            static volatile LONG count_;
            static ManualResetEvent completeEvent_;

            MyEventListner(){}

            static void ReceivedEvent0(EventArgs const& args)
            {
                args;
                Trace.WriteInfo(TraceType, "Received event 0 by 0");
                AddCount();
            }

            static void ReceivedEvent2(MyEventArgs const& args)
            {
                Trace.WriteInfo(TraceType, "Received event 2 value {0}", args.value_);
                AddCount();
            }

            static void ReceivedEvent0_1(EventArgs const& args)
            {
                args;
                Trace.WriteInfo(TraceType, "Received event 0 by 1");
                AddCount();
            }

            static void ReceivedEvent0_2(EventArgs const& args)
            {
                args;
                Trace.WriteInfo(TraceType, "Received event 0 by 2");
                AddCount();
            }

            static void AddCount()
            {
                InterlockedIncrement(&count_);
                if (count_ >= 3)
                {
                    completeEvent_.Set();
                }
            }
        };
    };

    volatile LONG TestEventHandler::MyEventListner::count_;
    ManualResetEvent TestEventHandler::MyEventListner::completeEvent_;

    BOOST_FIXTURE_TEST_SUITE(TestEventHandlerSuite,TestEventHandler)

    BOOST_AUTO_TEST_CASE(BasicEventHandlerTest)
    {
        {
            MyEventListner::count_ = 0;
            MyEventListner::completeEvent_.Reset();

            MyEventSource source;

            Trace.WriteInfo(TraceType, "Firing event before register");

            source.FireEvent0();
            source.FireEvent2();

            auto h0 = source.event0_.Add(&MyEventListner::ReceivedEvent0);
            auto h1 = source.event0_.Add(&MyEventListner::ReceivedEvent0);
            auto h2 = source.event2_.Add(&MyEventListner::ReceivedEvent2);

            Trace.WriteInfo(TraceType, "Firing event after register");

            source.FireEvent0();
            source.FireEvent2();

            source.event0_.Remove(h0);
            source.event0_.Remove(h1);
            source.event2_.Remove(h2);
            Trace.WriteInfo(TraceType, "Firing event after unregister");

            source.FireEvent0();
            source.FireEvent2();
        }

        MyEventListner::completeEvent_.WaitOne();
    }

    BOOST_AUTO_TEST_CASE(MultipleEventHandlerTest)
    {
        {
            MyEventListner::count_ = 0;
            MyEventListner::completeEvent_.Reset();

            MyEventSource source;

            auto h0 = source.event0_.Add(&MyEventListner::ReceivedEvent0);
            auto h1 = source.event0_.Add(&MyEventListner::ReceivedEvent0);
            VERIFY_IS_TRUE(source.event0_.HandlerCount == 2);
            source.event0_.Remove(h0);
            VERIFY_IS_TRUE(source.event0_.HandlerCount == 1);
            source.event0_.Remove(h1);
            VERIFY_IS_TRUE(source.event0_.HandlerCount == 0);

            source.event0_.Add(&MyEventListner::ReceivedEvent0);
            source.event0_.Add(&MyEventListner::ReceivedEvent0_1);
            source.event0_.Add(&MyEventListner::ReceivedEvent0_2);
            source.event0_.Add(nullptr);

            source.FireEvent0();
            source.event0_.Close();
        }

        MyEventListner::completeEvent_.WaitOne();
    }

    BOOST_AUTO_TEST_SUITE_END()
}
