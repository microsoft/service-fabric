// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

using namespace Common;
using namespace Reliability;
using namespace ServiceModel;
using namespace std;

namespace ServiceModelTests
{
    namespace ReservedIdType
    {
        enum Enum
        {
            Naming,
            FM,
            RM,
            CM,
            FileStoreService
        };
    }

    class TestConsistencyUnitId
    {
    protected:
        void Verify(std::wstring const & guid, bool expected, ReservedIdType::Enum type);
    };

    function<bool(ConsistencyUnitId const &)> GetFunction(ReservedIdType::Enum type)
    {
        switch (type)
        {
        case ReservedIdType::Naming:
            return [](ConsistencyUnitId const & cuid) { return cuid.IsNaming(); };
        case ReservedIdType::RM:
            return [](ConsistencyUnitId const & cuid) { return cuid.IsRepairManager(); };
        case ReservedIdType::FM:
            return [](ConsistencyUnitId const & cuid) { return cuid.IsFailoverManager(); };
        case ReservedIdType::FileStoreService:
            return [](ConsistencyUnitId const & cuid) { return cuid.IsFileStoreService(); };
        case ReservedIdType::CM:
            return [](ConsistencyUnitId const & cuid) { return cuid.IsClusterManager(); };
        default:
            Assert::CodingError("unknown type {0}", static_cast<int>(type));
        }
    }


    BOOST_FIXTURE_TEST_SUITE(TestConsistencyUnitIdSuite,TestConsistencyUnitId)

    BOOST_AUTO_TEST_CASE(CuidTest)
    {
        Verify(L"00000000-0000-0000-0000-000000000000", false, ReservedIdType::FM);
        Verify(L"00000000-0000-0000-0000-000000000001", true, ReservedIdType::FM);
        Verify(L"00000000-0000-0000-0000-00000000000F", true, ReservedIdType::FM);
        Verify(L"00000000-0000-0000-0000-000000000FFF", true, ReservedIdType::FM);
        Verify(L"00000000-0000-0000-0000-000000001000", false, ReservedIdType::FM);
        Verify(L"00000000-0000-0000-0000-00001000000F", false, ReservedIdType::FM);

        Verify(L"00000000-0000-0000-0000-000000000FFF", false, ReservedIdType::Naming);
        Verify(L"00000000-0000-0000-0000-000000001000", true, ReservedIdType::Naming);
        Verify(L"00000000-0000-0000-0000-00000000100F", true, ReservedIdType::Naming);
        Verify(L"00000000-0000-0000-0000-000000001FFF", true, ReservedIdType::Naming);
        Verify(L"00000000-0000-0000-0000-000000002000", false, ReservedIdType::Naming);
        Verify(L"00000000-0000-0000-0000-00001000100F", false, ReservedIdType::Naming);

        Verify(L"00000000-0000-0000-0000-000000001FFF", false, ReservedIdType::CM);
        Verify(L"00000000-0000-0000-0000-000000002000", true, ReservedIdType::CM);
        Verify(L"00000000-0000-0000-0000-00000000200F", true, ReservedIdType::CM);
        Verify(L"00000000-0000-0000-0000-000000002FFF", true, ReservedIdType::CM);
        Verify(L"00000000-0000-0000-0000-000000003000", false, ReservedIdType::CM);
        Verify(L"00000000-0000-0000-0000-00002000200F", false, ReservedIdType::CM);

        Verify(L"00000000-0000-0000-0000-000000002FFF", false, ReservedIdType::FileStoreService);
        Verify(L"00000000-0000-0000-0000-000000003000", true, ReservedIdType::FileStoreService);
        Verify(L"00000000-0000-0000-0000-00000000300F", true, ReservedIdType::FileStoreService);
        Verify(L"00000000-0000-0000-0000-000000003FFF", true, ReservedIdType::FileStoreService);
        Verify(L"00000000-0000-0000-0000-000000004000", false, ReservedIdType::FileStoreService);
        Verify(L"00000000-0000-0000-0000-00003000300F", false, ReservedIdType::FileStoreService);

        Verify(L"00000000-0000-0000-0000-000000003FFF", false, ReservedIdType::RM);
        Verify(L"00000000-0000-0000-0000-000000004000", true, ReservedIdType::RM);
        Verify(L"00000000-0000-0000-0000-00000000400F", true, ReservedIdType::RM);
        Verify(L"00000000-0000-0000-0000-000000004FFF", true, ReservedIdType::RM);
        Verify(L"00000000-0000-0000-0000-000000005000", false, ReservedIdType::RM);
        Verify(L"00000000-0000-0000-0000-00001000400F", false, ReservedIdType::RM);
    }

    BOOST_AUTO_TEST_SUITE_END()

    void TestConsistencyUnitId::Verify(std::wstring const & guid, bool expected, ReservedIdType::Enum type)
    {
        auto func = GetFunction(type);

        ConsistencyUnitId cuid(guid);

        bool actual = func(cuid);
        VERIFY_ARE_EQUAL(expected, actual, guid.c_str());
    }

}
