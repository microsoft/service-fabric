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

    Common::StringLiteral const TraceType("ComponentRootTest");

    class ComponentRootTest :  TextTraceComponent<Common::TraceTaskCodes::Common>
    {
    };

    class MockComponentRoot : public ComponentRoot
    {
    public:
        ~MockComponentRoot() { }

        void SetTraceId(wstring const & value, size_t capacity) { ComponentRoot::SetTraceId(value, capacity); }
        bool TryPutReservedTraceId(wstring const & value) { return ComponentRoot::TryPutReservedTraceId(value); }
    };

    BOOST_FIXTURE_TEST_SUITE(ComponentRootTestSuite,ComponentRootTest)

    BOOST_AUTO_TEST_CASE(TraceIdTest)
    {
        Trace.WriteInfo(TraceType, "*** TraceIdTest");

        auto root = make_shared<MockComponentRoot>();

        // Reserve capacity: this will result in memory allocation
        
        wstring traceId1(L"TraceId1");
        size_t capacity = 32;
        root->SetTraceId(traceId1, capacity);

        size_t oldSize = root->TraceId.size();
        size_t oldCapacity = root->TraceId.capacity();
        void const * oldAddress = static_cast<void const *>(root->TraceId.c_str());

        VERIFY_IS_TRUE_FMT(traceId1 == root->TraceId, "{0}, {1}", traceId1, root->TraceId)
        VERIFY_IS_TRUE_FMT(oldSize == traceId1.size(), "{0}, {1}", oldSize, traceId1.size())
        VERIFY_IS_TRUE_FMT(oldCapacity >= capacity, "{0}, {1}", oldCapacity, capacity)

        // Put longer string: should overwrite old string in-place

        wstring traceId2(L"LongerTraceId");
        bool success = root->TryPutReservedTraceId(traceId2);
        VERIFY_IS_TRUE(success);

        size_t newSize = root->TraceId.size();
        size_t newCapacity = root->TraceId.capacity();
        void const * newAddress = static_cast<void const *>(root->TraceId.c_str());

        VERIFY_IS_TRUE_FMT(traceId2 == root->TraceId, "{0}, {1}", traceId2, root->TraceId)
        VERIFY_IS_TRUE_FMT(newSize == traceId2.size(), "{0}, {1}", newSize, traceId2.size())
        VERIFY_IS_TRUE_FMT(newCapacity == oldCapacity, "{0}, {1}", newCapacity, oldCapacity)
        VERIFY_IS_TRUE_FMT(newAddress == oldAddress, "{0}, {1}", newAddress, oldAddress);

        // Put shorter string: should overwrite old string in-place

        wstring traceId3(L"sTraceId");
        success = root->TryPutReservedTraceId(traceId3);
        VERIFY_IS_TRUE(success);

        newSize = root->TraceId.size();
        newCapacity = root->TraceId.capacity();
        newAddress = static_cast<void const *>(root->TraceId.c_str());

        VERIFY_IS_TRUE_FMT(traceId3 == root->TraceId, "{0}, {1}", traceId3, root->TraceId);
        VERIFY_IS_TRUE_FMT(newSize == traceId3.size(), "{0}, {1}", newSize, traceId3.size());
        VERIFY_IS_TRUE_FMT(newCapacity == oldCapacity, "{0}, {1}", newCapacity, oldCapacity);
        VERIFY_IS_TRUE_FMT(newAddress == oldAddress, "{0}, {1}", newAddress, oldAddress);

        // Put string that is greater than capacity: should fail and leave traceId unchanged

        wstring traceId4;
        for (size_t ix = 0; ix <= oldCapacity; ++ix)
        {
            traceId4.push_back(L'X');
        }

        success = root->TryPutReservedTraceId(traceId4);
        VERIFY_IS_FALSE(success);

        newSize = root->TraceId.size();
        newCapacity = root->TraceId.capacity();
        newAddress = static_cast<void const *>(root->TraceId.c_str());

        VERIFY_IS_TRUE_FMT(traceId3 == root->TraceId, "{0}, {1}", traceId3, root->TraceId);
        VERIFY_IS_TRUE_FMT(newSize == traceId3.size(), "{0}, {1}", newSize, traceId3.size());
        VERIFY_IS_TRUE_FMT(newCapacity == oldCapacity, "{0}, {1}", newCapacity, oldCapacity);
        VERIFY_IS_TRUE_FMT(newAddress == oldAddress, "{0}, {1}", newAddress, oldAddress);

        // Put string that is equal to capacity: should succeed

        wstring traceId5;
        for (size_t ix = 0; ix < oldCapacity; ++ix)
        {
            traceId5.push_back(L'Y');
        }

        success = root->TryPutReservedTraceId(traceId5);
        VERIFY_IS_TRUE(success);

        newSize = root->TraceId.size();
        newCapacity = root->TraceId.capacity();
        newAddress = static_cast<void const *>(root->TraceId.c_str());

        VERIFY_IS_TRUE_FMT(traceId5 == root->TraceId, "{0}, {1}", traceId5, root->TraceId);
        VERIFY_IS_TRUE_FMT(newSize == traceId5.size(), "{0}, {1}", newSize, traceId5.size());
        VERIFY_IS_TRUE_FMT(newCapacity == oldCapacity, "{0}, {1}", newCapacity, oldCapacity);
        VERIFY_IS_TRUE_FMT(newAddress == oldAddress, "{0}, {1}", newAddress, oldAddress);
    }

    BOOST_AUTO_TEST_SUITE_END()
}
