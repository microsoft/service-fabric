// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    struct ApplicationHealthPolicyMap 
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
    public:
        ApplicationHealthPolicyMap();

        ApplicationHealthPolicyMap(ApplicationHealthPolicyMap const & other) = default;
        ApplicationHealthPolicyMap & operator = (ApplicationHealthPolicyMap const & other) = default;

        ApplicationHealthPolicyMap(ApplicationHealthPolicyMap && other) = default;
        ApplicationHealthPolicyMap & operator = (ApplicationHealthPolicyMap && other) = default;

        std::shared_ptr<ApplicationHealthPolicy> GetApplicationHealthPolicy(std::wstring const & applicationName) const;
        
        void SetMap(std::map<std::wstring, ApplicationHealthPolicy> && map) { map_ = std::move(map); }

#if defined(PLATFORM_UNIX)
        void Insert(std::wstring & appName, ApplicationHealthPolicy && policy);
#else
        void Insert(std::wstring const & appName, ApplicationHealthPolicy && policy);
#endif

        bool TryValidate(__inout std::wstring & validationErrorMessage) const;

        std::wstring ToString() const;
        static Common::ErrorCode FromString(std::wstring const & applicationHealthPolicyStr, __out ApplicationHealthPolicyMap & ApplicationHealthPolicyMap);

        Common::ErrorCode FromPublicApi(FABRIC_APPLICATION_HEALTH_POLICY_MAP const & publicAppHealthPolicyMap);
        void ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_APPLICATION_HEALTH_POLICY_MAP & publicAppHealthPolicyMap) const;

        static Common::ErrorCode FromPublicApiMap(
            FABRIC_APPLICATION_HEALTH_POLICY_MAP const & publicAppHealthPolicyMap,
            __inout std::map<std::wstring, ApplicationHealthPolicy> & map);
        
        void clear() { return map_.clear(); }
        bool empty() const { return map_.empty(); }
        size_t size() const { return map_.size(); }

        bool operator == (ApplicationHealthPolicyMap const & other) const;
        bool operator != (ApplicationHealthPolicyMap const & other) const;

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::ApplicationHealthPolicyMap, map_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        FABRIC_FIELDS_01(map_);

    private:
        std::map<std::wstring, ApplicationHealthPolicy> map_;
    };

    using ApplicationHealthPolicyMapUPtr = std::unique_ptr<ApplicationHealthPolicyMap>;
    using ApplicationHealthPolicyMapSPtr = std::shared_ptr<ApplicationHealthPolicyMap>;
}

