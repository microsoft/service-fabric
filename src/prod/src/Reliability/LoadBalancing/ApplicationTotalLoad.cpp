// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Movement.h"
#include "ApplicationTotalLoad.h"
#include "ApplicationEntry.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

ApplicationTotalLoad::ApplicationTotalLoad(size_t globalMetricCount, size_t totalMetricCount)
    : LazyMap < ApplicationEntry const*, LoadEntry> (LoadEntry(globalMetricCount)),
    totalMetricCount_ (totalMetricCount)
{
}

ApplicationTotalLoad::ApplicationTotalLoad(ApplicationTotalLoad && other)
    : LazyMap < ApplicationEntry const*, LoadEntry>(move(other)),
    totalMetricCount_(other.totalMetricCount_)
{
}

ApplicationTotalLoad::ApplicationTotalLoad(ApplicationTotalLoad const& other)
    : LazyMap < ApplicationEntry const*, LoadEntry>(other),
    totalMetricCount_(other.totalMetricCount_)
{
}

ApplicationTotalLoad::ApplicationTotalLoad(ApplicationTotalLoad const* baseAppLoad)
    : LazyMap < ApplicationEntry const*, LoadEntry>(baseAppLoad),
    totalMetricCount_(baseAppLoad->totalMetricCount_)
{
}

ApplicationTotalLoad & ApplicationTotalLoad::operator = (ApplicationTotalLoad && other)
{
    if (this != &other)
    {
        LazyMap<ApplicationEntry const*, LoadEntry>::operator = (move(other));
        totalMetricCount_ = other.totalMetricCount_;
    }

    return *this;
}

void ApplicationTotalLoad::ChangeMovement(Movement const& oldMovement, Movement const& newMovement)
{
    if (oldMovement.IsValid && oldMovement.Partition && oldMovement.Partition->Service &&
        oldMovement.Partition->Service->Application)
    {
        ApplicationEntry const* application = oldMovement.Partition->Service->Application;
        if (application->HasAppCapacity)
        {
            if (oldMovement.IsMove)
            {
                if (oldMovement.Partition->StandByLocations.size() > 0)
                {
                    AddSbLoad(oldMovement.TargetNode, oldMovement.Partition);
                    DeleteSbLoad(oldMovement.SourceNode, oldMovement.Partition);
                }
            }
            else if (oldMovement.IsAdd)
            {
                if (oldMovement.Partition->StandByLocations.size() > 0)
                {
                    AddSbLoad(oldMovement.TargetNode, oldMovement.Partition);
                }
                DeleteLoad(application, oldMovement.Partition, oldMovement.TargetToBeAddedReplica->ReplicaEntry);
            }
            else if (oldMovement.IsDrop)
            {
                if (oldMovement.Partition->StandByLocations.size() > 0)
                {
                    DeleteSbLoad(oldMovement.SourceNode, oldMovement.Partition);
                }
                AddLoad(application, oldMovement.Partition, oldMovement.SourceToBeDeletedReplica->ReplicaEntry);
            }
        }
    }

    if (newMovement.IsValid && newMovement.Partition && newMovement.Partition->Service &&
        newMovement.Partition->Service->Application)
    {
        ApplicationEntry const* application = newMovement.Partition->Service->Application;
        if (application->HasAppCapacity)
        {
            if (newMovement.IsMove)
            {
                if (newMovement.Partition->StandByLocations.size() > 0)
                {
                    DeleteSbLoad(newMovement.TargetNode, newMovement.Partition);
                    AddSbLoad(newMovement.SourceNode, newMovement.Partition);
                }
            }
            else if (newMovement.IsAdd)
            {
                if (newMovement.Partition->StandByLocations.size() > 0)
                {
                    DeleteSbLoad(newMovement.TargetNode, newMovement.Partition);
                }
                AddLoad(application, newMovement.Partition, newMovement.TargetToBeAddedReplica->ReplicaEntry);
            }
            else if (newMovement.IsDrop)
            {
                if (newMovement.Partition->StandByLocations.size() > 0)
                {
                    AddSbLoad(newMovement.SourceNode, newMovement.Partition);
                }
                DeleteLoad(application, newMovement.Partition, newMovement.SourceToBeDeletedReplica->ReplicaEntry);
            }
        }
    }
}

void ApplicationTotalLoad::SetLoad(ApplicationEntry const* application)
{
    if (application)
    {
        this->operator[](application) += application->TotalLoads;
    }
}

void ApplicationTotalLoad::AddSbLoad(NodeEntry const* node, PartitionEntry const* partition)
{
    for(auto nodeEntry : partition->StandByLocations)
    {
        if (node == nodeEntry)
        {
            LoadEntry const& standByReplicaLoad = partition->GetReplicaEntry(ReplicaRole::Enum::StandBy, true /* useNodeId */, nodeEntry->NodeId);
            LoadEntry& appLoad = this->operator[](partition->Service->Application);
            for (size_t i = 0; i < partition->Service->MetricCount; i++)
            {
                int64 metricValue = standByReplicaLoad.Values[i];
                if (metricValue != 0)
                {
                    size_t globalIndex = partition->Service->GlobalMetricIndices[i] - (totalMetricCount_ - appLoad.Values.size());
                    appLoad.AddLoad(globalIndex, metricValue);
                }
            }
            break;
        }
    }
}

void ApplicationTotalLoad::DeleteSbLoad(NodeEntry const* node, PartitionEntry const* partition)
{
    for(auto nodeEntry : partition->StandByLocations)
    {
        if (node == nodeEntry)
        {
            LoadEntry const& standByReplicaLoad = partition->GetReplicaEntry(ReplicaRole::Enum::StandBy, true /* useNodeId */, nodeEntry->NodeId);
            LoadEntry& appLoad = this->operator[](partition->Service->Application);
            for (size_t i = 0; i < partition->Service->MetricCount; i++)
            {
                int64 metricValue = standByReplicaLoad.Values[i];
                if (metricValue != 0)
                {
                    size_t globalIndex = partition->Service->GlobalMetricIndices[i] - (totalMetricCount_ - appLoad.Values.size());
                    appLoad.AddLoad(globalIndex, -metricValue);
                }
            }
            break;
        }
    }
}

void ApplicationTotalLoad::AddLoad(ApplicationEntry const* application, PartitionEntry const* partition, LoadEntry const* load)
{
    if(application && partition && load)
    {
        LoadEntry& appLoad = this->operator[](application);
        for (size_t i = 0; i < partition->Service->MetricCount; i++)
        {
            int64 metricValue = load->Values[i];
            if (metricValue != 0)
            {
                size_t globalIndex = partition->Service->GlobalMetricIndices[i] - (totalMetricCount_ - appLoad.Values.size());
                appLoad.AddLoad(globalIndex, metricValue);
            }
        }
    }
}

void ApplicationTotalLoad::DeleteLoad(ApplicationEntry const* application, PartitionEntry const* partition, LoadEntry const* load)
{
    if(application && partition && load)
    {
        LoadEntry& appLoad = this->operator[](application);
        for (size_t i = 0; i < partition->Service->MetricCount; i++)
        {
            int64 metricValue = load->Values[i];
            if (metricValue != 0)
            {
                size_t globalIndex = partition->Service->GlobalMetricIndices[i] - (totalMetricCount_ - appLoad.Values.size());
                appLoad.AddLoad(globalIndex, -metricValue);
            }
        }
    }
}
