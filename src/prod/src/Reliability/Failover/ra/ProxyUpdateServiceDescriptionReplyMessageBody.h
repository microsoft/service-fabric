// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        class ProxyUpdateServiceDescriptionReplyMessageBody : public Serialization::FabricSerializable
        {
        public:
            ProxyUpdateServiceDescriptionReplyMessageBody(){}

            ProxyUpdateServiceDescriptionReplyMessageBody(
                Common::ErrorCode && errorCode,
                Reliability::FailoverUnitDescription const & fuDesc,
                ReplicaDescription const & localReplica,
                Reliability::ServiceDescription const & serviceDescription) 
                : errorCode_(std::move(errorCode)),
                  fuDesc_(fuDesc),
                  localReplica_(localReplica),
                  serviceDescription_(serviceDescription)
            {
            }

            ProxyUpdateServiceDescriptionReplyMessageBody(ProxyUpdateServiceDescriptionReplyMessageBody && other)
                : errorCode_(std::move(other.errorCode_)),
                  fuDesc_(std::move(other.fuDesc_)),
                  localReplica_(std::move(other.localReplica_)),
                  serviceDescription_(std::move(other.serviceDescription_))
            {
            }

            ProxyUpdateServiceDescriptionReplyMessageBody & operator=(ProxyUpdateServiceDescriptionReplyMessageBody && other)
            {
                if (this != &other)
                {
                    errorCode_ = std::move(other.errorCode_);
                    fuDesc_ = std::move(other.fuDesc_);
                    localReplica_ = std::move(other.localReplica_);
                    serviceDescription_ = std::move(other.serviceDescription_);
                }

                return *this;
            }

            __declspec (property(get=get_FailoverUnitDescription)) Reliability::FailoverUnitDescription const & FailoverUnitDescription;
            Reliability::FailoverUnitDescription const & get_FailoverUnitDescription() const { return fuDesc_; }

            __declspec (property(get=get_LocalReplicaDescription)) Reliability::ReplicaDescription const & LocalReplicaDescription;
            Reliability::ReplicaDescription const & get_LocalReplicaDescription() const { return localReplica_; }

            __declspec (property(get = get_ServiceDescription)) Reliability::ServiceDescription const & ServiceDescription;
            Reliability::ServiceDescription const & get_ServiceDescription() const { return serviceDescription_; }

            __declspec (property(get=get_ErrorCode)) Common::ErrorCode const & ErrorCode;
            Common::ErrorCode const & get_ErrorCode() const { return errorCode_; }

            void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const;

            void WriteToEtw(uint16 contextSequenceId) const;

            FABRIC_FIELDS_04(errorCode_, fuDesc_, localReplica_, serviceDescription_);

        private:
            Common::ErrorCode errorCode_;
            Reliability::ServiceDescription serviceDescription_;
            Reliability::FailoverUnitDescription fuDesc_;
            Reliability::ReplicaDescription localReplica_;
        };
    }
}
