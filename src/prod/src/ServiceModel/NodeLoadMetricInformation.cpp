// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

NodeLoadMetricInformation::NodeLoadMetricInformation()
{
}

NodeLoadMetricInformation::NodeLoadMetricInformation(NodeLoadMetricInformation const & other) :
    name_(other.name_),
    nodeCapacity_(other.nodeCapacity_),
    nodeLoad_(other.nodeLoad_),
    nodeRemainingCapacity_(other.nodeRemainingCapacity_),
    isCapacityViolation_(other.isCapacityViolation_),
    nodeBufferedCapacity_(other.nodeBufferedCapacity_),
    nodeRemainingBufferedCapacity_(other.nodeRemainingBufferedCapacity_),
    currentNodeLoad_(other.currentNodeLoad_),
    nodeCapacityRemaining_(other.nodeCapacityRemaining_),
    bufferedNodeCapacityRemaining_(other.bufferedNodeCapacityRemaining_)
{
}

NodeLoadMetricInformation::NodeLoadMetricInformation(NodeLoadMetricInformation && other) :
    name_(move(other.name_)),
    nodeCapacity_(move(other.nodeCapacity_)),
    nodeLoad_(move(other.nodeLoad_)),
    nodeRemainingCapacity_(move(other.nodeRemainingCapacity_)),
    isCapacityViolation_(move(other.isCapacityViolation_)),
    nodeBufferedCapacity_(move(other.nodeBufferedCapacity_)),
    nodeRemainingBufferedCapacity_(move(other.nodeRemainingBufferedCapacity_)),
    currentNodeLoad_(move(other.currentNodeLoad_)),
    nodeCapacityRemaining_(move(other.nodeCapacityRemaining_)),
    bufferedNodeCapacityRemaining_(move(other.bufferedNodeCapacityRemaining_))
{
}

NodeLoadMetricInformation const & NodeLoadMetricInformation::operator = (NodeLoadMetricInformation const & other)
{
    if (this != & other)
    {
        name_ = other.name_;
        nodeCapacity_ = other.nodeCapacity_;
        nodeLoad_ = other.nodeLoad_;
        nodeRemainingCapacity_ = other.nodeRemainingCapacity_;
        isCapacityViolation_ = other.isCapacityViolation_;
        nodeBufferedCapacity_ = other.nodeBufferedCapacity_;
        nodeRemainingBufferedCapacity_ = other.nodeRemainingBufferedCapacity_;
        currentNodeLoad_ = other.currentNodeLoad_;
        nodeCapacityRemaining_ = other.nodeCapacityRemaining_;
        bufferedNodeCapacityRemaining_ = other.bufferedNodeCapacityRemaining_;
    }

    return *this;
}

NodeLoadMetricInformation const & NodeLoadMetricInformation::operator=(NodeLoadMetricInformation && other)
{
    if (this != &other)
    {
        name_ = move(other.name_);
        nodeCapacity_ = move(other.nodeCapacity_);
        nodeLoad_ = move(other.nodeLoad_);
        nodeRemainingCapacity_ = move(other.nodeRemainingCapacity_);
        isCapacityViolation_ = move(other.isCapacityViolation_);
        nodeBufferedCapacity_ = move(other.nodeBufferedCapacity_);
        nodeRemainingBufferedCapacity_ = move(other.nodeRemainingBufferedCapacity_);
        currentNodeLoad_ = move(other.currentNodeLoad_);
        nodeCapacityRemaining_ = move(other.nodeCapacityRemaining_);
        bufferedNodeCapacityRemaining_ = move(other.bufferedNodeCapacityRemaining_);
    }

    return *this;
}

void NodeLoadMetricInformation::ToPublicApi(__in ScopedHeap & heap, __out FABRIC_NODE_LOAD_METRIC_INFORMATION & publicValue) const
{
    publicValue.Name = heap.AddString(name_);
    publicValue.NodeCapacity = nodeCapacity_;
    publicValue.NodeLoad = nodeLoad_;
    publicValue.NodeRemainingCapacity = nodeRemainingCapacity_;
    publicValue.IsCapacityViolation = isCapacityViolation_;

    auto publicValueEx1 = heap.AddItem<FABRIC_NODE_LOAD_METRIC_INFORMATION_EX1>();
    publicValueEx1->NodeBufferedCapacity = nodeBufferedCapacity_;
    publicValueEx1->NodeRemainingBufferedCapacity = nodeRemainingBufferedCapacity_;
    publicValue.Reserved = publicValueEx1.GetRawPointer();

    // write double values
    auto publicValueEx2 = heap.AddItem<FABRIC_NODE_LOAD_METRIC_INFORMATION_EX2>();
    publicValueEx2->CurrentNodeLoad = currentNodeLoad_;
    publicValueEx2->NodeCapacityRemaining = nodeCapacityRemaining_;
    publicValueEx2->BufferedNodeCapacityRemaining = bufferedNodeCapacityRemaining_;
    publicValueEx2->Reserved = NULL;

    publicValueEx1->Reserved = publicValueEx2.GetRawPointer();
}

Common::ErrorCode NodeLoadMetricInformation::FromPublicApi(__in FABRIC_NODE_LOAD_METRIC_INFORMATION const & publicValue)
{
    auto hr = StringUtility::LpcwstrToWstring(publicValue.Name, false, name_);
    if (FAILED(hr))
    {
        return ErrorCode::FromHResult(hr);
    }

    nodeCapacity_ = publicValue.NodeCapacity;
    nodeLoad_ = publicValue.NodeLoad;
    nodeRemainingCapacity_ = publicValue.NodeRemainingCapacity;
    isCapacityViolation_ = publicValue.IsCapacityViolation ? true : false;;

    if (publicValue.Reserved != NULL)
    {
        FABRIC_NODE_LOAD_METRIC_INFORMATION_EX1 const & publicValueEx1 =
            *(static_cast<FABRIC_NODE_LOAD_METRIC_INFORMATION_EX1*>(publicValue.Reserved));

        nodeBufferedCapacity_ = publicValueEx1.NodeBufferedCapacity;
        nodeRemainingBufferedCapacity_ = publicValueEx1.NodeRemainingBufferedCapacity;

        if (publicValueEx1.Reserved != NULL)
        {
            // get double values
            FABRIC_NODE_LOAD_METRIC_INFORMATION_EX2 const & publicValueEx2 =
                *(static_cast<FABRIC_NODE_LOAD_METRIC_INFORMATION_EX2*>(publicValueEx1.Reserved));

            currentNodeLoad_ = publicValueEx2.CurrentNodeLoad;
            nodeCapacityRemaining_ = publicValueEx2.NodeCapacityRemaining;
            bufferedNodeCapacityRemaining_ = publicValueEx2.BufferedNodeCapacityRemaining;
        }
    }

    return ErrorCode(ErrorCodeValue::Success);
}

void NodeLoadMetricInformation::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.Write(ToString());
}

std::wstring NodeLoadMetricInformation::ToString() const
{
    wstring objectString;
    auto error = JsonHelper::Serialize(const_cast<NodeLoadMetricInformation&>(*this), objectString);
    if (!error.IsSuccess())
    {
        return wstring();
    }
    return objectString;
}
