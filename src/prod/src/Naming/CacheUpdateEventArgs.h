// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class CacheUpdateEventArgs : public Common::EventArgs
    {
    public:
        CacheUpdateEventArgs(
            ResolvedServicePartitionSPtr const & update, 
            Transport::FabricActivityHeader && activityHeader)
          : partition_(update)
          , failure_()
          , activityHeader_(std::move(activityHeader))
        {
        }

        CacheUpdateEventArgs(
            AddressDetectionFailureSPtr const & failure,
            Transport::FabricActivityHeader && activityHeader)
          : partition_()
          , failure_(failure)
          , activityHeader_(std::move(activityHeader))
        {
        }

        __declspec(property(get=get_Partition)) ResolvedServicePartitionSPtr const & Partition;
        inline ResolvedServicePartitionSPtr const & get_Partition() const { return partition_; }

        __declspec(property(get=get_Failure)) AddressDetectionFailureSPtr const & Failure;
        inline AddressDetectionFailureSPtr const & get_Failure() const { return failure_; }

        __declspec(property(get=get_ActivityHeader)) Transport::FabricActivityHeader const & ActivityHeader;
        inline Transport::FabricActivityHeader const & get_ActivityHeader() const { return activityHeader_; }

    private:
        ResolvedServicePartitionSPtr partition_;
        AddressDetectionFailureSPtr failure_;
        Transport::FabricActivityHeader activityHeader_;
    };
}
