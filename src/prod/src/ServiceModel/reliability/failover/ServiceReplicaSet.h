// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class ServiceReplicaSet : 
        public Serialization::FabricSerializable
        , public Common::ISizeEstimator
    {
    public:
        ServiceReplicaSet();

        ServiceReplicaSet(
            bool isStateful,
            bool isPrimaryLocationValid,
            std::wstring && primaryLocation,
            std::vector<std::wstring> && replicaLocations,
            int64 lookupVersion);

        ServiceReplicaSet(ServiceReplicaSet const & other);
        ServiceReplicaSet & operator=(ServiceReplicaSet const & other);
        
        ServiceReplicaSet(ServiceReplicaSet && other);
        ServiceReplicaSet & operator=(ServiceReplicaSet && other);

        __declspec(property(get=get_IsStateful)) bool IsStateful;
        bool get_IsStateful() const { return isStateful_; }

        __declspec(property(get=get_IsPrimaryLocationValid)) bool IsPrimaryLocationValid;
        bool get_IsPrimaryLocationValid() const { return isPrimaryLocationValid_; }

        __declspec(property(get=get_PrimaryLocation)) std::wstring const& PrimaryLocation;
        std::wstring const& get_PrimaryLocation() const
        {
            ASSERT_IFNOT(isStateful_, "This is not a stateful service.");
            ASSERT_IFNOT(isPrimaryLocationValid_, "The Primary location is not valid.");

            return primaryLocation_;
        }

        __declspec(property(get=get_SecondaryLocations)) std::vector<std::wstring> const& SecondaryLocations;
        std::vector<std::wstring> const& get_SecondaryLocations() const
        {
            ASSERT_IFNOT(isStateful_, "This is not a stateful service.");

            return replicaLocations_;
        }

        __declspec(property(get=get_ReplicaLocations)) std::vector<std::wstring> const& ReplicaLocations;
        std::vector<std::wstring> const& get_ReplicaLocations() const
        {
            ASSERT_IF(isStateful_, "This is not a stateless service.");
            
            return replicaLocations_;
        }

        __declspec(property(get=get_Version)) int64 LookupVersion;
        int64 get_Version() const { return lookupVersion_; }

        bool IsEmpty() const;

        bool operator == (ServiceReplicaSet const &) const;
        bool operator != (ServiceReplicaSet const &) const;

        void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;
        static std::string AddField(Common::TraceEvent &traceEvent, std::string const& name);
        void FillEventData(__in Common::TraceEventContext &context) const;        

        FABRIC_FIELDS_05(isStateful_, isPrimaryLocationValid_, primaryLocation_, replicaLocations_, lookupVersion_);

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(primaryLocation_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(replicaLocations_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:
        bool isStateful_;
        bool isPrimaryLocationValid_;
        std::wstring primaryLocation_;
        std::vector<std::wstring> replicaLocations_;
        int64 lookupVersion_;
    };
}
