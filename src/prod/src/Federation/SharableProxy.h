// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Federation/VoteProxy.h"
#include "Store/StoreBase.h"

namespace Federation
{
    class SharableProxy : public VoteProxy
    {
        DENY_COPY(SharableProxy);
    public:
        class ArbitrationRecord : public Serialization::FabricSerializable
        {
        public:
            ArbitrationRecord()
            {
            }
    
            ArbitrationRecord(LeaseWrapper::LeaseAgentInstance const & monitor, Common::DateTime expireTime)
                :   monitor_(monitor),
                    expireTime_(expireTime)
            {
            }
    
            ArbitrationRecord(ArbitrationRecord && other)
                :   monitor_(std::move(other.monitor_)),
                    expireTime_(other.expireTime_)
            {
            }
        
            ArbitrationRecord& operator=(ArbitrationRecord && other)
            {
                if (this != &other)
                {
                    monitor_ = std::move(other.monitor_);
                    expireTime_ = other.expireTime_;
                }
            
                return *this;
            }
    
            __declspec (property(get=getMonitor)) LeaseWrapper::LeaseAgentInstance const & Monitor;
            LeaseWrapper::LeaseAgentInstance const & getMonitor() const
            {
                return monitor_;
            }
    
            __declspec (property(get=getExpireTime, put=setExpireTime)) Common::DateTime ExpireTime;
            Common::DateTime const & getExpireTime() const
            {
                return expireTime_;
            }
            void setExpireTime(Common::DateTime expireTime)
            {
                expireTime_ = expireTime;
            }
    
            FABRIC_FIELDS_02(monitor_, expireTime_);
    
        private:
            LeaseWrapper::LeaseAgentInstance monitor_;
            Common::DateTime expireTime_;
        };

        SharableProxy(std::shared_ptr<Store::ILocalStore> && store, std::wstring const & ringName, NodeId voteId, NodeId proxyId);
        ~SharableProxy();

    protected:
        Common::ErrorCode OnOpen();
        Common::ErrorCode OnClose();
        void OnAbort();

        virtual std::wstring const& GetRingName() const { return ringName_; }

        virtual Common::ErrorCode OnAcquire(__inout NodeConfig & ownerConfig,
            Common::TimeSpan ttl,
            bool preempt,
            __out VoteOwnerState & state);
        virtual Common::ErrorCode OnSetGlobalTicket(Common::DateTime globalTicket);
        virtual Common::ErrorCode OnSetSuperTicket(Common::DateTime superTicket);

        virtual Common::AsyncOperationSPtr BeginArbitrate(
            SiteNode & siteNode,
            ArbitrationRequestBody const & request,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        virtual Common::ErrorCode EndArbitrate(
            Common::AsyncOperationSPtr const & operation,
            SiteNode & siteNode,
            __out ArbitrationReplyBody & result);

    private:    
        class VoteOwnerData : public Serialization::FabricSerializable
        {
        public:
            VoteOwnerData()
                : nodeConfig_(),
                expiryTime_(Common::DateTime::Zero),
                globalTicketExpiryTime_(Common::DateTime::Zero),
                superTicketExpiryTime_(Common::DateTime::Zero)
            {
            }
    
            VoteOwnerData(
                NodeConfig const & nodeConfig, 
                Common::DateTime const & expiryTime,
                Common::DateTime const & globalTicketExpiryTime,
                Common::DateTime const & superTicketExpiryTime
                )
                : nodeConfig_(nodeConfig),
                expiryTime_(expiryTime),
                globalTicketExpiryTime_(globalTicketExpiryTime),
                superTicketExpiryTime_(superTicketExpiryTime)
            {
            }
    
            __declspec(property(get=getId)) NodeId const Id;
            NodeId const getId() const { return nodeConfig_.Id; }
    
            __declspec(property(get=getExpiryTime)) Common::DateTime ExpiryTime;
            Common::DateTime const getExpiryTime() const { return expiryTime_; }
    
            __declspec(property(get=getConfig)) NodeConfig Config;
            NodeConfig const getConfig() const { return nodeConfig_; }
    
            __declspec(property(get=getGlobalTicketExpiryTime)) Common::DateTime GlobalTicketExpiryTime;
            Common::DateTime const getGlobalTicketExpiryTime() const { return globalTicketExpiryTime_; }
    
            __declspec(property(get=getSuperTicketExpiryTime)) Common::DateTime SuperTicketExpiryTime;
            Common::DateTime const getSuperTicketExpiryTime() const { return superTicketExpiryTime_; }
    
            FABRIC_FIELDS_04(nodeConfig_, expiryTime_, globalTicketExpiryTime_, superTicketExpiryTime_);
    
        private:
            NodeConfig nodeConfig_;
            Common::DateTime expiryTime_;
            Common::DateTime globalTicketExpiryTime_;
            Common::DateTime superTicketExpiryTime_;
        };

        class RecordVector : public Serialization::FabricSerializable
        {
        public:
            RecordVector(int64 instance)
                :   instance_(instance),
                    updated_(true),
                    isNew_(true)
            {
            }
    
            __declspec (property(get=getInstance)) int64 Instance;
            int64 const & getInstance() const
            {
                return instance_;
            }
    
            bool IsEmpty() const
            {
                return (records_.size() == 0);
            }
    
            bool IsUpdated() const
            {
                return updated_;
            }
    
            bool IsNew() const
            {
                return isNew_;
            }
    
            void PostRead()
            {
                updated_ = false;
                isNew_ = false;
            }
    
            bool IsReverted() const;
    
            void RevertMonitor();
    
            bool Contains(LeaseWrapper::LeaseAgentInstance const & monitor);
    
            void RevertSubject(LeaseWrapper::LeaseAgentInstance const & monitor);
    
            void Reset(int64 instance);
    
            void Add(LeaseWrapper::LeaseAgentInstance const & monitor, Common::DateTime expireTime);
    
            void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const;
    
            FABRIC_FIELDS_02(instance_, records_);
    
        private:
            int64 instance_;
            std::vector<ArbitrationRecord> records_;
            bool updated_;
            bool isNew_;
        };

        class AribtrateAsyncOperation : public Common::TimedAsyncOperation, Common::TextTraceComponent<Common::TraceTaskCodes::Arbitration>
        {
        public:
            AribtrateAsyncOperation(
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent,
                Common::TimeSpan ttl,
                LeaseWrapper::LeaseAgentInstance const & monitor,
                LeaseWrapper::LeaseAgentInstance const & subject,
                NodeId proxyId,
                NodeId voteId,
                std::shared_ptr<Store::ILocalStore> const & store)
                : Common::TimedAsyncOperation(timeout, callback, parent),
                ttl_(ttl), monitor_(monitor), subject_(subject), proxyId_(proxyId), voteId_(voteId), store_(store)
            {
            }

            __declspec(property(get = get_Result)) ArbitrationReplyBody const & Result;
            ArbitrationReplyBody const & get_Result() { return result_; }

            void WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const;

        protected:
            virtual void OnStart(Common::AsyncOperationSPtr const & thisSPtr);

        private:
            Common::ErrorCode GetRecords(Store::ILocalStore::TransactionSPtr const & trans, LeaseWrapper::LeaseAgentInstance const & instance, SharableProxy::RecordVector & records);
            Common::ErrorCode StoreRecords(Store::ILocalStore::TransactionSPtr const & trans, LeaseWrapper::LeaseAgentInstance const & instance, SharableProxy::RecordVector const & records);

            ArbitrationReplyBody ProcessRequest(SharableProxy::RecordVector & monitorRecords, SharableProxy::RecordVector & subjectRecords, Common::DateTime expireTime);
            Common::ErrorCode ProcessRequest(Store::ILocalStore::TransactionSPtr const & trans);
            Common::ErrorCode ProcessRequest();

            Common::ErrorCode PreventDeadlock(Store::ILocalStore::TransactionSPtr const & transPtr);

            Common::ErrorCode GetProxyTime(Common::DateTime & timeStamp);
            
            bool IsRevert() const {return ttl_ == Common::TimeSpan::MinValue;}

            void StartProcessing(Common::AsyncOperationSPtr const & operation);

            LeaseWrapper::LeaseAgentInstance monitor_;
            LeaseWrapper::LeaseAgentInstance subject_;
            Common::TimeSpan ttl_;
            NodeId proxyId_;
            NodeId voteId_;
            ArbitrationReplyBody result_;
            std::shared_ptr<Store::ILocalStore> store_;
			RWLOCK(Federation.AribtrateAsyncOperation, transactionLock_);
        };

        Common::ErrorCode GetProxyTime(Common::DateTime & ticket);
        Common::ErrorCode GetVoteOwnerData(Store::ILocalStore::TransactionSPtr const & trans, VoteOwnerData &owner, _int64 & sequenceNumber);
        Common::ErrorCode SetTicket(bool isGlobalTicket, Common::DateTime expiryTime);
        Common::ErrorCode SetVoteOwnerData(VoteOwnerData const & data, Store::ILocalStore::TransactionSPtr const & trans, _int64 sequenceNumber);

        std::shared_ptr<Store::ILocalStore> store_;
        std::wstring ringName_;
    };
}

DEFINE_USER_ARRAY_UTILITY(Federation::SharableProxy::ArbitrationRecord);
