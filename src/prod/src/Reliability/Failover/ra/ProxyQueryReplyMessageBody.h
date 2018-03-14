// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        template <class T>
        class ProxyQueryReplyMessageBody : public Serialization::FabricSerializable
        {
        public:
            ProxyQueryReplyMessageBody(){}

            ProxyQueryReplyMessageBody(
                Common::ErrorCode && errorCode,
                Reliability::FailoverUnitDescription const & fuDesc,
                ReplicaDescription const & localReplica,
                Common::StopwatchTime replyTimestamp,
                T && queryResult) 
                : errorCode_(std::move(errorCode)),
                  fuDesc_(fuDesc),
                  localReplica_(localReplica),
                  queryResult_(std::move(queryResult)),
                  replyTimeStamp_(replyTimestamp)
            {
            }

            ProxyQueryReplyMessageBody(ProxyQueryReplyMessageBody && other)
                : errorCode_(std::move(other.errorCode_)),
                  replyTimeStamp_(std::move(other.replyTimeStamp_)),
                  fuDesc_(std::move(other.fuDesc_)),
                  localReplica_(std::move(other.localReplica_)),
                  queryResult_(std::move(other.queryResult_))
            {
            }

            ProxyQueryReplyMessageBody & operator=(ProxyQueryReplyMessageBody && other)
            {
                if (this != &other)
                {
                    errorCode_ = std::move(other.errorCode_);
                    replyTimeStamp_ = std::move(other.replyTimeStamp_);
                    fuDesc_ = std::move(other.fuDesc_);
                    localReplica_ = std::move(other.localReplica_);
                    queryResult_ = std::move(other.queryResult_);
                }

                return *this;
            }

            __declspec (property(get=get_FailoverUnitDescription)) Reliability::FailoverUnitDescription const & FailoverUnitDescription;
            Reliability::FailoverUnitDescription const & get_FailoverUnitDescription() const { return fuDesc_; }

            __declspec (property(get=get_LocalReplicaDescription)) Reliability::ReplicaDescription const & LocalReplicaDescription;
            Reliability::ReplicaDescription const & get_LocalReplicaDescription() const { return localReplica_; }

            __declspec (property(get=get_QueryResult)) T const & QueryResult;
            T const & get_QueryResult() const { return queryResult_; }

            __declspec (property(get=get_ReplyTimeStamp)) Common::StopwatchTime const & ReplyTimeStamp;
            Common::StopwatchTime const & get_ReplyTimeStamp() const { return replyTimeStamp_; }

            __declspec (property(get=get_ErrorCode)) Common::ErrorCode const & ErrorCode;
            Common::ErrorCode const & get_ErrorCode() const { return errorCode_; }

            FABRIC_FIELDS_05(errorCode_, replyTimeStamp_, fuDesc_, localReplica_, queryResult_);

            ServiceModel::QueryResult CreateQueryResult()
            {
                return ServiceModel::QueryResult(Common::make_unique<T>(queryResult_));
            }

        private:
            Common::ErrorCode errorCode_;
            Common::StopwatchTime replyTimeStamp_;
            Reliability::FailoverUnitDescription fuDesc_;
            Reliability::ReplicaDescription localReplica_;
            T queryResult_;
        };
    }
}
