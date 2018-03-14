// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    // This class encapsulates special parsing that occurs on service group
    // RSPs to extract service group member RSPs. The parsed results
    // are lazily cached for each member RSP.
    //
    class ResolvedServicePartitionCacheEntry 
        : public Common::TextTraceComponent<Common::TraceTaskCodes::Client>
    {
        DENY_COPY(ResolvedServicePartitionCacheEntry);

    public:
        ResolvedServicePartitionCacheEntry(Naming::ResolvedServicePartitionSPtr const &);

        __declspec(property(get=get_UnparsedRsp)) Naming::ResolvedServicePartitionSPtr const & UnparsedRsp;
        Naming::ResolvedServicePartitionSPtr const & get_UnparsedRsp() const { return rsp_; }

        // FullName is used as a hint to parse service group member RSPs
        // from the original RSP if necessary
        //
        bool TryGetParsedRsp(
            Common::NamingUri const & fullName,
            __out Naming::ResolvedServicePartitionSPtr &);

    private:
        Naming::ResolvedServicePartitionSPtr rsp_;

        std::map<std::wstring, Naming::ResolvedServicePartitionSPtr> memberRsps_;
        Common::RwLock memberRspsLock_;
    };

    typedef std::shared_ptr<ResolvedServicePartitionCacheEntry> ResolvedServicePartitionCacheEntrySPtr;
}
