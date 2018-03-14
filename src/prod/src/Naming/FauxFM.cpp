// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include "FauxFM.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace Reliability;
using namespace Federation;
using namespace SystemServices;

namespace Naming
{
    namespace TestHelper
    {
        struct SampleMessageBody : public Serialization::FabricSerializable
        {
            SampleMessageBody()
            {
            }

            SampleMessageBody(std::wstring const & message)
                : message_(message)
            {
            }

            FABRIC_FIELDS_01(message_);

            std::wstring const & get_Message() const { return this->message_; }

        private:
            std::wstring message_;

        };

        class FauxFM::ServiceOperationAsyncOperation : public AsyncOperation
        {
        public:
            ServiceOperationAsyncOperation(
                FauxFM & fm,
                wstring const & operation,
                wstring const & serviceName,
                vector<ConsistencyUnitId> const & newCuids,
                AsyncCallback const & callback,
                AsyncOperationSPtr const & operationRoot)
                : AsyncOperation(callback, operationRoot)
                , operation_(operation)
                , serviceName_(serviceName)
                , services_(fm.services_)
                , serviceDescriptions_(fm.serviceDescriptions_)
                , newCuids_(newCuids)
                , locations_()
                , fm_(fm)
            {
            }

            static ErrorCode End(
                AsyncOperationSPtr const & asyncOperation,
                __out wstring & operation,
                __out vector<ServiceTableEntry> & result)
            {
                auto casted = AsyncOperation::End<ServiceOperationAsyncOperation>(asyncOperation);
                swap(casted->locations_, result);
                operation = casted->operation_;
                return casted->Error;
            }

        private:
            void OnStart(AsyncOperationSPtr const & thisSPtr)
            {
                if (operation_ == RSMessage::GetServiceTableRequest().Action)
                {
                    for (auto pair : services_)
                    {
                        for (auto cuid : pair.second)
                        {
                            if (fm_.partitionLocations_.find(cuid) != fm_.partitionLocations_.end())
                            {
                                wstring partitionLocation = fm_.partitionLocations_[cuid].first;
                                ServiceReplicaSet repSet;

                                if (serviceDescriptions_[pair.first].IsStateful)
                                {
                                    vector<wstring> secondaryLocations;
                                    for (size_t i = 1; i < static_cast<size_t>(serviceDescriptions_[pair.first].TargetReplicaSetSize); ++i)
                                    {
                                        secondaryLocations.push_back(wformatString("secondary{0}", i));
                                    }

                                    repSet = ServiceReplicaSet(
                                        true,
                                        true,
                                        move(partitionLocation),
                                        move(secondaryLocations),
                                        fm_.partitionLocations_[cuid].second);
                                }
                                else
                                {
                                    wstring primary = L"";
                                    vector<wstring> instanceLocations;
                                    instanceLocations.push_back(partitionLocation);
                                    for (size_t i = 1; i < static_cast<size_t>(serviceDescriptions_[pair.first].TargetReplicaSetSize); ++i)
                                    {
                                        instanceLocations.push_back(wformatString("instance{0}", i));
                                    }

                                    fm_.IncrementFMVersion();
                                    repSet = ServiceReplicaSet(
                                        false,
                                        false,
                                        move(primary),
                                        move(instanceLocations),
                                        fm_.GetFMVersion());
                                }

                                locations_.push_back(ServiceTableEntry(cuid, pair.first, move(repSet)));
                            }
                            else
                            {
                                wstring primary = L"";
                                vector<wstring> replicas = vector<wstring>();
                                fm_.IncrementFMVersion();
                                ServiceReplicaSet repSet(
                                    true,
                                    false,
                                    move(primary),
                                    move(replicas),
                                    fm_.GetFMVersion());
                                locations_.push_back(ServiceTableEntry(cuid, pair.first, move(repSet)));
                            }
                        }
                    }
                    
                    Trace.WriteInfo(Constants::TestSource, "FauxFM returning service table: {0}", locations_);
                }
                else if (operation_ == RSMessage::GetCreateService().Action)
                {
                    Trace.WriteInfo(Constants::TestSource, "FauxFM creating service {0}.", serviceName_);
                    services_[serviceName_] = newCuids_;
                }
                else if (operation_ == RSMessage::GetDeleteService().Action)
                {
                    Trace.WriteInfo(Constants::TestSource, "FauxFM deleting service {0}.", serviceName_);
                    services_.erase(serviceName_);
                    serviceDescriptions_.erase(serviceName_);
                }
                else if (operation_ == RSMessage::GetNodeUp().Action
                        || operation_ == RSMessage::GetChangeNotification().Action)
                {
                    // Intentional no-op
                }
                else
                {
                    Assert::CodingError("Unexpected operation {0}", operation_);
                }
                

                TryComplete(thisSPtr, ErrorCode());
            }
            
            FauxFM & fm_;
            wstring serviceName_;
            map<wstring, vector<ConsistencyUnitId>> & services_;
            map<wstring, ServiceDescription> & serviceDescriptions_;
            vector<ConsistencyUnitId> newCuids_;
            vector<ServiceTableEntry> locations_;
            wstring operation_;
        };

        AsyncOperationSPtr FauxFM::BeginProcessRequest(
            MessageUPtr && message,
            TimeSpan const,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & operationRoot)
        {
            if (message->Action == RSMessage::GetServiceTableRequest().Action)
            {
                return AsyncOperation::CreateAndStart<ServiceOperationAsyncOperation>(
                    *this,
                    message->Action,
                    L"",
                    vector<ConsistencyUnitId>(),
                    callback,
                    operationRoot);
            }
            else if (message->Action == RSMessage::GetCreateService().Action)
            {
                Reliability::CreateServiceMessageBody body;
                if (!message->GetBody(body))
                {
                    Assert::CodingError("Failed to get CreateService message body");
                }

                vector<ConsistencyUnitId> consistencyUnitIds;
                for (auto const& consistencyUnitDescription : body.ConsistencyUnitDescriptions)
                {
                    consistencyUnitIds.push_back(consistencyUnitDescription.ConsistencyUnitId);
                }

                serviceDescriptions_[body.ServiceDescription.Name] = body.ServiceDescription;

                return AsyncOperation::CreateAndStart<ServiceOperationAsyncOperation>(
                    *this,
                    message->Action,
                    body.ServiceDescription.Name,
                    consistencyUnitIds,
                    callback,
                    operationRoot);
            }
            else if (message->Action == RSMessage::GetDeleteService().Action)
            {
                SampleMessageBody body;
                if (!message->GetBody(body))
                {
                    Assert::CodingError("Failed to get DeleteService message body");
                }

                return AsyncOperation::CreateAndStart<ServiceOperationAsyncOperation>(
                    *this,
                    message->Action,
                    body.get_Message(),
                    vector<ConsistencyUnitId>(),
                    callback,
                    operationRoot);
            }
            else if (message->Action == RSMessage::GetNodeUp().Action
                    || message->Action == RSMessage::GetChangeNotification().Action)
            {
                return AsyncOperation::CreateAndStart<ServiceOperationAsyncOperation>(
                    *this,
                    message->Action,
                    L"",
                    vector<ConsistencyUnitId>(),
                    callback,
                    operationRoot);
            }
            else
            {
                Assert::CodingError("Unexpected message action {0}", message->Action);
            }
        }

        ErrorCode FauxFM::EndProcessRequest(
            AsyncOperationSPtr const & asyncOperation,
            __out MessageUPtr & reply)
        {
            vector<ServiceTableEntry> result;
            wstring operation;
            ErrorCode error = ServiceOperationAsyncOperation::End(asyncOperation, operation, result);
            if (operation == RSMessage::GetServiceTableRequest().Action)
            {
                ServiceTableUpdateMessageBody body(move(result), GenerationNumber(), VersionRangeCollection(), 0, false);
                reply = RSMessage::GetServiceTableUpdate().CreateMessage<ServiceTableUpdateMessageBody>(body);
            }
            else if (operation == RSMessage::GetCreateService().Action)
            {
                reply = RSMessage::GetCreateServiceReply().CreateMessage(CreateServiceReplyMessageBody(error.ReadValue(), ServiceDescription()));
            }
            else if (operation == RSMessage::GetDeleteService().Action)
            {
                reply = RSMessage::GetDeleteServiceReply().CreateMessage(BasicFailoverReplyMessageBody(error.ReadValue()));
            }
            else if (operation == RSMessage::GetNodeUp().Action)
            {
                reply = RSMessage::GetNodeUpAck().CreateMessage();
            }
            else if (operation == RSMessage::GetChangeNotification().Action)
            {
                reply = RSMessage::GetChangeNotificationAck().CreateMessage();
            }

            return error;
        }

        void FauxFM::DirectlyAddService(ServiceDescription const & service, std::vector<Reliability::ConsistencyUnitId> const & cuids, bool place)
        {
            services_[service.Name] = cuids;
            serviceDescriptions_[service.Name] = service;
            if (place)
            {
                if (service.Name == ServiceModel::SystemServiceApplicationNameHelper::InternalNamingServiceName)
                {
                    for (size_t i = 0; i < cuids.size(); ++i)
                    {
                        ChangeCuidLocation(cuids[i], CreateStoreServiceLocation(
                                NodeInstance(NodeId(LargeInteger(i+1,i+1)), i), 
                                cuids[i].Guid).Location);
                    }
                }
                else
                {
                    for (ConsistencyUnitId cuid : cuids)
                    {
                        ChangeCuidLocation(cuid, L"Place-" + cuid.Guid.ToString());
                    }
                }
            }
        }

        void FauxFM::ChangeCuidLocation(
            Reliability::ConsistencyUnitId const & cuid,
            std::wstring const & location)
        {
            ++fmVersion_;
            partitionLocations_[cuid] = pair<wstring, int64>(location, fmVersion_);
            Trace.WriteInfo(Constants::TestSource, "FauxFM: Change CUID={0} location={1}, fmVersion={2}", cuid, location, fmVersion_);
        }

        int64 FauxFM::GetCuidFMVersion(Reliability::ConsistencyUnitId const & cuid) const
        {
            auto it = partitionLocations_.find(cuid);
            if (it == partitionLocations_.end())
            {
                return -1;
            }

            return it->second.second;
        }

        void FauxFM::IncrementCuidFMVersion(Reliability::ConsistencyUnitId const & cuid)
        {
            auto it = partitionLocations_.find(cuid);
            ASSERT_IF(it == partitionLocations_.end(), "CUID {0} is not in the FauxFM map", cuid);
            ++fmVersion_;
            it->second.second = fmVersion_;
        }

        void FauxFM::DirectlyRemoveService(std::wstring const & serviceName)
        {
            services_.erase(serviceName);
            serviceDescriptions_.erase(serviceName);
        }

        void FauxFM::PlaceCuids(vector<ConsistencyUnitId> const & cuids)
        {
            for (ConsistencyUnitId cuid : cuids)
            {
                ChangeCuidLocation(cuid, L"Place-" + cuid.Guid.ToString());
            }
        }

        void FauxFM::RegisterHandler()
        {
            federation_->RegisterMessageHandler(
                RSMessage::FMActor,
                [&](Transport::MessageUPtr &, Federation::OneWayReceiverContextUPtr &)
                { 
                    Assert::CodingError("FauxFM does not support oneway messages");
                },
                [&](Transport::MessageUPtr & message, Federation::RequestReceiverContextUPtr & requestReceiverContext)
                { 
                    FMMessageHandler(*message, move(requestReceiverContext));
                },
                true/*dispatchOnTransportThread*/);
        }

        void FauxFM::FMMessageHandler(
            __in Message & message,
            RequestReceiverContextUPtr && requestReceiverContext)
        {
            Trace.WriteNoise(Constants::TestSource, "Sending message {0} to the FauxFM", message);
            MoveUPtr<RequestReceiverContext> contextMover(move(requestReceiverContext));
            BeginProcessRequest(
                message.Clone(),
                TimeSpan::FromSeconds(15),
                [&, contextMover](AsyncOperationSPtr const & asyncOperation) mutable
                { 
                    OnFMProcessRequestComplete(*(contextMover.TakeUPtr()), asyncOperation);
                },
                CreateAsyncOperationRoot());
        }

        void FauxFM::OnFMProcessRequestComplete(
            RequestReceiverContext & requestReceiverContext,
            AsyncOperationSPtr const & operation)
        {
            MessageUPtr reply;
            ErrorCode error = EndProcessRequest(operation, reply);
            if (error.IsSuccess())
            {
                requestReceiverContext.Reply(std::move(reply));
            }
            else
            {
                requestReceiverContext.Reject(error);
            }
        }

        void FauxFM::MapNamingPartition(ConsistencyUnitId const & cuid, NodeInstance const & namingNode)
        {
            ChangeCuidLocation(cuid, CreateStoreServiceLocation(namingNode, cuid.Guid).Location);
        }

        bool FauxFM::DoesServiceExist(std::wstring const & serviceName)
        {
            return services_.find(serviceName) != services_.end();
        }

        void FauxFM::IncrementFMVersion()
        {
            ++fmVersion_;
        }

        int64 FauxFM::GetFMVersion()
        {
            return fmVersion_;
        }

        SystemServiceLocation FauxFM::CreateStoreServiceLocation(NodeInstance const & nodeInstance, Guid const & guid)
        {
            return SystemServiceLocation(nodeInstance, guid, 0, 0);
        }
    }
}
