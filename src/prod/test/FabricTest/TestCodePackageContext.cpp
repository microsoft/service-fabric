// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace FabricTest;
using namespace ServiceModel;
using namespace Hosting2;

wstring const TestCodePackageContext::ParamDelimiter = L";";

TestCodePackageContext::TestCodePackageContext(
    wstring const& nodeId,
    Hosting2::CodePackageInstanceIdentifier const& codePackageId,
    wstring const& instanceId,
    wstring const& codePackageVersion)
    : nodeId_(nodeId),
    codePackageId_(codePackageId),
    instanceId_(instanceId),
    codePackageVersion_(codePackageVersion),
    isMultiHost_(false),
    testMultiPackageHostContext_()
{
}

TestCodePackageContext::TestCodePackageContext(
    wstring const& nodeId,
    Hosting2::CodePackageInstanceIdentifier const& codePackageId,
    wstring const& instanceId,
    wstring const& codePackageVersion,
    TestMultiPackageHostContext const& testMultiPackageHostContext)
    : nodeId_(nodeId),
    codePackageId_(codePackageId),
    instanceId_(instanceId),
    codePackageVersion_(codePackageVersion),
    isMultiHost_(true),
    testMultiPackageHostContext_(testMultiPackageHostContext)
{
}


bool TestCodePackageContext::FromClientId(std::wstring const& clientId, TestCodePackageContext & testCodePackageContext)
{
    StringCollection collection;
    StringUtility::Split<wstring>(clientId, collection, TestCodePackageContext::ParamDelimiter);
    if(collection.size() == 4)
    {
        Federation::NodeId nodeId;
        TestSession::FailTestIfNot(Federation::NodeId::TryParse(collection[0], nodeId), "Could not parse NodeId: {0}", clientId);
        
        CodePackageInstanceIdentifier codePackageInstanceId;
        auto error = CodePackageInstanceIdentifier::FromString(collection[1], codePackageInstanceId);

        TestSession::FailTestIfNot(error.IsSuccess(), "Could not parse codePackageInstanceId: {0}", collection[1]);
        
        testCodePackageContext = TestCodePackageContext(nodeId.ToString(), codePackageInstanceId, collection[2], collection[3]);
        return true;
    }
    else
    {
        return false;
    }
}

wstring TestCodePackageContext::ToClientId() const
{
    if(IsMultiHost)
    {
        return move(testMultiPackageHostContext_.ToClientId());
    }
    else
    {
        return move(wformatString(
            "{0}{1}{2}{3}{4}{5}{6}", 
            nodeId_,
            ParamDelimiter,
            codePackageId_.ToString(), 
            ParamDelimiter,
            instanceId_, 
            ParamDelimiter,
            codePackageVersion_));
    }
}

bool TestCodePackageContext::operator == (TestCodePackageContext const & other) const
{
    bool equals = true;

    equals = StringUtility::AreEqualCaseInsensitive(this->nodeId_, other.nodeId_);
    if (!equals) { return equals; }

    equals = this->codePackageId_ == other.codePackageId_;
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->instanceId_, other.instanceId_);
    if (!equals) { return equals; }

    equals = this->isMultiHost_ == other.isMultiHost_;
    if (!equals) { return equals; }

    if(isMultiHost_)
    {
        equals = this->testMultiPackageHostContext_ == other.testMultiPackageHostContext_;
        if (!equals) { return equals; }
    }

    return equals;
}

bool TestCodePackageContext::operator != (TestCodePackageContext const & other) const
{
    return !(*this == other);
}

void TestCodePackageContext::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("TestCodePackageContext { ");
    w.Write("NodeId = {0}, ", nodeId_);
    w.Write("CodePackageInstanceId = {0}, ", codePackageId_);
    w.Write("InstaceId = {0}, ", instanceId_);
    w.Write("IsMultiHost = {0}, ", isMultiHost_);
    if(isMultiHost_)
    {
        w.Write("TestMultiPackageHostContext = {0}", testMultiPackageHostContext_);
    }
    w.Write("}");
}
