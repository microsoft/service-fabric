// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "IViolation.h"
#include "PlacementReplica.h"
#include "PartitionEntry.h"
#include "NodeEntry.h"
#include "ApplicationEntry.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

size_t ReplicaSetViolation::GetCount() const
{
    return invalidReplicas_.size();
}

bool ReplicaSetViolation::IsEmpty() const
{
    return invalidReplicas_.empty();
}

bool ReplicaSetViolation::operator<=(ReplicaSetViolation const& other) const
{
    PlacementReplicaPointerCompare comparator;
    return std::includes(other.invalidReplicas_.begin(), other.invalidReplicas_.end(), invalidReplicas_.begin(), invalidReplicas_.end(), comparator);
}

ViolationRelation::Enum ReplicaSetViolation::CompareViolation(ReplicaSetViolation const& other) const
{
    if (invalidReplicas_.size() > other.invalidReplicas_.size())
    {
        return ViolationRelation::Enum::Greater;
    }
    else if (invalidReplicas_.size() < other.invalidReplicas_.size())
    {
        return ViolationRelation::Enum::Smaller;
    }
    else
    {
        return ViolationRelation::Enum::Equal;
    }
}

vector<wstring> ReplicaSetViolation::GetViolationListDetail()
{
    typedef std::map<Common::Guid, vector<PlacementReplica const*>> PlacementReplicaMap;

    vector<wstring> violationListDetail;
    PlacementReplicaMap replicasGroupedByPartitionId;

    for (const auto replica : invalidReplicas_ )
    {
        replicasGroupedByPartitionId[replica->Partition->PartitionId].push_back(replica);
    }

    for (const auto & replicasGroup : replicasGroupedByPartitionId)
    {
        wstringstream replicasDetail;
        replicasDetail << L"partition:" << replicasGroup.first.ToString() << L" replicas:";
        for (const auto & replica : replicasGroup.second)
        {
            auto nodeInfo = replica->Node != nullptr ? replica->Node->NodeId : Federation::NodeId(LargeInteger::Zero);
            auto roleInfo = replica->Role;
            wstring replicaDetail;
            StringWriter w(replicaDetail);
            w.Write("{{node:{0} role:{1}}", nodeInfo, roleInfo);
            replicasDetail << replicaDetail;
        }
        violationListDetail.push_back(replicasDetail.str());
    }
    return violationListDetail;
}

size_t PartitionSetViolation::GetCount() const
{
    return invalidPartitions_.size();
}

bool PartitionSetViolation::IsEmpty() const
{
    return invalidPartitions_.empty();
}

bool PartitionSetViolation::operator<=(PartitionSetViolation const& other) const
{
    return std::includes(other.invalidPartitions_.begin(), other.invalidPartitions_.end(), invalidPartitions_.begin(), invalidPartitions_.end());
}

ViolationRelation::Enum PartitionSetViolation::CompareViolation(PartitionSetViolation const& other) const
{
    if (invalidPartitions_.size() > other.invalidPartitions_.size())
    {
        return ViolationRelation::Enum::Greater;
    }
    else if (invalidPartitions_.size() < other.invalidPartitions_.size())
    {
        return ViolationRelation::Enum::Smaller;
    }
    else
    {
        return ViolationRelation::Enum::Equal;
    }
}


vector<wstring> PartitionSetViolation::GetViolationListDetail()
{
    vector<wstring> violationListDetail;
    for (const auto & invalidPartition : invalidPartitions_)
    {
        violationListDetail.push_back(L"partition:" + invalidPartition->PartitionId.ToString());
    }
    return violationListDetail;
}

size_t NodeLoadViolation::GetCount() const
{
    if (totalLoadOverCapacity_ < SIZE_MAX)
    {
        return static_cast<size_t>(totalLoadOverCapacity_);
    }
    else
    {
        return SIZE_MAX;
    }
}

bool NodeLoadViolation::IsEmpty() const
{
    return nodeLoadOverCapacity_.empty();
}

bool NodeLoadViolation::operator<=(NodeLoadViolation const& other) const
{
    bool ret = true;
    auto it1 = nodeLoadOverCapacity_.begin();
    auto it2 = other.nodeLoadOverCapacity_.begin();

    while (it1 != nodeLoadOverCapacity_.end() && it2 != other.nodeLoadOverCapacity_.end())
    {
        if (it1->first < it2->first)
        {
            ret = false;
            break;
        }
        else if (it1->first > it2->first)
        {
            ++it2;
            continue;
        }
        else if (it1->second <= it2->second)
        {
            ++it1;
            ++it2;
            continue;
        }
        else
        {
            ret = false;
            break;
        }
    }

    if (ret && it1 != nodeLoadOverCapacity_.end())
    {
        ret = false;
    }

    return ret;
}

ViolationRelation::Enum NodeLoadViolation::CompareViolation(NodeLoadViolation const& other) const
{
    if (totalLoadOverCapacity_ > other.totalLoadOverCapacity_)
    {
        return ViolationRelation::Enum::Greater;
    }
    else if (totalLoadOverCapacity_ < other.totalLoadOverCapacity_)
    {
        return ViolationRelation::Enum::Smaller;
    }
    else
    {
        return ViolationRelation::Enum::Equal;
    }
}

vector<wstring> NodeLoadViolation::GetViolationListDetail()
{
    vector<wstring> violationListDetail;
    for (const auto & node : nodeLoadOverCapacity_)
    {
        wstringstream loads;

        vector<int64> const& values = node.second.Values;
 
        for (int i = 0; i < values.size(); i++)
        {
            // Only metric values that were in violation have been set, no need to print 0s
            if (values[i] != 0)
            {
                if (globalDomainEntryPointer_ != nullptr)
                {
                    loads << wformatString("{0}:{1}", globalDomainEntryPointer_->Metrics[i].Name, values[i]);
                }
                else
                {
                    loads << wformatString("{0}:{1}", i, values[i]);
                }
                loads << " ";
            }
        }
        violationListDetail.push_back(L"node:" + node.first->NodeId.ToString() + L", load:" + loads.str());
    }
    return violationListDetail;
}

size_t ScaleoutCountViolation::GetCount() const
{
    // return the number of invalid nodes for comparing scaleout count violations
    size_t numInvalidNodes = 0;
    for (auto itAppEntry = appInvalidNodes_.begin(); itAppEntry != appInvalidNodes_.end(); itAppEntry++)
    {
        numInvalidNodes += itAppEntry->second.size();
    }

    return numInvalidNodes;
}

bool ScaleoutCountViolation::IsEmpty() const
{
    return appInvalidNodes_.empty();
}

bool ScaleoutCountViolation::operator<=(ScaleoutCountViolation const& other) const
{
    if (appInvalidNodes_.size() > other.appInvalidNodes_.size())
    {
        return false;
    }

    for (auto it = appInvalidNodes_.begin(); it != appInvalidNodes_.end(); ++it)
    {
        auto itOther = other.appInvalidNodes_.find(it->first);
        // if there is new application violation or existing violated app has more nodes violations
        if (itOther == other.appInvalidNodes_.end() || it->second.size() > itOther->second.size())
        {
            return false;
        }
    }

    return true;
}

// Scaleout violation is lower if one of the following conditions are satisfied:
//  - the number of applications in scaleout violation is lower
//  - the number of applications in scaleout violation is the same,
//    but there is an application which has lower number of nodes in violation
//    (and no other application has more number of nodes in violation)
//  - the number of applications in scaleout violation is the same,
//    and there is no application which has lower number of nodes in violation,
//    but total number of replicas in violation is lower
//    (and no other application has more number of nodes in violation)
ViolationRelation::Enum ScaleoutCountViolation::CompareViolation(ScaleoutCountViolation const& other) const
{
    // The number of applications in scaleout violation is greater
    if (appInvalidNodes_.size() > other.appInvalidNodes_.size())
    {
        return ViolationRelation::Enum::Greater;
    }
    // The number of applications in scaleout violation is lower
    else if (appInvalidNodes_.size() < other.appInvalidNodes_.size())
    {
        return ViolationRelation::Enum::Smaller;
    }
    // The number of applications that are in scaleout violation are the same
    else
    {
        bool hasImprovement = false;
        bool hasDegradation = false;
        for (auto itAppInvalidNodes = appInvalidNodes_.begin(); itAppInvalidNodes != appInvalidNodes_.end(); ++itAppInvalidNodes)
        {
            auto itOtherAppInvalidNodes = other.appInvalidNodes_.find(itAppInvalidNodes->first);

            // If the application hasn't been in the violation, the solutions is worse
            // (and there is potential issue with CorrectSolution function)
            if (itOtherAppInvalidNodes == other.appInvalidNodes_.end())
            {
                return ViolationRelation::Enum::Greater;
            }

            if (itAppInvalidNodes->second.size() < itOtherAppInvalidNodes->second.size())
            {
                hasImprovement = true;
            }
            else if (itAppInvalidNodes->second.size() > itOtherAppInvalidNodes->second.size())
            {
                hasDegradation = true;
            }
        }

        // If there is an application with more nodes in violation,
        // then violation is greater
        if (hasDegradation)
        {
            return ViolationRelation::Enum::Greater;
        }
        // If there is an application with less nodes in violation,
        // and non of the applications is in worse state,
        // then violation is lower
        else if (hasImprovement)
        {
            return ViolationRelation::Enum::Smaller;
        }
        // If there were no changes in the number of nodes in violation,
        // compare the total number of replicas in violation
        else
        {
            if (invalidReplicas_.size() > other.invalidReplicas_.size())
            {
                return ViolationRelation::Enum::Greater;
            }
            else if (invalidReplicas_.size() < other.invalidReplicas_.size())
            {
                return ViolationRelation::Enum::Smaller;
            }
            else
            {
                return ViolationRelation::Enum::Equal;
            }
        }
    }
}


vector<wstring> ScaleoutCountViolation::GetViolationListDetail()
{
    vector<wstring> violationListDetail;
    for (const auto & app : appInvalidNodes_)
    {
        violationListDetail.push_back(L"application:" + app.first->Name);
    }
    return violationListDetail;
}

