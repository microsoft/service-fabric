// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

LoadMetricInformation::LoadMetricInformation()
{
}

LoadMetricInformation::LoadMetricInformation(LoadMetricInformation const & other) :
    name_(other.name_),
    isBlanacedBefore_(other.isBlanacedBefore_),
    isBlanacedAfter_(other.isBlanacedAfter_),
    deviationBefore_(other.deviationBefore_),
    deviationAfter_(other.deviationAfter_),
    balancingThreshold_(other.balancingThreshold_),
    action_(other.action_),
    activityThreshold_(other.activityThreshold_),
    clusterCapacity_(other.clusterCapacity_),
    clusterLoad_(other.clusterLoad_),
    remainingUnbufferedCapacity_(other.remainingUnbufferedCapacity_),
    nodeBufferPercentage_(other.nodeBufferPercentage_),
    bufferedCapacity_(other.bufferedCapacity_),
    remainingBufferedCapacity_(other.remainingBufferedCapacity_),
    isClusterCapacityViolation_(other.isClusterCapacityViolation_),
    minNodeLoadValue_(other.minNodeLoadValue_),
    minNodeLoadNodeId_(other.minNodeLoadNodeId_),
    maxNodeLoadValue_(other.maxNodeLoadValue_),
    maxNodeLoadNodeId_(other.maxNodeLoadNodeId_),
    currentClusterLoad_(other.currentClusterLoad_),
    bufferedClusterCapacityRemaining_(other.bufferedClusterCapacityRemaining_),
    clusterCapacityRemaining_(other.clusterCapacityRemaining_),
    maximumNodeLoad_(other.maximumNodeLoad_),
    minimumNodeLoad_(other.minimumNodeLoad_)
{
}

LoadMetricInformation::LoadMetricInformation(LoadMetricInformation && other) :
    name_(move(other.name_)),
    isBlanacedBefore_(move(other.isBlanacedBefore_)),
    isBlanacedAfter_(move(other.isBlanacedAfter_)),
    deviationBefore_(move(other.deviationBefore_)),
    deviationAfter_(move(other.deviationAfter_)),
    balancingThreshold_(move(other.balancingThreshold_)),
    action_(move(other.action_)),
    activityThreshold_(move(other.activityThreshold_)),
    clusterCapacity_(move(other.clusterCapacity_)),
    clusterLoad_(move(other.clusterLoad_)),
    remainingUnbufferedCapacity_(move(other.remainingUnbufferedCapacity_)),
    nodeBufferPercentage_(move(other.nodeBufferPercentage_)),
    bufferedCapacity_(move(other.bufferedCapacity_)),
    remainingBufferedCapacity_(move(other.remainingBufferedCapacity_)),
    isClusterCapacityViolation_(move(other.isClusterCapacityViolation_)),
    minNodeLoadValue_(move(other.minNodeLoadValue_)),
    minNodeLoadNodeId_(move(other.minNodeLoadNodeId_)),
    maxNodeLoadValue_(move(other.maxNodeLoadValue_)),
    maxNodeLoadNodeId_(move(other.maxNodeLoadNodeId_)),
    currentClusterLoad_(other.currentClusterLoad_),
    bufferedClusterCapacityRemaining_(other.bufferedClusterCapacityRemaining_),
    clusterCapacityRemaining_(other.clusterCapacityRemaining_),
    maximumNodeLoad_(other.maximumNodeLoad_),
    minimumNodeLoad_(other.minimumNodeLoad_)
{
}

LoadMetricInformation const & LoadMetricInformation::operator = (LoadMetricInformation const & other)
{
    if (this != & other)
    {
        name_ = other.name_;
        isBlanacedBefore_ = other.isBlanacedBefore_;
        isBlanacedAfter_ = other.isBlanacedAfter_;
        deviationBefore_ = other.deviationBefore_;
        deviationAfter_ = other.deviationAfter_;
        balancingThreshold_ = other.balancingThreshold_;
        action_ = other.action_;  
        activityThreshold_ = other.activityThreshold_;
        clusterCapacity_ = other.clusterCapacity_;
        clusterLoad_ = other.clusterLoad_;
        remainingUnbufferedCapacity_ = other.remainingUnbufferedCapacity_;
        nodeBufferPercentage_ = other.nodeBufferPercentage_;
        bufferedCapacity_ = other.bufferedCapacity_;
        remainingBufferedCapacity_ = other.remainingBufferedCapacity_;
        isClusterCapacityViolation_ = other.isClusterCapacityViolation_;
        minNodeLoadValue_ = other.minNodeLoadValue_;
        minNodeLoadNodeId_ = other.minNodeLoadNodeId_;
        maxNodeLoadValue_ = other.maxNodeLoadValue_;
        maxNodeLoadNodeId_ = other.maxNodeLoadNodeId_;
        currentClusterLoad_ = other.currentClusterLoad_;
        bufferedClusterCapacityRemaining_ = other.bufferedClusterCapacityRemaining_;
        clusterCapacityRemaining_ = other.clusterCapacityRemaining_;
        maximumNodeLoad_ = other.maximumNodeLoad_;
        minimumNodeLoad_ = other.minimumNodeLoad_;
    }

    return *this;
}

LoadMetricInformation const & LoadMetricInformation::operator=(LoadMetricInformation && other)
{
    if (this != &other)
    {
        name_ = move(other.name_);
        isBlanacedBefore_ = move(other.isBlanacedBefore_);
        isBlanacedAfter_ = move(other.isBlanacedAfter_);
        deviationBefore_ = move(other.deviationBefore_);
        deviationAfter_ = move(other.deviationAfter_);
        balancingThreshold_ = move(other.balancingThreshold_);
        action_ = move(other.action_);
        activityThreshold_ = move(other.activityThreshold_);
        clusterCapacity_ = move(other.clusterCapacity_);
        clusterLoad_ = move(other.clusterLoad_);
        remainingUnbufferedCapacity_ = move(other.remainingUnbufferedCapacity_);
        nodeBufferPercentage_ = move(other.nodeBufferPercentage_);
        bufferedCapacity_ = move(other.bufferedCapacity_);
        remainingBufferedCapacity_ = move(other.remainingBufferedCapacity_);
        isClusterCapacityViolation_ = move(other.isClusterCapacityViolation_);
        minNodeLoadValue_ = move(other.minNodeLoadValue_);
        minNodeLoadNodeId_ = move(other.minNodeLoadNodeId_);
        maxNodeLoadValue_ = move(other.maxNodeLoadValue_);
        maxNodeLoadNodeId_ = move(other.maxNodeLoadNodeId_);
        currentClusterLoad_ = other.currentClusterLoad_;
        bufferedClusterCapacityRemaining_ = other.bufferedClusterCapacityRemaining_;
        clusterCapacityRemaining_ = other.clusterCapacityRemaining_;
        maximumNodeLoad_ = other.maximumNodeLoad_;
        minimumNodeLoad_ = other.minimumNodeLoad_;
    }

    return *this;
}

void LoadMetricInformation::ToPublicApi(__in ScopedHeap & heap, __out FABRIC_LOAD_METRIC_INFORMATION & publicValue) const
{
    publicValue.Name = heap.AddString(name_);
    publicValue.IsBalancedAfter = isBlanacedAfter_ ? TRUE : FALSE;
    publicValue.IsBalancedBefore = isBlanacedBefore_ ? TRUE : FALSE;
    publicValue.DeviationAfter = deviationAfter_;
    publicValue.DeviationBefore = deviationBefore_;
    publicValue.BalancingThreshold = balancingThreshold_;
    publicValue.Action = heap.AddString(action_);

    auto publicValueEx1 = heap.AddItem<FABRIC_LOAD_METRIC_INFORMATION_EX1>();
    publicValueEx1->ActivityThreshold = activityThreshold_;
    publicValueEx1->ClusterCapacity = clusterCapacity_;
    publicValueEx1->ClusterLoad = clusterLoad_;
    publicValue.Reserved = publicValueEx1.GetRawPointer();

    auto publicValueEx2 = heap.AddItem<FABRIC_LOAD_METRIC_INFORMATION_EX2>();
    publicValueEx2->RemainingUnbufferedCapacity = remainingUnbufferedCapacity_;
    publicValueEx2->NodeBufferPercentage = nodeBufferPercentage_;
    publicValueEx2->BufferedCapacity = bufferedCapacity_;
    publicValueEx2->RemainingBufferedCapacity = remainingBufferedCapacity_;
    publicValueEx2->IsClusterCapacityViolation = isClusterCapacityViolation_;
    publicValueEx2->MinNodeLoadValue = minNodeLoadValue_;
    minNodeLoadNodeId_.ToPublicApi(publicValueEx2->MinNodeLoadNodeId);
    publicValueEx2->MaxNodeLoadValue = maxNodeLoadValue_;
    maxNodeLoadNodeId_.ToPublicApi(publicValueEx2->MaxNodeLoadNodeId);

    publicValueEx1->Reserved = publicValueEx2.GetRawPointer();

    auto publicValueEx3 = heap.AddItem<FABRIC_LOAD_METRIC_INFORMATION_EX3>();
    publicValueEx3->CurrentClusterLoad = currentClusterLoad_;
    publicValueEx3->BufferedClusterCapacityRemaining = bufferedClusterCapacityRemaining_;
    publicValueEx3->ClusterCapacityRemaining = clusterCapacityRemaining_;
    publicValueEx3->MaximumNodeLoad = maximumNodeLoad_;
    publicValueEx3->MinimumNodeLoad = minimumNodeLoad_;
    publicValueEx3->Reserved = NULL;

    publicValueEx2->Reserved = publicValueEx3.GetRawPointer();
}

Common::ErrorCode LoadMetricInformation::FromPublicApi(__in FABRIC_LOAD_METRIC_INFORMATION const & publicValue)
{
    auto hr = StringUtility::LpcwstrToWstring(publicValue.Name, false, name_);
    if (FAILED(hr))
    {
        return ErrorCode::FromHResult(hr);
    }

    isBlanacedAfter_ = publicValue.IsBalancedAfter ? true : false;
    isBlanacedBefore_ = publicValue.IsBalancedBefore ? true : false;
    deviationAfter_= publicValue.DeviationAfter;
    deviationBefore_ = publicValue.DeviationBefore;
    balancingThreshold_ = publicValue.BalancingThreshold;

    hr = StringUtility::LpcwstrToWstring(publicValue.Action, false, action_);
    if (FAILED(hr))
    {
        return ErrorCode::FromHResult(hr);
    }

    if (publicValue.Reserved != NULL)
    {
        FABRIC_LOAD_METRIC_INFORMATION_EX1 const & publicValueEx1 =
            *(static_cast<FABRIC_LOAD_METRIC_INFORMATION_EX1*>(publicValue.Reserved));
        activityThreshold_ = publicValueEx1.ActivityThreshold;
        clusterCapacity_ = publicValueEx1.ClusterCapacity;
        clusterLoad_ = publicValueEx1.ClusterLoad;

        if (publicValueEx1.Reserved != NULL)
        {
            FABRIC_LOAD_METRIC_INFORMATION_EX2 const & publicValueEx2 =
                *(static_cast<FABRIC_LOAD_METRIC_INFORMATION_EX2*>(publicValueEx1.Reserved));

            remainingUnbufferedCapacity_ = publicValueEx2.RemainingUnbufferedCapacity;
            nodeBufferPercentage_ = publicValueEx2.NodeBufferPercentage;
            bufferedCapacity_ = publicValueEx2.BufferedCapacity;
            remainingBufferedCapacity_ = publicValueEx2.RemainingBufferedCapacity;
            isClusterCapacityViolation_ = publicValueEx2.IsClusterCapacityViolation ? true : false;
            minNodeLoadValue_ = publicValueEx2.MinNodeLoadValue;
            minNodeLoadNodeId_.FromPublicApi(publicValueEx2.MinNodeLoadNodeId);
            maxNodeLoadValue_ = publicValueEx2.MaxNodeLoadValue;
            maxNodeLoadNodeId_.FromPublicApi(publicValueEx2.MaxNodeLoadNodeId);

            if (publicValueEx2.Reserved != NULL)
            {
                FABRIC_LOAD_METRIC_INFORMATION_EX3 const & publicValueEx3 =
                    *(static_cast<FABRIC_LOAD_METRIC_INFORMATION_EX3*>(publicValueEx2.Reserved));

                currentClusterLoad_ = publicValueEx3.CurrentClusterLoad;
                bufferedClusterCapacityRemaining_ = publicValueEx3.BufferedClusterCapacityRemaining;
                clusterCapacityRemaining_ = publicValueEx3.ClusterCapacityRemaining;
                maximumNodeLoad_ = publicValueEx3.MaximumNodeLoad;
                minimumNodeLoad_ = publicValueEx3.MinimumNodeLoad;
            }
        }
    }

    return ErrorCode(ErrorCodeValue::Success);
}

void LoadMetricInformation::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.Write(ToString());
}

std::wstring LoadMetricInformation::ToString() const
{
    wstring objectString;
    auto error = JsonHelper::Serialize(const_cast<LoadMetricInformation&>(*this), objectString);
    if (!error.IsSuccess())
    {
        return wstring();
    }
    return objectString;
}
