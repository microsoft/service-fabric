// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        struct EventContextMap
            : public Serialization::FabricSerializable
            , public Common::IFabricJsonSerializable
        {
        public:
            EventContextMap();
            EventContextMap(EventContextMap const & other) = default;
            EventContextMap(EventContextMap && other) = default;
            EventContextMap & operator = (EventContextMap const & other) = default;
            EventContextMap & operator = (EventContextMap && other) = default;

            Common::ErrorCode FromPublicApi(
                FABRIC_EVENT_CONTEXT_MAP const & publicEventContextMap);

            static Common::ErrorCode FromPublicApiMap(
                FABRIC_EVENT_CONTEXT_MAP const & publicEventContextMap,
                __inout std::map<std::wstring, std::wstring> & map);

            Common::ErrorCode ToPublicApi(
                __in Common::ScopedHeap & heap,
                __out FABRIC_EVENT_CONTEXT_MAP & publicEventContextMap) const;

            FABRIC_FIELDS_01(map_);

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
              SERIALIZABLE_PROPERTY_SIMPLE_MAP(ServiceModel::Constants::Map, map_)
            END_JSON_SERIALIZABLE_PROPERTIES()

        private:
            std::map<std::wstring, std::wstring> map_;
        };
    }
}
