// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Management/FaultAnalysisService/FaultAnalysisServiceConfig.h"

namespace Management
{
    namespace FaultAnalysisService
    {
        class ChaosTargetFilter
            : public Serialization::FabricSerializable
            , public Common::IFabricJsonSerializable
        {
        public:

            ChaosTargetFilter();
            ChaosTargetFilter(ChaosTargetFilter const &) = default;
            ChaosTargetFilter(ChaosTargetFilter &&) = default;

            __declspec(property(get = get_NodeTypeInclusionList)) Common::StringCollection const & NodeTypeInclusionList;
            Common::StringCollection const & get_NodeTypeInclusionList() const { return nodeTypeInclusionList_; }

            __declspec(property(get = get_ApplicationInclusionList)) Common::StringCollection const & ApplicationInclusionList;
            Common::StringCollection const & get_ApplicationInclusionList() const { return applicationInclusionList_; }

            Common::ErrorCode FromPublicApi(FABRIC_CHAOS_TARGET_FILTER const &);
            Common::ErrorCode ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_CHAOS_TARGET_FILTER &) const;

            Common::ErrorCode Validate() const;

            FABRIC_FIELDS_02(
                nodeTypeInclusionList_,
                applicationInclusionList_);

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::NodeTypeInclusionList, nodeTypeInclusionList_);
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::ApplicationInclusionList, applicationInclusionList_);
            END_JSON_SERIALIZABLE_PROPERTIES()

        private:

            Common::StringCollection nodeTypeInclusionList_;
            Common::StringCollection applicationInclusionList_;
        };
    }
}
