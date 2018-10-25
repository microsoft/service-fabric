// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TraceHelpers.h"
#include "PlacementAndLoadBalancing.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;


template <typename T>
struct TraceLoadsFilter
{
    static bool Enabled(T const & item)
    {
        return true;
    }
};

template <>
struct TraceLoadsFilter<Node>
{
    static bool Enabled(Node const & node)
    {
        return node.NodeDescriptionObj.IsUpAndActivated;
    }
};

template <>
struct TraceLoadsFilter<Application>
{
    static bool Enabled(Application const & app)
    {
        return app.Services.size() != 0 && app.ApplicationDesc.HasScaleoutOrCapacity();
    }
};

template <typename Identifier, typename Item, typename ItemStatus, typename ItemMap, typename ItemStatusMap> 
void TraceLoads<Identifier, Item, ItemStatus, ItemMap, ItemStatusMap>::WriteTo(Common::TextWriter& writer, Common::FormatOptions const&) const
{
    if (itemMap_.empty()) { return; }

    //Trace iteration#...
    if (iteration_ != 1)
    {
        writer.WriteLine("Tracing iteration {0}", iteration_);
    }

    //Printing out the Header Line as {Item Status:(ListofstatusDesriptors)}...
    writer.Write("{0} {1}:(",*(Item::FormatHeader),*(ItemStatus::FormatHeader));
    for (auto itStatus = itemStatusMap_.begin(); itStatus!= itemStatusMap_.end(); ++itStatus)
    {
        if (statusHeader_)
        {
            if (itStatus != itemStatusMap_.begin())
            {
                writer.Write(" ");
            }
            writer.Write("{0}", itStatus->first);
        }
    }
    writer.WriteLine(")");

    //setup counters...
    size_t noOfItemsTraced(0);
    auto itemId = *itemPtr_;
    auto itItem = itemMap_.find(*itemPtr_);

    //Tracing out Status Info...
    while (itItem != itemMap_.end())
    {
        auto const& item = itItem->second;
        if (TraceLoadsFilter<Item>::Enabled(item))
        {
            string itemStatusLine;
            StringWriterA w(itemStatusLine);
            bool isToBeTraced = false;

            w.Write("{0}:{1} (", itItem->first, itItem->second);
            for (auto itStatus = itemStatusMap_.begin(); itStatus != itemStatusMap_.end(); ++itStatus)
            {
                isToBeTraced |= itStatus->second.WriteFor(itItem->second, w);
            }
            w.WriteLine(")");
            if (isToBeTraced) { writer.Write(itemStatusLine); }
            noOfItemsTraced++;
        }

        ++itItem;

        //Break condition is obvious....
        if (noOfItemsTraced >= maxEntries_ && itItem != itemMap_.end())
        {
            writer.WriteLine("Traced out status of {0} entries; next Trace will start with Entry {1}...", noOfItemsTraced, itItem->first);
            *itemPtr_ = itItem->first;
            break;
        }
    }

    //We will reset itemPtr so that the outer loop knows that we have finished tracing...
    if (itemId != itemMap_.begin()->first && itItem == itemMap_.end())
    {
        writer.WriteLine("Traced out loads for last {0} entries; next trace will start from begining of table.", noOfItemsTraced);
        *itemPtr_ = itemMap_.begin()->first;
    }
}

template <typename Identifier, typename Item, typename ItemStatus, typename ItemMap, typename ItemStatusMap>
void TraceLoads<Identifier, Item, ItemStatus, ItemMap, ItemStatusMap>::WriteToEtw(uint16 contextSequenceId) const
{
    PlacementAndLoadBalancing::PLBTrace->PLBDump(contextSequenceId, wformatString(*this));
}

template <typename Identifier, typename Item, typename ItemStatus, typename ItemMap, typename ItemStatusMap>
void TraceLoadsVector<Identifier, Item, ItemStatus, ItemMap, ItemStatusMap>::WriteTo(Common::TextWriter& writer, Common::FormatOptions const&) const
{
    if (itemMap_.empty()) { return; }

    //Trace iteration#...
    if (iteration_ != 1)
    {
        writer.WriteLine("Tracing iteration {0}", iteration_);
    }

    //Printing out the Header Line as {Item Status:(ListofstatusDesriptors)}...
    writer.Write("{0} {1}:(", *(Item::FormatHeader), *(ItemStatus::FormatHeader));
    for (auto itStatus = itemStatusMap_.begin(); itStatus != itemStatusMap_.end(); ++itStatus)
    {
        if (itStatus != itemStatusMap_.begin())
        {
            writer.Write(" ");
        }
        if (statusHeader_)
        {
            writer.Write("{0}", itStatus->first);
        }
    }
    writer.WriteLine(")");

    //setup counters...
    size_t noOfItemsTraced(0);

    //Tracing out Status Info...
    while (*itemPtr_ < itemMap_.size())
    {
        if (TraceLoadsFilter<Item>::Enabled(itemMap_[*itemPtr_]))
        {
            string itemStatusLine;
            StringWriterA w(itemStatusLine);
            bool isToBeTraced = false;

            w.Write("{0} (", itemMap_[*itemPtr_]);
            for (auto itStatus = itemStatusMap_.begin(); itStatus != itemStatusMap_.end(); ++itStatus)
            {
                isToBeTraced |= itStatus->second.WriteFor(itemMap_[*itemPtr_], w);
            }
            w.WriteLine(")");
            if (isToBeTraced) { writer.Write(itemStatusLine); }
            noOfItemsTraced++;
        }

        //Break condition is obvious....
        if (noOfItemsTraced >= maxEntries_)
        {
            writer.WriteLine("Traced out status of {0} entries; next Trace will start with Entry {1}...", noOfItemsTraced, *itemPtr_);
            break;
        }
        (*itemPtr_)++;
    }

    //We will reset itemPtr so that the outer loop knows that we have finished tracing...
    if (*itemPtr_ >= itemMap_.size())
    {
        writer.WriteLine("Traced out loads for last {0} entries; next trace will start from begining of table.", noOfItemsTraced);
        *itemPtr_ = 0;
    }
}

template <typename Identifier, typename Item, typename ItemStatus, typename ItemMap, typename ItemStatusMap>
void TraceLoadsVector<Identifier, Item, ItemStatus, ItemMap, ItemStatusMap>::WriteToEtw(uint16 contextSequenceId) const
{
    PlacementAndLoadBalancing::PLBTrace->PLBDump(contextSequenceId, wformatString(*this));
}

template <typename T>
struct TraceDescriptionsFilter
{
    static bool Enabled(T const &)
    {
        return true;
    }
};

template <>
struct TraceDescriptionsFilter<Node>
{
    static bool Enabled(Node const & node)
    {
        return node.NodeDescriptionObj.IsUp;
    }
};

template <>
struct TraceDescriptionsFilter<ServiceType>
{
    static bool Enabled(ServiceType const & servType)
    {
        return !servType.ServiceTypeDesc.BlockList.empty();
    }
};

template <typename T, typename Description>
struct TraceDescriptionsDescriptor
{
    static Description const & Get(T const & item);
    static bool IsServiceDescription();
};

template <>
struct TraceDescriptionsDescriptor<Node, NodeDescription>
{
    static NodeDescription const & Get(Node const & node)
    {
        return node.NodeDescriptionObj;
    }
    static bool IsServiceDescription()
    {
        return false;
    }
};

template <>
struct TraceDescriptionsDescriptor<Application, ApplicationDescription>
{
    static ApplicationDescription const & Get(Application const & app)
    {
        return app.ApplicationDesc;
    }
    static bool IsServiceDescription()
    {
        return false;
    }
};

template <>
struct TraceDescriptionsDescriptor<ServiceType, ServiceTypeDescription>
{
    static ServiceTypeDescription const & Get(ServiceType const & serviceType)
    {
        return serviceType.ServiceTypeDesc;
    }
    static bool IsServiceDescription()
    {
        return false;
    }
};

template <>
struct TraceDescriptionsDescriptor<const Service*, ServiceDescription>
{
    static ServiceDescription const & Get(const Service* const & service)
    {
        return service->ServiceDesc;
    }
    static bool IsServiceDescription()
    {
        return true;
    }
};

template<typename T>
struct TraceDistributionLogicIndex
{
    static int Get(T const & )
    {
        return 0;
    }
};

template<>
struct TraceDistributionLogicIndex<int>
{
    static int Get(int const & index)
    {
        return index;
    }
};

template <typename Identifier, typename Item, typename Description, typename ItemMap, typename ItemVector>
void TraceDescriptions<Identifier, Item, Description, ItemMap, ItemVector>::WriteTo(Common::TextWriter& writer, Common::FormatOptions const&) const
{
    if (itemMap_.empty()) { return; }

    //Printing out the Header Line as {ItemDescriptor}...
    writer.WriteLine("{0}", *(Description::FormatHeader));

    //setup counters...
    size_t noOfItemsTraced(0);
    auto itemId = *itemPtr_;
    auto itItem = itemMap_.find(*itemPtr_);

    //Tracing out Status Info...
    while (itItem != itemMap_.end())
    {
        auto const& item = itItem->second;
        Description const& desc = TraceDescriptionsDescriptor<Item, Description>::Get(item);

        if (TraceDescriptionsFilter<Item>::Enabled(item))
        {
            if (TraceDescriptionsDescriptor<Item, Description>::IsServiceDescription())
            {
                int itemDistributionLogicIndex = TraceDistributionLogicIndex<Identifier>::Get(itItem->first);
                writer.WriteLine("{0} QuorumBasedLogic:{1}",
                    desc, itemQuorumDomainLogic_[itemDistributionLogicIndex] ? true : false);
            }
            else
            {
                writer.WriteLine("{0}", desc);
            }
            ++noOfItemsTraced;
        }

        //increment counter...
        ++itItem;

        //Break condition is obvious....
        if (noOfItemsTraced >= maxEntries_ && itItem != itemMap_.end())
        {
            writer.WriteLine("Traced out status of {0} entries; next Trace will start with Entry id: {1}...", noOfItemsTraced, itItem->first);
            *itemPtr_ = itItem->first;
            break;
        }
    }

    //We will reset itemPtr so that the outer loop knows that we have finished tracing...
    if (itemId != itemMap_.begin()->first && itItem == itemMap_.end())
    {
        writer.WriteLine("Traced out status of {0} entries; next Trace will start from begining of Table.", noOfItemsTraced);
        *itemPtr_ = itemMap_.begin()->first;
    }
}

template <typename Identifier, typename Item, typename Description, typename ItemMap, typename ItemVector>
void TraceDescriptions<Identifier, Item, Description, ItemMap, ItemVector>::WriteToEtw(uint16 contextSequenceId) const
{
    PlacementAndLoadBalancing::PLBTrace->PLBDump(contextSequenceId, wformatString(*this));
}

template <typename Item, typename Description>
void TraceVectorDescriptions<Item, Description>::WriteTo(Common::TextWriter& writer, Common::FormatOptions const&) const
{
    if (itemVector_.empty()) { return; }

    //Printing out the Header Line as {ItemDescriptor}...
    writer.WriteLine("{0}", *(Description::FormatHeader));

    //setup counters...
    size_t noOfItemsTraced(0);

    //Tracing out Status Info...
    while (*itemPtr_ != itemVector_.size())
    {
        auto const& item = itemVector_[*itemPtr_];
        Description const& desc = TraceDescriptionsDescriptor<Item, Description>::Get(item);

        if (TraceDescriptionsFilter<Item>::Enabled(item))
        {
            writer.WriteLine("{0}", desc);
            ++noOfItemsTraced;
        }

        //Break condition is obvious....
        if (noOfItemsTraced >= maxEntries_ && *itemPtr_ != itemVector_.size())
        {
            writer.WriteLine("Traced out status of {0} entries; next Trace will start with Entry id: {1}...", noOfItemsTraced, *itemPtr_);
            break;
        }

        (*itemPtr_)++;
    }

    //We will reset itemPtr so that the outer loop knows that we have finished tracing...
    if (*itemPtr_ >= itemVector_.size())
    {
        writer.WriteLine("Traced out status of {0} entries; next Trace will start from begining of Table.", noOfItemsTraced);
        *itemPtr_ = 0;
    }
}

template <typename Item, typename Description>
void TraceVectorDescriptions<Item, Description>::WriteToEtw(uint16 contextSequenceId) const
{
    PlacementAndLoadBalancing::PLBTrace->PLBDump(contextSequenceId, wformatString(*this));
}

void  TraceAllBalancingMetricInfo::WriteTo(Common::TextWriter& writer, Common::FormatOptions const&) const
{
    if (items_.empty())
    {
        return;
    }

    writer.WriteLine("MetricName: StdDev Average TotalLoad ActivityThreshold BalancingThreshold");
    for (auto item : items_)
    {
        writer.WriteLine("{0}: {1} {2} {3} {4} {5}",
            item.first,
            item.second.avgStdDev_,
            item.second.averageLoad_,
            item.second.totalClusterLoad_,
            item.second.activityThreshold_,
            item.second.balancingThreshold_);
    }
}

void TraceAllBalancingMetricInfo::WriteToEtw(uint16 contextSequenceId) const
{
    PlacementAndLoadBalancing::PLBTrace->PLBDump(contextSequenceId, wformatString(*this));
}

void TraceDefragMetricInfo::WriteTo(Common::TextWriter& writer, Common::FormatOptions const&) const
{
    writer.WriteLine(metricName);

    writer.WriteLine(
        "DefragInfo: Type:{0}, TargetEmptyNodeCount:{1}, EmptyNodesDistribution:{2}, EmptyNodeLoadThreshold:{3}, EmptyNodesWeight:{4}, ActivityThreshold:{5}, BalancingThreshold:{6}, ReservationLoad:{7}, NonEmptyNodesWeight:{8}",
        type_,
        targetEmptyNodeCount_,
        emptyNodesDistribution_,
        emptyNodeLoadThreshold_,
        emptyNodesWeight_,
        activityThreshold_,
        balancingThreshold_,
        reservationLoad_,
        nonEmptyNodesWeight_);

    writer.WriteLine(
        "ClusterMetricInfo: StdDev:{0}, FdStdDev:{1}, UdStdDev:{2}, EmptyNodes:{3}, NonEmptyAverageLoad:{4}, Average:{5}, FdAverage:{6}, UdAverage:{7}, TotalLoad:{8}",
        avgStdDev_,
        avgStdDevFaultDomain_,
        avgStdDevUpgradeDomain_,
        emptyNodeCnt_,
        nonEmptyNodeLoad_,
        averageLoad_,
        averageFdLoad_,
        averageUdLoad_,
        totalClusterLoad_);

    if (faultDomainItems_.size() > 0)
    {
        writer.WriteLine("Fault domain distribution: DomainName EmptyNodes NonEmptyAverageLoad");
        for (auto faultDomainInfo : faultDomainItems_)
        {
            writer.WriteLine("{0} {1} {2}", faultDomainInfo.first, faultDomainInfo.second.first, faultDomainInfo.second.second);
        }
    }

    if (upgradeDomainItems_.size() > 0)
    {
        writer.WriteLine("Upgrade domain distribution: DomainName EmptyNodes NonEmptyAverageLoad");
        for (auto upgradeDomainInfo : upgradeDomainItems_)
        {
            writer.WriteLine("{0} {1} {2}", upgradeDomainInfo.first, upgradeDomainInfo.second.first, upgradeDomainInfo.second.second);
        }
    }
}

void TraceDefragMetricInfo::WriteToEtw(uint16 contextSequenceId) const
{
    PlacementAndLoadBalancing::PLBTrace->PLBDump(contextSequenceId, wformatString(*this));
}

// There needs to be an entry for every instantiation here...
// it's just to avoid link error.

template class Reliability::LoadBalancingComponent::TraceLoads <uint64, Application, ApplicationLoad, Uint64UnorderedMap<Application>, Uint64UnorderedMap<ApplicationLoad>> ;
template class Reliability::LoadBalancingComponent::TraceLoadsVector < uint64, Node, ServiceDomainMetric, vector<Node>, map<wstring, ServiceDomainMetric> > ;
//Same for this...

template class Reliability::LoadBalancingComponent::TraceVectorDescriptions<Node, NodeDescription>;
template class Reliability::LoadBalancingComponent::TraceDescriptions<uint64, Application, ApplicationDescription, Uint64UnorderedMap<Application>>;
template class Reliability::LoadBalancingComponent::TraceDescriptions<std::wstring, ServiceType, ServiceTypeDescription>;
template class Reliability::LoadBalancingComponent::TraceDescriptions<int, const Service*, ServiceDescription>;
