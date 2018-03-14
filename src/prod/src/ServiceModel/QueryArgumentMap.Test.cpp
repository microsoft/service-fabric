// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

using namespace std;
using namespace Common;
using namespace ::Query;
using namespace ServiceModel;

class TestQueryRequestParameterParserUtility
{
protected:
    void Insert(
        wstring key,
        wstring value)
    {
        queryArgumentMap_.Insert(key, value);
    }

    void Insert(
        wstring fieldName,
        int64 value)
    {
        auto stringValue = wformatString(value);
        queryArgumentMap_.Insert(fieldName, stringValue);
    }

    void Insert(
        wstring fieldName,
        Guid value)
    {
        auto stringValue = wformatString(value);
        queryArgumentMap_.Insert(fieldName, stringValue);
    }

    bool TryGetValue(wstring key, wstring & outVal) const
    {
        return queryArgumentMap_.TryGetValue(key, outVal);
    }

    bool TryGetValue(wstring key, int64 & outVal) const
    {
        return queryArgumentMap_.TryGetValue(key, outVal);
    }

    bool TryGetValue(wstring key, Guid & outVal) const
    {
        return queryArgumentMap_.TryGetValue(key, outVal);
    }

    bool TryGetPartitionId(Guid & outVal) const
    {
        return queryArgumentMap_.TryGetPartitionId(outVal);
    }

    bool TryGetReplicaId(int64 & outVal) const
    {
        return queryArgumentMap_.TryGetReplicaId(outVal);
    }

protected:
    QueryArgumentMap queryArgumentMap_;
};

BOOST_AUTO_TEST_SUITE(Unit)

BOOST_FIXTURE_TEST_SUITE(TestQueryRequestParameterParserUtilitySuite, TestQueryRequestParameterParserUtility)

BOOST_AUTO_TEST_CASE(WithStringValue)
{
    auto key = L"key";
    auto inputVal = L"applicationName";
    Insert(key, inputVal);

    wstring outVal;
    VERIFY_IS_TRUE(TryGetValue(key, outVal));
    VERIFY_ARE_EQUAL(inputVal, outVal);
}

BOOST_AUTO_TEST_CASE(WithMissingStringValue)
{
    auto key = L"key";
    wstring outVal;

    VERIFY_IS_FALSE(TryGetValue(key, outVal));
    VERIFY_IS_TRUE(outVal.empty());
}

BOOST_AUTO_TEST_CASE(WithInt64Value)
{
    auto key = L"key";
    int64 inputVal = 123456789;
    Insert(key, inputVal);
    int64 outVal;

    VERIFY_IS_TRUE(TryGetValue(key, outVal));
    VERIFY_ARE_EQUAL(inputVal, outVal);
}

BOOST_AUTO_TEST_CASE(WithMissingInt64Value)
{
    auto key = L"key";
    int64 outVal;

    VERIFY_IS_TRUE(!TryGetValue(key, outVal));
}

BOOST_AUTO_TEST_CASE(WithInvalidInt64FormatValue)
{
    auto key = L"key";
    wstring inputVal = L"invalidFormatValue";
    Insert(key, inputVal);
    int64 outVal;
     
    VERIFY_IS_TRUE(!TryGetValue(key, outVal));
}

BOOST_AUTO_TEST_CASE(WithGuidValue)
{
    auto key = L"key";
    auto inputVal = Guid::NewGuid();
    Insert(key, inputVal);
    Guid outVal;

    VERIFY_IS_TRUE(TryGetValue(key, outVal));
    VERIFY_ARE_EQUAL(inputVal, outVal);
}

BOOST_AUTO_TEST_CASE(WithMissingGuidValue)
{
    auto key = L"key";
    Guid outVal;

    VERIFY_IS_FALSE(TryGetValue(key, outVal));
}

BOOST_AUTO_TEST_CASE(WithEmptyStringGuidValue)
{
    auto key = L"key";
    wstring inputVal = L"";
    Insert(key, inputVal);
    Guid outVal;

    VERIFY_IS_FALSE(TryGetValue(key, outVal));
}

BOOST_AUTO_TEST_CASE(WithInvalidGuidStringValue)
{
    auto key = L"key";
    auto inputVal = L"invalidFormatValue";
    Insert(key, inputVal);
    Guid outVal;

    VERIFY_IS_FALSE(TryGetValue(key, outVal));
}

BOOST_AUTO_TEST_CASE(WithPartitionId)
{
    auto inputVal = Guid::NewGuid();
    Insert(QueryResourceProperties::Partition::PartitionId, inputVal);
    Guid outVal;

    VERIFY_IS_TRUE(TryGetPartitionId(outVal));
    VERIFY_ARE_EQUAL(inputVal, outVal);
}

BOOST_AUTO_TEST_CASE(WithReplicaId)
{
    int64 inputVal = 123456789;
    Insert(QueryResourceProperties::Replica::ReplicaId, inputVal);
    int64 outVal;

    VERIFY_IS_TRUE(TryGetReplicaId(outVal));
    VERIFY_ARE_EQUAL(inputVal, outVal);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
