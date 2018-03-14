// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "Common/Bique.h"
#include "Common/DateTime.h"

#include <deque>
#include <list>
#include <vector>
#include <map>
#include <numeric>

namespace Common
{
    using std::deque;
    using std::list;
    using std::max;
    using std::min;

    using std::move;
    using std::forward;

    using std::bind;
    using std::function;

    using std::rel_ops::operator<=;
    using std::rel_ops::operator>=;
    using std::rel_ops::operator!=;
    using std::rel_ops::operator>;

    using Common::DateTime;
    using Common::TimeSpan;
}

Common::StringLiteral const TraceType("BiqueTest");

namespace Common
{
    class TestBique
    {
    };

    BOOST_FIXTURE_TEST_SUITE(TestBiqueSuite,TestBique)

    BOOST_AUTO_TEST_CASE(SmokeTest)
    {
        bique_base<char> b(3);
        Trace.WriteInfo(TraceType, "{0}", b);

        b.reserve(5,9);

        Trace.WriteInfo(TraceType, "{0}", b);

        //Log.Info.WriteLine("unbeg() {0}", b.uninitialized_begin().fragment_size());

        //for (bique_base<char>::iterator p = b.uninitialized_begin(); p != b.uninitialized_end(); ++p)
        //{
        //    Log.Info.WriteLine("{0}: {1}", p.sequence(), p.fragment_size());
        //}

        //bool r = b.uninitialized_begin() + b.uninitialized_begin().fragment_size() <

        b.truncate_before(b.uninitialized_begin() + 3);

        Trace.WriteInfo(TraceType, "{0}", b);

        b.truncate_before(b.uninitialized_begin() + 3);

        Trace.WriteInfo(TraceType, "{0}", b);
    }

    BOOST_AUTO_TEST_CASE(QueueTest)
    {
        bique<char> b;

        b.push_back('a');
        b.push_back('b');
        b.push_back('c');

        Trace.WriteInfo(TraceType, "{0}", b);

        b.pop_front();

        Trace.WriteInfo(TraceType, "{0}", b);

        b.push_back('d');
        Trace.WriteInfo(TraceType, "{0}", b);
        b.push_back('e');
        b.push_back('f');

        Trace.WriteInfo(TraceType, "{0}", b);

        b.pop_front();
        Trace.WriteInfo(TraceType, "{0}", b);
    }

    BOOST_AUTO_TEST_CASE(BiqueMoveTest)
    {
        char data1[] = {'a', 'b', 'c', 'd', 'e', 'f', 'g'};
        bique<char> b1;
        size_t size1 = sizeof(data1)/sizeof(char);
        for (size_t i = 0; i < size1; ++i)
        {
            char ch = data1[i];
            b1.push_back(std::move(ch));
        }

        bique<char> b2(std::move(b1));
        Trace.WriteInfo(TraceType, "{0}", b1);
        Trace.WriteInfo(TraceType, "{0}", b2);
        VERIFY_IS_TRUE(b1.size() == 0);
        VERIFY_IS_TRUE(b2.size() == size1);

        // check if b1 is corrupted after moving
        char data2[] = {'i', 'j', 'k', 'l', 'm'};
        size_t size2 = sizeof(data2)/sizeof(char);
        for (size_t i = 0; i < size2; ++i)
        {
            char ch = data2[i];
            b1.push_back(std::move(ch));
        }
        Trace.WriteInfo(TraceType, "{0}", b1);
        Trace.WriteInfo(TraceType, "{0}", b2);
        VERIFY_IS_TRUE(b1.size() == size2);
        VERIFY_IS_TRUE(b2.size() == size1);

        bique<char> b3(std::move(b1));
        VERIFY_IS_TRUE(b1.size() == 0);
        VERIFY_IS_TRUE(b3.size() == size2);

        b1 = std::move(b2);
        Trace.WriteInfo(TraceType, "{0}", b1);
        Trace.WriteInfo(TraceType, "{0}", b2);
        VERIFY_IS_TRUE(b1.size() == size1);
        VERIFY_IS_TRUE(b2.size() == 0);

        std::vector<char> v(size1);
        pop_front_n(b1, size1, v.data());
        for (size_t i = 0; i < size1; ++i)
        {
            VERIFY_IS_TRUE(v[i] == data1[i]);
        }
    }

    BOOST_AUTO_TEST_CASE(BiqueRangeMoveTest)
    {
        bique<char> b;
        b.push_back('a');
        b.push_back('b');
        b.push_back('c');
        b.push_back('d');
        b.push_back('e');
        b.push_back('f');
        b.push_back('g');
        Trace.WriteInfo(TraceType, "{0}", b);

        BiqueRange<char> br(std::move(b));
        Trace.WriteInfo(TraceType, "{0}", b);
        for (auto iter = br.Begin; iter != br.End; ++ iter)
        {
            Trace.WriteInfo(TraceType, "{0}", *iter);
        }
    }

    class vqueue
    {
    public:
        typedef std::vector<int>::iterator iterator;
        typedef std::vector<int>::const_iterator const_iterator;

        explicit vqueue(size_t reserve) 
            : read_(0)
        {
            storage_.reserve(reserve);
        }
        template <class InputIterator>
        void insert(const_iterator position, InputIterator first, InputIterator last)
        {
            storage_.insert(position, first, last);
        }
        int front() const { return storage_[read_]; }
        void pop_front() { ++read_; }

        void erase(const_iterator first, const_iterator last)
        {
            read_ += last - first;
        }

        iterator begin() { return storage_.begin() + read_; }
        const_iterator end() const { return storage_.end(); }

        bool empty() const { return size() == 0; }
        size_t size() const { return storage_.size() - read_; }

    private:
        size_t read_;
        std::vector<int> storage_;
    };

    BOOST_AUTO_TEST_CASE(PerfTest)
    {
        static const size_t chunk = 100;
        size_t N = 10000000;
        size_t send = 0;
        size_t recv = 0;
        size_t max_size = 0;

        std::vector<int> v(N);
        std::vector<int> vo(N);
        std::iota(v.begin(), v.end(), 0);
        bique<int> b( 2048 );
        //vqueue b(N);
        //deque<int> b;
        //list<int> b;

        DateTime started = DateTime::Now();

        for(int iteration = 0;recv < N; ++iteration)
        {
            size_t available = min(chunk, N - send);

            //Log.Info.WriteLine("iteration {0} available {1}", iteration, available);

            if (available > 0)
            {
                b.insert(b.end(), &v[send], &v[send] + available);
                send += available;
            }

            if (b.size() > max_size)
                max_size = b.size();

            //Log.Info.WriteLine(b.size());

            if ( (iteration & 3) == 0 )
            {
                size_t available_inner = min(chunk - 1, b.size());

                pop_front_n(b, available_inner, &vo[recv]);

                recv += available_inner;
            }
        }
        VERIFY_IS_TRUE(recv == send);
        VERIFY_IS_TRUE(equal(v.begin(), v.end(), vo.begin()));

        TimeSpan elapsed = DateTime::Now() - started;

        Trace.WriteInfo(TraceType, "{0} pushes / pops in {1}, max size {2}", recv, elapsed, max_size);
    }

    typedef bique<int>::iterator IntBiqueIterator;

    class IntRangeInfo
    {
        DENY_COPY(IntRangeInfo);
    public:
        IntRangeInfo(IntRangeInfo && other)
            : Start(other.Start), Count(other.Count), Range(std::move(other.Range))
        { }

        IntRangeInfo(int start, int count, BiqueRange<int> && range)
            : Start(start), Count(count), Range(std::move(range))
        { }

        IntRangeInfo & operator = (IntRangeInfo && other)
        {
            Start = other.Start;
            Count = other.Count;
            Range = std::move(other.Range);

            return *this;
        }

        void swap(IntRangeInfo & other)
        {
            std::swap(Start, other.Start);
            std::swap(Count, other.Count);
            Range.swap(other.Range);
        }

        int Start;
        int Count;
        BiqueRange<int> Range;
    };

    BOOST_AUTO_TEST_CASE(BiqueRangeTest)
    {
        std::vector<IntRangeInfo> ranges;
        bique<int> bq;

        const int iterationLimit = 1000;
        const int pushBytes = 10;
        const int rangeStartOffset = 2;
        const int rangeSize = 5;
        const int steadyStateBiqueBytes = 100;
        const int steadyStateRangeCount = 100;

        int nextPushItem = 0;
        int nextPopItem = 0;

        for (int iteration = 0; iteration < iterationLimit; iteration++)
        {
            {for (int i=0; i<pushBytes; i++) { bq.push_back(nextPushItem++); }}
            {for (int i=0; i<rangeStartOffset; i++) { nextPopItem++; bq.pop_front(); }}

            ranges.emplace_back(
                IntRangeInfo(
                nextPopItem,
                rangeSize,
                BiqueRange<int>(bq.begin(), bq.begin()+rangeSize, true)));

            while (bq.end() - bq.begin() > steadyStateBiqueBytes)
            {
                nextPopItem++;
                bq.pop_front();
            }
            bq.truncate_before(bq.begin());

            // validate existing ranges
            {for (size_t rangeIndex = 0; rangeIndex < ranges.size(); rangeIndex++) {
                IntRangeInfo & r = ranges[rangeIndex];
                IntBiqueIterator bi = r.Range.Begin;
                for (int i=0; i<r.Count; i++)
                {
                    if (*bi != r.Start + i)
                    {
                        Trace.WriteInfo(TraceType,
                            "Mismatch: iteration={0} start={1} count={2} index={3} expected={4} actual={5}",
                            iteration, r.Start, r.Count, i, r.Start+i, *bi);
                        throw L"BiqueRangeTest: data stored in BiqueRange did not match expected.";
                    }
                    ++bi;
                }
            }}

            if (ranges.size() > steadyStateRangeCount)
            {
                int index = (iterationLimit - iteration) % ranges.size();
                ranges.erase(ranges.begin() + index);

                int index1 = (index + 1) % ranges.size();
                int index2 = (index + 2) % ranges.size();
                ranges[index1].swap(ranges[index2]);
            }
        }
    }

    BOOST_AUTO_TEST_SUITE_END()
}
