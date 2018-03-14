// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "ApplicationNodeLoads.h"
#include "Movement.h"
#include "ServiceEntry.h"
#include "ApplicationEntry.h"

using namespace Reliability::LoadBalancingComponent;

ApplicationNodeLoads::ApplicationNodeLoads(size_t globalMetricCount, size_t globalMetricStartIndex)
    : LazyMap<ApplicationEntry const*, NodeMetrics>(NodeMetrics(globalMetricCount, globalMetricStartIndex, false)),
    globalMetricCount_(globalMetricCount),
    globalMetricStartIndex_(globalMetricStartIndex)
{
}

ApplicationNodeLoads::ApplicationNodeLoads(ApplicationNodeLoads && other)
    : LazyMap<ApplicationEntry const*, NodeMetrics>(move(other)),
    globalMetricCount_(other.globalMetricCount_),
    globalMetricStartIndex_(other.globalMetricStartIndex_)
{
}

ApplicationNodeLoads::ApplicationNodeLoads(ApplicationNodeLoads const* base)
    : LazyMap<ApplicationEntry const*, NodeMetrics>(base),
    globalMetricCount_(base->globalMetricCount_),
    globalMetricStartIndex_(base->globalMetricStartIndex_)
{
}

ApplicationNodeLoads & ApplicationNodeLoads::operator = (ApplicationNodeLoads && other)
{
    globalMetricCount_ = other.globalMetricCount_;
    globalMetricStartIndex_ = other.globalMetricStartIndex_;
    return (ApplicationNodeLoads &)LazyMap<ApplicationEntry const*, NodeMetrics>::operator = (std::move(other));
}

NodeMetrics const& ApplicationNodeLoads::operator[](ApplicationEntry const* key) const
{
    return LazyMap<ApplicationEntry const*, NodeMetrics>::operator[](key);
}

NodeMetrics & ApplicationNodeLoads::operator[](ApplicationEntry const* key)
{
    auto it = data_.find(key);

    if (it != data_.end())
    {
        return it->second;
    }
    else if (baseMap_ != nullptr)
    {
        // Initialize new member of the LazyMap with pointer to base, instead of with copy of base as in LazyMap
        it = data_.insert(std::make_pair(key, NodeMetrics(&(baseMap_->operator[] (key))))).first;
        return it->second;
    }
    else
    {
        it = data_.insert(std::make_pair(key, NodeMetrics(globalMetricCount_, globalMetricStartIndex_, false))).first;
        return it->second;
    }
}

void ApplicationNodeLoads::SetNodeLoadsForApplication(ApplicationEntry const* app, std::map<NodeEntry const*, LoadEntry> && data)
{
    ASSERT_IF(baseMap_ != nullptr, "Setting node loads for application is allowed only in base map.");
    ASSERT_IF(data_.find(app) != data_.end(), "Setting node loads for application is allowed only if application is not present in data_.");

    data_.insert(make_pair(app, NodeMetrics(globalMetricCount_, move(data), false, globalMetricStartIndex_)));
}

void ApplicationNodeLoads::ChangeMovement(Movement const& oldMovement, Movement const& newMovement)
{
    ApplicationEntry const* appOldMove = oldMovement.IsValid ? oldMovement.Partition->Service->Application : nullptr;
    ApplicationEntry const* appNewMove = newMovement.IsValid ? newMovement.Partition->Service->Application : nullptr;

    if (appOldMove == nullptr && appNewMove == nullptr)
    {
        return;
    }

    if (appOldMove != nullptr && appOldMove->HasPerNodeCapacity && oldMovement.IsValid)
    {
        NodeMetrics & nodeMetrics = this->operator[] (appOldMove);
        nodeMetrics.ChangeMovement(oldMovement, Movement::Invalid);
    }

    if (appNewMove != nullptr && appNewMove->HasPerNodeCapacity && newMovement.IsValid)
    {
        NodeMetrics & nodeMetrics = this->operator[] (appNewMove);
        nodeMetrics.ChangeMovement(Movement::Invalid, newMovement);
    }
}
