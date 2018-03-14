// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        class ReplicaInformation 
        {
            DENY_COPY_ASSIGNMENT(ReplicaInformation);
    
        public:
            ReplicaInformation(
                FABRIC_REPLICA_ID const & id,
                ::FABRIC_REPLICA_ROLE const & role,
                std::wstring const & replicatorAddress,
                bool mustCatchup,
                FABRIC_SEQUENCE_NUMBER currentProgress = Constants::NonInitializedLSN,
                FABRIC_SEQUENCE_NUMBER catchupCapability = Constants::NonInitializedLSN);

            ReplicaInformation(FABRIC_REPLICA_INFORMATION const * value);
            ReplicaInformation(ReplicaInformation const & other);
            ReplicaInformation(ReplicaInformation && other);
            virtual ~ReplicaInformation();
    
            __declspec(property(get=get_Value)) FABRIC_REPLICA_INFORMATION const * Value;
            inline FABRIC_REPLICA_INFORMATION const * get_Value() const { return &info_; }

            __declspec(property(get=get_Id)) FABRIC_REPLICA_ID const & Id;
            inline FABRIC_REPLICA_ID const & get_Id() const { return info_.Id; }

            __declspec(property(get=get_Role)) ::FABRIC_REPLICA_ROLE const & Role;
            inline ::FABRIC_REPLICA_ROLE const & get_Role() const { return info_.Role; }

            __declspec(property(get=get_ReplicatorAddress)) std::wstring const & ReplicatorAddress;
            inline std::wstring const & get_ReplicatorAddress() const { return replicatorAddress_; }

            __declspec(property(get=get_TransportId)) std::wstring const & TransportEndpointId;
            inline std::wstring const & get_TransportId() const { return transportId_; }

            __declspec(property(get=get_CurrentProgress)) FABRIC_SEQUENCE_NUMBER const & CurrentProgress;
            inline FABRIC_SEQUENCE_NUMBER const & get_CurrentProgress() const { return info_.CurrentProgress; }

            __declspec(property(get=get_CatchUpCapability)) FABRIC_SEQUENCE_NUMBER const & CatchUpCapability;
            inline FABRIC_SEQUENCE_NUMBER const & get_CatchUpCapability() const { return info_.CatchUpCapability; }

            __declspec(property(get=get_MustCatchup)) bool MustCatchup;
            inline bool get_MustCatchup() const { return ex1Info_.MustCatchup == TRUE; }

            void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const;

        private:
            FABRIC_REPLICA_INFORMATION info_;
            FABRIC_REPLICA_INFORMATION_EX1 ex1Info_;
            std::wstring replicatorAddress_;
            std::wstring transportId_;
        };
    } // end namespace ReplicationComponent
} // end namespace Reliability
