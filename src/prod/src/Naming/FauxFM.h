// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

#include "Reliability/Failover/common/RSMessage.h"
#include "TestNodeWrapper.h"

namespace Naming
{
    namespace TestHelper
    {
        class FauxFM : public Naming::INamingStoreServiceRouter, public Common::ComponentRoot
        {
            DENY_COPY(FauxFM);

        public:
            explicit FauxFM(Federation::FederationSubsystemSPtr const & federation) :
              federation_(federation),
              federationWrapper_(*federation),
              services_(),
              serviceDescriptions_(),
              partitionLocations_(),
              fmVersion_(0)
            {
                RegisterHandler();
            }

            __declspec(property(get=get_Id)) Federation::NodeId Id;
            Federation::NodeId get_Id() const
            {
                return Federation::NodeId::MinNodeId;
            }

            virtual Common::AsyncOperationSPtr BeginProcessRequest(
                Transport::MessageUPtr && message,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);
                    
            virtual Common::ErrorCode EndProcessRequest(
                Common::AsyncOperationSPtr const & operation,
                __out Transport::MessageUPtr & reply);

            virtual Common::AsyncOperationSPtr BeginRequestReplyToPeer(
                Transport::MessageUPtr &&,
                Federation::NodeInstance const &,
                Transport::FabricActivityHeader const &,
                Common::TimeSpan,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &) { return Common::AsyncOperationSPtr(); }
            virtual Common::ErrorCode EndRequestReplyToPeer(
                Common::AsyncOperationSPtr const &,
                __out Transport::MessageUPtr &) { return Common::ErrorCodeValue::NotImplemented; }

            virtual bool ShouldAbortProcessing() { return false; }

            void DirectlyAddService(Reliability::ServiceDescription const &, std::vector<Reliability::ConsistencyUnitId> const &, bool place = true);

            void DirectlyRemoveService(std::wstring const &);

            void PlaceCuids(std::vector<Reliability::ConsistencyUnitId> const & cuids);
            void ChangeCuidLocation(
                Reliability::ConsistencyUnitId const & cuid,
                std::wstring const & location);

            void MapNamingPartition(Reliability::ConsistencyUnitId const & cuid, Federation::NodeInstance const & namingNode);

            bool DoesServiceExist(std::wstring const & serviceName);

            void IncrementFMVersion();

            int64 GetFMVersion();

            int64 GetCuidFMVersion(Reliability::ConsistencyUnitId const & cuid) const;
            void IncrementCuidFMVersion(Reliability::ConsistencyUnitId const & cuid);

            static SystemServices::SystemServiceLocation CreateStoreServiceLocation(Federation::NodeInstance const &, Common::Guid const & partitionId);

        private:
            class ServiceOperationAsyncOperation;

            void RegisterHandler();

            void FMMessageHandler(
                __in Transport::Message & message,
                Federation::RequestReceiverContextUPtr && requestReceiverContext);

            void OnFMProcessRequestComplete(
                Federation::RequestReceiverContext &,
                Common::AsyncOperationSPtr const &);
                        
            Federation::FederationSubsystemSPtr federation_;
            Reliability::FederationWrapper federationWrapper_;
            std::map<std::wstring, std::vector<Reliability::ConsistencyUnitId>> services_;
            std::map<std::wstring, Reliability::ServiceDescription> serviceDescriptions_;
            int64 fmVersion_;

            std::map<Reliability::ConsistencyUnitId, std::pair<std::wstring, int64>> partitionLocations_;
        };

        typedef std::shared_ptr<FauxFM> FauxFMSPtr;
    }
}
