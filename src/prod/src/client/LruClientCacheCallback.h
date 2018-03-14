// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    // Notifies cache updates to service location notification feature.
    // Callbacks are registered per-name and ref-counted by the number of 
    // trackers interested in notifications for the same service name.
    //
    class LruClientCacheCallback
    {
        DENY_COPY(LruClientCacheCallback)

    public:
        typedef std::function<void(
            Naming::ResolvedServicePartitionSPtr const &,
            Transport::FabricActivityHeader const &)> 
            RspUpdateCallback; 

    public:
        explicit LruClientCacheCallback(RspUpdateCallback const & callback) 
            : callback_(callback) 
            , refcount_(1)
        { }

        LruClientCacheCallback & operator() (Naming::ResolvedServicePartitionSPtr const & rsp)
        {
            callback_(rsp, Transport::FabricActivityHeader(Common::ActivityId()));
            return *this;
        }

        LONG IncrementRefCount() { return refcount_++; }
        LONG DecrementRefCount() { return refcount_--; }

    private:
        RspUpdateCallback callback_;
        Common::atomic_long refcount_;
    };

    typedef std::shared_ptr<LruClientCacheCallback> LruClientCacheCallbackSPtr;
}
