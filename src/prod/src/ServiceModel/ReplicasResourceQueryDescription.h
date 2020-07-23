// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class ReplicasResourceQueryDescription
        : public Serialization::FabricSerializable
    {
        DENY_COPY(ReplicasResourceQueryDescription)
    public:
        ReplicasResourceQueryDescription() = default;

        ReplicasResourceQueryDescription(
            Common::NamingUri && applicationUri,
            Common::NamingUri && serviceName);

        ReplicasResourceQueryDescription(
            Common::NamingUri && applicationUri,
            Common::NamingUri && serviceName,
            int64 replicaId);

        __declspec(property(get=get_ApplicationUri)) Common::NamingUri const & ApplicationUri;
        Common::NamingUri const & get_ApplicationUri() const { return applicationUri_; }

        __declspec(property(get=get_ServiceName)) Common::NamingUri const & ServiceName;
        Common::NamingUri const & get_ServiceName() const { return serviceName_; }

        __declspec(property(get=get_ReplicaId)) int64 ReplicaId;
        int64 get_ReplicaId() const { return replicaId_; }

        __declspec(property(get = get_QueryPagingDescription, put = set_QueryPagingDescription)) QueryPagingDescription const & PagingDescription;
        QueryPagingDescription const & get_QueryPagingDescription() const { return queryPagingDescription_; }
        void set_QueryPagingDescription(QueryPagingDescription && queryPagingDescription) { queryPagingDescription_ = std::move(queryPagingDescription); }

    private:
        Common::NamingUri applicationUri_;
        Common::NamingUri serviceName_;
        int64 replicaId_;
        QueryPagingDescription queryPagingDescription_;
    };
}
