// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;

using namespace Management::RepairManager;

NodeImpact::NodeImpact()
    : nodeName_()
    , impactLevel_(NodeImpactLevel::None)
{
}

NodeImpact::NodeImpact(NodeImpact && other)
    : nodeName_(move(other.nodeName_))
    , impactLevel_(move(other.impactLevel_))
{
}

bool NodeImpact::operator == (NodeImpact const & other) const
{
    return (impactLevel_ == other.impactLevel_) && (nodeName_ == other.nodeName_);
}

bool NodeImpact::operator != (NodeImpact const & other) const
{
    return !(*this == other);
}

ErrorCode NodeImpact::FromPublicApi(FABRIC_REPAIR_NODE_IMPACT const & publicDescription)
{
    if (publicDescription.Reserved != NULL) { return ErrorCodeValue::InvalidArgument; }

    HRESULT hr = StringUtility::LpcwstrToWstring(publicDescription.NodeName, false, nodeName_); // ISSUE: max length?
    if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

    auto error = NodeImpactLevel::FromPublicApi(publicDescription.ImpactLevel, impactLevel_);
    if (!error.IsSuccess()) { return error; }

    return ErrorCode::Success();
}

void NodeImpact::ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_REPAIR_NODE_IMPACT & result) const
{
    result.NodeName = heap.AddString(nodeName_);
    result.ImpactLevel = NodeImpactLevel::ToPublicApi(impactLevel_);
}

void NodeImpact::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.Write("NodeImpact[{0},{1}]", nodeName_, impactLevel_);
}

bool NodeImpact::IsValid() const
{
    return (!nodeName_.empty() && (impactLevel_ != NodeImpactLevel::Invalid));
}
