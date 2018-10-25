// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class NodeHealthStateFilter
        : public Common::IFabricJsonSerializable
        , public Serialization::FabricSerializable
    {
        DENY_COPY(NodeHealthStateFilter)

    public:
        NodeHealthStateFilter();

        explicit NodeHealthStateFilter(
            DWORD healthStateFilter);

        NodeHealthStateFilter(NodeHealthStateFilter && other) = default;
        NodeHealthStateFilter & operator = (NodeHealthStateFilter && other) = default;

        virtual ~NodeHealthStateFilter();

        __declspec(property(get=get_HealthStateFilter)) DWORD HealthStateFilter;
        DWORD get_HealthStateFilter() const { return healthStateFilter_; }

        __declspec(property(get=get_NodeNameFilter,put=set_NodeNameFilter)) std::wstring const & NodeNameFilter;
        std::wstring const & get_NodeNameFilter() const { return nodeNameFilter_; }
        void set_NodeNameFilter(std::wstring const & value) { nodeNameFilter_ = value; }

        std::wstring ToJsonString() const;
        static Common::ErrorCode FromJsonString(std::wstring const & str, __inout NodeHealthStateFilter & filter);

        bool IsRespected(FABRIC_HEALTH_STATE healthState) const;

        Common::ErrorCode FromPublicApi(FABRIC_NODE_HEALTH_STATE_FILTER const & publicNodeHealthStateFilter);

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_NODE_HEALTH_STATE_FILTER & publicFilter) const;

        FABRIC_FIELDS_02(healthStateFilter_, nodeNameFilter_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::HealthStateFilter, healthStateFilter_)
            SERIALIZABLE_PROPERTY(Constants::NodeNameFilter, nodeNameFilter_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        DWORD healthStateFilter_;
        std::wstring nodeNameFilter_;
    };

    using NodeHealthStateFilterList = std::vector<NodeHealthStateFilter>;
    using NodeHealthStateFilterListWrapper = HealthStateFilterList<NodeHealthStateFilter, FABRIC_NODE_HEALTH_STATE_FILTER, FABRIC_NODE_HEALTH_STATE_FILTER_LIST>;
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::NodeHealthStateFilter)
