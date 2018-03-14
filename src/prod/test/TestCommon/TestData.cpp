// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace TestCommon;
using namespace Common;

TestData::TestData()
    : type_(Invalid), result_(), collection_(), map_()
{
}

TestData::TestData(wstring const& result)
    : type_(Value), result_(result), collection_(), map_()
{
}

TestData::TestData(std::vector<TestData> const& collection)
    : type_(Collection), result_(), collection_(collection), map_()
{
}

TestData::TestData(std::map<std::wstring,TestData> const& map)
    : type_(Map), result_(), collection_(), map_(map)
{
}

wstring const& TestData::getResult() const
{
    TestSession::FailTestIfNot(type_ == Value, "getResult called with invalid FabricTestData::Type");
    return result_;
}

size_t TestData::getSize() const
{
    TestSession::FailTestIfNot(type_ == Collection, "Collection[] called with invalid FabricTestData::Type");
    return collection_.size();
}

TestData const& TestData::operator[](int index) const
{
    TestSession::FailTestIfNot(type_ == Collection, "Collection[] called with invalid FabricTestData::Type");
    TestSession::FailTestIfNot(index < collection_.size(), "provided index {0} is greater than actual size {1}", index, collection_.size());
    return collection_[index];
}

bool TestData::ContainsKey(std::wstring const& key) const
{
    TestSession::FailTestIfNot(type_ == Map, "Map[] called with invalid FabricTestData::Type");
    auto iter = map_.find(key);
    return iter != map_.end();
}

TestData const& TestData::operator[](std::wstring key) const
{
    TestSession::FailTestIfNot(type_ == Map, "Map[] called with invalid FabricTestData::Type");
    auto iter = map_.find(key);
    TestSession::FailTestIf(iter == map_.end(), "provided key {0} not found in map", key);
    return iter->second;
}
