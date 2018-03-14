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

    StringLiteral const TraceComponent("EmbeddedListTest");

    class EmbeddedListTest
    {
    protected:
        typedef EmbeddedList<int> List;
        typedef EmbeddedListEntry<int> ListEntry;
        typedef shared_ptr<ListEntry> ListEntrySPtr;

        EmbeddedListTest() { BOOST_REQUIRE(TestcaseSetup()); }
        TEST_METHOD_SETUP(TestcaseSetup)

        void VerifyEmpty(List const &);
        ListEntrySPtr VerifySingleton(List const &, int expectedValue);
        ListEntrySPtr VerifyAndGetHead(List const &, int expectedSize, int expectedValue);
        ListEntrySPtr VerifyAndGetNextEntry(ListEntrySPtr const &, int expectedValue);
        void VerifyTail(List const &, ListEntrySPtr const &);
        void UpdateHead(List &, int value);
        void UpdateHead(List &, ListEntrySPtr const &);
        void UpdateHeadAndTrim(List &, int value, size_t limit);
        void UpdateHeadAndTrim(List &, ListEntrySPtr const &, size_t limit);
        void Remove(List &, ListEntrySPtr const &);
    };

    // Tests growing and shrinking the list.
    //
    BOOST_FIXTURE_TEST_SUITE(EmbeddedListTestSuite,EmbeddedListTest)

    BOOST_AUTO_TEST_CASE(AddRemoveTest)
    {
        Trace.WriteWarning(TraceComponent, "AddRemoveTest");

        EmbeddedList<int> list;

        VerifyEmpty(list);

        Trace.WriteInfo(TraceComponent, "Testcase: add/remove single entry");

        UpdateHead(list, 1);

        auto entry = VerifySingleton(list, 1);

        Remove(list, entry);

        VerifyEmpty(list);

        Trace.WriteInfo(TraceComponent, "Testcase: add 2 entries");

        UpdateHead(list, 2);
        UpdateHead(list, 3);

        entry = VerifyAndGetHead(list, 2, 3);
        entry = VerifyAndGetNextEntry(entry, 2);
        VerifyTail(list, entry);

        Trace.WriteInfo(TraceComponent, "Testcase: add 3rd entry");

        UpdateHead(list, 4);

        entry = VerifyAndGetHead(list, 3, 4);
        entry = VerifyAndGetNextEntry(entry, 3);
        entry = VerifyAndGetNextEntry(entry, 2);
        VerifyTail(list, entry);

        Trace.WriteInfo(TraceComponent, "Testcase: add 4th entry");

        UpdateHead(list, 5);

        entry = VerifyAndGetHead(list, 4, 5);
        entry = VerifyAndGetNextEntry(entry, 4);
        entry = VerifyAndGetNextEntry(entry, 3);
        entry = VerifyAndGetNextEntry(entry, 2);
        VerifyTail(list, entry);

        auto toMove = entry;

        Trace.WriteInfo(TraceComponent, "Testcase: move tail to head");

        UpdateHead(list, toMove);

        entry = VerifyAndGetHead(list, 4, 2);
        entry = VerifyAndGetNextEntry(entry, 5);
        toMove = entry = VerifyAndGetNextEntry(entry, 4);
        entry = VerifyAndGetNextEntry(entry, 3);
        VerifyTail(list, entry);

        Trace.WriteInfo(TraceComponent, "Testcase: move entry before tail to head");

        UpdateHead(list, toMove);

        entry = VerifyAndGetHead(list, 4, 4);
        toMove = entry = VerifyAndGetNextEntry(entry, 2);
        entry = VerifyAndGetNextEntry(entry, 5);
        entry = VerifyAndGetNextEntry(entry, 3);
        VerifyTail(list, entry);

        Trace.WriteInfo(TraceComponent, "Testcase: move entry after head to head (swap)");

        UpdateHead(list, toMove);

        toMove = entry = VerifyAndGetHead(list, 4, 2);
        entry = VerifyAndGetNextEntry(entry, 4);
        entry = VerifyAndGetNextEntry(entry, 5);
        entry = VerifyAndGetNextEntry(entry, 3);
        VerifyTail(list, entry);

        Trace.WriteInfo(TraceComponent, "Testcase: move head to itself (should be no-op)");

        UpdateHead(list, toMove);

        entry = VerifyAndGetHead(list, 4, 2);
        entry = VerifyAndGetNextEntry(entry, 4);
        entry = VerifyAndGetNextEntry(entry, 5);
        entry = VerifyAndGetNextEntry(entry, 3);
        VerifyTail(list, entry);

        Trace.WriteInfo(TraceComponent, "Testcase: add another entry to create a 'middle' entry");

        UpdateHead(list, 6);

        entry = VerifyAndGetHead(list, 5, 6);
        entry = VerifyAndGetNextEntry(entry, 2);
        auto toRemove = entry = VerifyAndGetNextEntry(entry, 4);
        entry = VerifyAndGetNextEntry(entry, 5);
        entry = VerifyAndGetNextEntry(entry, 3);
        VerifyTail(list, entry);

        Trace.WriteInfo(TraceComponent, "Testcase: remove middle entry");
        
        Remove(list, toRemove);

        entry = VerifyAndGetHead(list, 4, 6);
        entry = VerifyAndGetNextEntry(entry, 2);
        toRemove = entry = VerifyAndGetNextEntry(entry, 5);
        entry = VerifyAndGetNextEntry(entry, 3);
        VerifyTail(list, entry);

        Trace.WriteInfo(TraceComponent, "Testcase: remove entry before tail");
        
        Remove(list, toRemove);

        entry = VerifyAndGetHead(list, 3, 6);
        entry = VerifyAndGetNextEntry(entry, 2);
        entry = VerifyAndGetNextEntry(entry, 3);
        VerifyTail(list, entry);

        Trace.WriteInfo(TraceComponent, "Testcase: add another entry to create an entry after head");

        UpdateHead(list, 7);

        entry = VerifyAndGetHead(list, 4, 7);
        toRemove = entry = VerifyAndGetNextEntry(entry, 6);
        entry = VerifyAndGetNextEntry(entry, 2);
        entry = VerifyAndGetNextEntry(entry, 3);
        VerifyTail(list, entry);

        Trace.WriteInfo(TraceComponent, "Testcase: remove entry after head");

        Remove(list, toRemove);

        toRemove = entry = VerifyAndGetHead(list, 3, 7);
        entry = VerifyAndGetNextEntry(entry, 2);
        entry = VerifyAndGetNextEntry(entry, 3);
        VerifyTail(list, entry);

        Trace.WriteInfo(TraceComponent, "Testcase: remove head");

        Remove(list, toRemove);

        entry = VerifyAndGetHead(list, 2, 2);
        toRemove = entry = VerifyAndGetNextEntry(entry, 3);
        VerifyTail(list, entry);
        
        Trace.WriteInfo(TraceComponent, "Testcase: add another entry to create middle entry");

        UpdateHead(list, 8);

        entry = VerifyAndGetHead(list, 3, 8);
        entry = VerifyAndGetNextEntry(entry, 2);
        toRemove = entry = VerifyAndGetNextEntry(entry, 3);
        VerifyTail(list, entry);

        Trace.WriteInfo(TraceComponent, "Testcase: remove tail");

        Remove(list, toRemove);

        entry = VerifyAndGetHead(list, 2, 8);
        toRemove = entry = VerifyAndGetNextEntry(entry, 2);
        VerifyTail(list, entry);

        Trace.WriteInfo(TraceComponent, "Testcase: remove tail again");

        Remove(list, toRemove);

        toRemove = VerifySingleton(list, 8);
        
        Trace.WriteInfo(TraceComponent, "Testcase: remove last entry");

        Remove(list, toRemove);

        VerifyEmpty(list);
    }

    // Tests maintenance of list size limit
    //
    BOOST_AUTO_TEST_CASE(TrimTest)
    {
        Trace.WriteWarning(TraceComponent, "TrimTest");

        EmbeddedList<int> list;

        VerifyEmpty(list);

        Trace.WriteInfo(TraceComponent, "Testcase: initialize for trim test");

        UpdateHeadAndTrim(list, 1, 1);

        auto entry = VerifySingleton(list, 1);

        Trace.WriteInfo(TraceComponent, "Testcase: trim to singleton");

        UpdateHeadAndTrim(list, 2, 1);

        entry = VerifySingleton(list, 2);

        Trace.WriteInfo(TraceComponent, "Testcase: grow to 2");

        UpdateHeadAndTrim(list, 3, 2);

        entry = VerifyAndGetHead(list, 2, 3);
        entry = VerifyAndGetNextEntry(entry, 2);
        VerifyTail(list, entry);

        Trace.WriteInfo(TraceComponent, "Testcase: grow to 3");

        UpdateHeadAndTrim(list, 4, 3);

        entry = VerifyAndGetHead(list, 3, 4);
        entry = VerifyAndGetNextEntry(entry, 3);
        entry = VerifyAndGetNextEntry(entry, 2);
        VerifyTail(list, entry);

        Trace.WriteInfo(TraceComponent, "Testcase: grow to 5");

        UpdateHeadAndTrim(list, 5, 5);

        entry = VerifyAndGetHead(list, 4, 5);
        entry = VerifyAndGetNextEntry(entry, 4);
        entry = VerifyAndGetNextEntry(entry, 3);
        entry = VerifyAndGetNextEntry(entry, 2);
        VerifyTail(list, entry);

        UpdateHeadAndTrim(list, 6, 5);

        entry = VerifyAndGetHead(list, 5, 6);
        entry = VerifyAndGetNextEntry(entry, 5);
        entry = VerifyAndGetNextEntry(entry, 4);
        entry = VerifyAndGetNextEntry(entry, 3);
        entry = VerifyAndGetNextEntry(entry, 2);
        VerifyTail(list, entry);

        Trace.WriteInfo(TraceComponent, "Testcase: trim at 5");

        UpdateHeadAndTrim(list, 7, 5);

        entry = VerifyAndGetHead(list, 5, 7);
        entry = VerifyAndGetNextEntry(entry, 6);
        entry = VerifyAndGetNextEntry(entry, 5);
        entry = VerifyAndGetNextEntry(entry, 4);
        entry = VerifyAndGetNextEntry(entry, 3);
        VerifyTail(list, entry);

        UpdateHeadAndTrim(list, 8, 5);

        entry = VerifyAndGetHead(list, 5, 8);
        entry = VerifyAndGetNextEntry(entry, 7);
        entry = VerifyAndGetNextEntry(entry, 6);
        entry = VerifyAndGetNextEntry(entry, 5);
        auto toMove = entry = VerifyAndGetNextEntry(entry, 4);
        VerifyTail(list, entry);

        Trace.WriteInfo(TraceComponent, "Testcase: move tail and trim");

        UpdateHeadAndTrim(list, toMove, 5);

        entry = VerifyAndGetHead(list, 5, 4);
        entry = VerifyAndGetNextEntry(entry, 8);
        entry = VerifyAndGetNextEntry(entry, 7);
        entry = VerifyAndGetNextEntry(entry, 6);
        entry = VerifyAndGetNextEntry(entry, 5);
        VerifyTail(list, entry);

        UpdateHeadAndTrim(list, 9, 5);

        entry = VerifyAndGetHead(list, 5, 9);
        entry = VerifyAndGetNextEntry(entry, 4);
        entry = VerifyAndGetNextEntry(entry, 8);
        toMove = entry = VerifyAndGetNextEntry(entry, 7);
        entry = VerifyAndGetNextEntry(entry, 6);
        VerifyTail(list, entry);

        Trace.WriteInfo(TraceComponent, "Testcase: move entry before tail and trim");

        UpdateHeadAndTrim(list, toMove, 5);

        entry = VerifyAndGetHead(list, 5, 7);
        entry = VerifyAndGetNextEntry(entry, 9);
        entry = VerifyAndGetNextEntry(entry, 4);
        entry = VerifyAndGetNextEntry(entry, 8);
        entry = VerifyAndGetNextEntry(entry, 6);
        VerifyTail(list, entry);

        UpdateHeadAndTrim(list, 10, 5);

        entry = VerifyAndGetHead(list, 5, 10);
        entry = VerifyAndGetNextEntry(entry, 7);
        toMove = entry = VerifyAndGetNextEntry(entry, 9);
        entry = VerifyAndGetNextEntry(entry, 4);
        entry = VerifyAndGetNextEntry(entry, 8);
        VerifyTail(list, entry);

        Trace.WriteInfo(TraceComponent, "Testcase: move middle entry and trim");

        UpdateHeadAndTrim(list, toMove, 5);

        entry = VerifyAndGetHead(list, 5, 9);
        entry = VerifyAndGetNextEntry(entry, 10);
        entry = VerifyAndGetNextEntry(entry, 7);
        entry = VerifyAndGetNextEntry(entry, 4);
        entry = VerifyAndGetNextEntry(entry, 8);
        VerifyTail(list, entry);

        UpdateHeadAndTrim(list, 11, 5);

        entry = VerifyAndGetHead(list, 5, 11);
        toMove = entry = VerifyAndGetNextEntry(entry, 9);
        entry = VerifyAndGetNextEntry(entry, 10);
        entry = VerifyAndGetNextEntry(entry, 7);
        entry = VerifyAndGetNextEntry(entry, 4);
        VerifyTail(list, entry);

        Trace.WriteInfo(TraceComponent, "Testcase: move entry after head and trim");

        UpdateHeadAndTrim(list, toMove, 5);

        entry = VerifyAndGetHead(list, 5, 9);
        entry = VerifyAndGetNextEntry(entry, 11);
        entry = VerifyAndGetNextEntry(entry, 10);
        entry = VerifyAndGetNextEntry(entry, 7);
        entry = VerifyAndGetNextEntry(entry, 4);
        VerifyTail(list, entry);

        UpdateHeadAndTrim(list, 12, 5);

        toMove = entry = VerifyAndGetHead(list, 5, 12);
        entry = VerifyAndGetNextEntry(entry, 9);
        entry = VerifyAndGetNextEntry(entry, 11);
        entry = VerifyAndGetNextEntry(entry, 10);
        entry = VerifyAndGetNextEntry(entry, 7);
        VerifyTail(list, entry);

        Trace.WriteInfo(TraceComponent, "Testcase: move head and trim");

        UpdateHeadAndTrim(list, toMove, 5);

        entry = VerifyAndGetHead(list, 5, 12);
        entry = VerifyAndGetNextEntry(entry, 9);
        entry = VerifyAndGetNextEntry(entry, 11);
        entry = VerifyAndGetNextEntry(entry, 10);
        entry = VerifyAndGetNextEntry(entry, 7);
        VerifyTail(list, entry);

        UpdateHeadAndTrim(list, 13, 5);

        entry = VerifyAndGetHead(list, 5, 13);
        entry = VerifyAndGetNextEntry(entry, 12);
        entry = VerifyAndGetNextEntry(entry, 9);
        entry = VerifyAndGetNextEntry(entry, 11);
        entry = VerifyAndGetNextEntry(entry, 10);
        VerifyTail(list, entry);
    }

    BOOST_AUTO_TEST_SUITE_END()

    bool EmbeddedListTest::TestcaseSetup()
    {
        Config cfg;
        return true;
    }
    void EmbeddedListTest::VerifyEmpty(List const & list)
    {
        auto size = list.GetSize();
        VERIFY_IS_TRUE_FMT(size == 0, "size = {0}", size);

        auto entry = list.GetHead();
        VERIFY_IS_TRUE_FMT(!entry, "head = {0}", entry.get());

        auto tail = list.GetTail();
        VERIFY_IS_TRUE_FMT(!tail, "tail = {0}", tail.get());
    }
    EmbeddedListTest::ListEntrySPtr EmbeddedListTest::VerifySingleton(List const & list, int expectedValue)
    {
        auto size = list.GetSize();
        VERIFY_IS_TRUE_FMT(size == 1, "size = {0}", size);

        auto entry = list.GetHead();
        VERIFY_IS_TRUE_FMT(entry, "head = {0}", entry.get());

        auto tail = list.GetTail();
        VERIFY_IS_TRUE_FMT(tail == entry, "tail = {0}", tail.get());

        auto value = *(entry->GetListEntry());
        VERIFY_IS_TRUE_FMT(value == expectedValue, "value = {0}", value);

        VERIFY_IS_TRUE_FMT(!entry->GetPrev(), "prev = {0}", entry->GetPrev().get());
        VERIFY_IS_TRUE_FMT(!entry->GetNext(), "next = {0}", entry->GetNext().get());

        return entry;
    }
    EmbeddedListTest::ListEntrySPtr EmbeddedListTest::VerifyAndGetHead(EmbeddedListTest::List const & list, int expectedSize, int expectedValue)
    {
        auto size = list.GetSize();
        VERIFY_IS_TRUE_FMT(size == expectedSize, "size = {0}", size);

        auto entry = list.GetHead();
        VERIFY_IS_TRUE_FMT(entry, "entry = {0}", entry.get());

        auto value = *(entry->GetListEntry());
        VERIFY_IS_TRUE_FMT(value == expectedValue, "value = {0}", value);

        VERIFY_IS_TRUE_FMT(!entry->GetPrev(), "prev = {0}", entry->GetPrev().get());

        return entry;
    }
    EmbeddedListTest::ListEntrySPtr EmbeddedListTest::VerifyAndGetNextEntry(EmbeddedListTest::ListEntrySPtr const & entry, int expectedValue)
    {
        auto prev = entry;
        auto next = entry->GetNext();
        VERIFY_IS_TRUE_FMT(next, "next = {0}", next.get());
        VERIFY_IS_TRUE_FMT(prev == next->GetPrev(), "prev = {0}", next->GetPrev().get());

        auto value = *(next->GetListEntry());
        VERIFY_IS_TRUE_FMT(value == expectedValue, "value = {0}", value);

        return next;
    }
    void EmbeddedListTest::VerifyTail(List const & list, EmbeddedListTest::ListEntrySPtr const & entry)
    {
        auto tail = list.GetTail();
        VERIFY_IS_TRUE_FMT(tail == entry, "tail = {0}", tail.get());

        auto next = entry->GetNext();
        VERIFY_IS_TRUE_FMT(!next, "next = {0}", next.get());
    }
    void EmbeddedListTest::UpdateHead(List & list, int value)
    {
        list.UpdateHeadAndTrim(make_shared<ListEntry>(make_shared<int>(value)), 0);
    }
    void EmbeddedListTest::UpdateHead(List & list, EmbeddedListTest::ListEntrySPtr const & entry)
    {
        list.UpdateHeadAndTrim(entry, 0);
    }
    void EmbeddedListTest::UpdateHeadAndTrim(List & list, int value, size_t limit)
    {
        list.UpdateHeadAndTrim(make_shared<ListEntry>(make_shared<int>(value)), limit);
    }
    void EmbeddedListTest::UpdateHeadAndTrim(List & list, ListEntrySPtr const & entry, size_t limit)
    {
        list.UpdateHeadAndTrim(entry, limit);
    }
    void EmbeddedListTest::Remove(List & list, EmbeddedListTest::ListEntrySPtr const & entry)
    {
        list.RemoveFromList(entry);
    }
}
