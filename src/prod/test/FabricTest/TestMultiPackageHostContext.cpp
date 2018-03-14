// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace FabricTest;

wstring const TestMultiPackageHostContext::ParamDelimiter = L";";

TestMultiPackageHostContext::TestMultiPackageHostContext(
    std::wstring const& nodeId, 
    std::wstring const& hostId)
    : nodeId_(nodeId),
    hostId_(hostId)
{
}

bool TestMultiPackageHostContext::FromClientId(std::wstring const& clientId, TestMultiPackageHostContext & testMultiPackageHostContext)
{
    StringCollection collection;
    StringUtility::Split<wstring>(clientId, collection, TestMultiPackageHostContext::ParamDelimiter);
    if(collection.size() == 2)
    {
        Federation::NodeId nodeId;
        TestSession::FailTestIfNot(Federation::NodeId::TryParse(collection[0], nodeId), "Could not parse NodeId: {0}", clientId);
        testMultiPackageHostContext = TestMultiPackageHostContext(nodeId.ToString(), collection[1]);
        return true;
    }
    else
    {
        return false;
    }
}

wstring TestMultiPackageHostContext::ToClientId() const
{
    return wformatString("{0};{1}", nodeId_, hostId_);
}

bool TestMultiPackageHostContext::operator == (TestMultiPackageHostContext const & other) const
{
    bool equals = true;

    equals = StringUtility::AreEqualCaseInsensitive(this->nodeId_, other.nodeId_);
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->hostId_, other.hostId_);
    if (!equals) { return equals; }

    return equals;
}

bool TestMultiPackageHostContext::operator != (TestMultiPackageHostContext const & other) const
{
    return !(*this == other);
}

void TestMultiPackageHostContext::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("TestMultiPackageHostContext { ");
    w.Write("NodeId = {0}, ", nodeId_);
    w.Write("HostId = {0}, ", hostId_);
    w.Write("}");
}
