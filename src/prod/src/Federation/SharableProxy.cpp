// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Federation/SharableProxy.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace Store;
using namespace Transport;
using namespace LeaseWrapper;

wchar_t* ArbitrationRecordType = L"Arbitration";
wchar_t* LockKey = L"Lock";
wchar_t* VoteOwnerType = L"VoteOwnerData";

StringLiteral const TraceFailure("Failure");

SharableProxy::SharableProxy(std::shared_ptr<Store::ILocalStore> && store, wstring const & ringName, NodeId voteId, NodeId proxyId)
    :   VoteProxy(voteId, proxyId, true),
        store_(move(store)),
        ringName_(ringName)
{
}

SharableProxy::~SharableProxy()
{
    Abort();
}

ErrorCode SharableProxy::OnOpen()
{
    return this->store_->Open();
}

ErrorCode SharableProxy::OnClose()
{
    return this->store_->Close();
}

void SharableProxy::OnAbort()
{
    this->store_->Abort();
}

ErrorCode SharableProxy::OnAcquire(__inout NodeConfig & ownerConfig,
    Common::TimeSpan ttl,
    bool preempt,
    __out VoteOwnerState & state)
{
    DateTime now;
    ErrorCode error = GetProxyTime(now);
    if (!error.IsSuccess())
    {
        return error;
    }

    ILocalStore::TransactionSPtr trans;
    error = store_->CreateTransaction(trans);
    if (!error.IsSuccess())
    {
        return error;
    }

    VoteOwnerData ownerData;
    wstring key;
    _int64 sequenceNumber;
    error = GetVoteOwnerData(trans, ownerData, sequenceNumber);
    bool dataFound = !error.IsError(ErrorCodeValue::EnumerationCompleted);

    if (dataFound && !error.IsSuccess())
    {
        return error;
    }

    if (!preempt && dataFound)
    {
        VoteProxy::WriteInfo("Acquire", "Owner [{0},{1}] seen at time {2} at {3} for {4}",
            ownerData.Config, ownerData.ExpiryTime, now, ProxyId, VoteId);
        if (ownerData.Id != ownerConfig.Id && ownerData.ExpiryTime > now)
        {
            ownerConfig = NodeConfig(ownerData.Config);
            return ErrorCodeValue::OwnerExists;
        }
    }

    VoteOwnerData newOwnerData(ownerConfig, now + ttl, ownerData.GlobalTicketExpiryTime, ownerData.SuperTicketExpiryTime);
    error = SetVoteOwnerData(newOwnerData, trans, dataFound ? sequenceNumber : -1);
    if (!error.IsSuccess())
    {
        return error;
    }

    state.GlobalTicket = ownerData.GlobalTicketExpiryTime;
    state.SuperTicket = ownerData.SuperTicketExpiryTime;

    return trans->Commit();
}

ErrorCode SharableProxy::GetVoteOwnerData(ILocalStore::TransactionSPtr const & trans, VoteOwnerData &ownerData, _int64 & operationLSN)
{
    ILocalStore::EnumerationSPtr owners;
    ErrorCode error = store_->CreateEnumerationByTypeAndKey(trans, VoteOwnerType, L"", owners);

    if (error.IsSuccess())
    {
        error = owners->MoveNext();
    }

    if (!error.IsSuccess())
    {
        return error;
    }

    vector<byte> value;
    if (!(error = owners->CurrentValue(value)).IsSuccess())
    {
        return error;
    }

    error = owners->CurrentOperationLSN(operationLSN);
    if (!error.IsSuccess())
    {
        return error;
    }

    error = FabricSerializer::Deserialize(ownerData, static_cast<ULONG>(value.size()), value.data());
    if (error.IsSuccess())
    {
        VoteProxyEventSource::Events->VoteOwnerDataRead(
            ownerData.Id,
            ownerData.ExpiryTime,
            ownerData.GlobalTicketExpiryTime,
            ownerData.SuperTicketExpiryTime);
    }

    return error;
}

ErrorCode SharableProxy::SetTicket(bool isGlobalTicket, DateTime expiryTime)
{
    VoteOwnerData owner;
    _int64 ownerSequenceNumber;
    ILocalStore::TransactionSPtr trans;
    ErrorCode error = store_->CreateTransaction(trans);

    if (!error.IsSuccess())
    {
        return error;
    }

    error = GetVoteOwnerData(trans, owner, ownerSequenceNumber);

    if (!error.IsSuccess())
    {
        return error;
    }

    if (owner.Id != ProxyId)
    {
        VoteProxy::WriteInfo("SetTicket", "Owner {0} expiration {1} is seen when trying to set ticket for {2}@{3}",
            owner.Id, owner.ExpiryTime, VoteId, ProxyId);
        return ErrorCodeValue::NotOwner;
    }

    TimeSpan interval = expiryTime - DateTime::Now();
    DateTime timeAtProxy;
    error = GetProxyTime(timeAtProxy);
    if (!error.IsSuccess())
    {
        return error;
    }

    DateTime expiryTimeAtProxy = timeAtProxy + interval;
    VoteOwnerData data(
        owner.Config,
        max<DateTime>(expiryTimeAtProxy, owner.ExpiryTime),
        isGlobalTicket ? expiryTimeAtProxy : owner.GlobalTicketExpiryTime,
        isGlobalTicket ? owner.SuperTicketExpiryTime : expiryTimeAtProxy);

    error = SetVoteOwnerData(data, trans, ownerSequenceNumber);
    if (error.IsSuccess())
    {
        error = trans->Commit();
    }

    return error;
}

ErrorCode SharableProxy::SetVoteOwnerData(VoteOwnerData const & ownerData, ILocalStore::TransactionSPtr const & trans, _int64 sequenceNumber)
{
    VoteProxyEventSource::Events->VoteOwnerDataWrite(
        ownerData.Id,
        ownerData.ExpiryTime,
        ownerData.GlobalTicketExpiryTime,
        ownerData.SuperTicketExpiryTime,
        sequenceNumber);

    vector<byte> buffer;
    ErrorCode error = FabricSerializer::Serialize(&ownerData, buffer);
    if (!error.IsSuccess())
    {
        return error;
    }

    if (sequenceNumber == -1)
    {
        return store_->Insert(trans, VoteOwnerType, L"", buffer.data(), buffer.size(), ILocalStore::OperationNumberUnspecified);
    }
    else 
    {
        return store_->Update(trans, VoteOwnerType, L"", sequenceNumber, L"", buffer.data(), buffer.size(), ILocalStore::OperationNumberUnspecified);
    }
}

ErrorCode SharableProxy::GetProxyTime(DateTime & timeStamp)
{
    FILETIME time = store_->GetStoreUtcFILETIME();
    ULARGE_INTEGER convert;
    convert.LowPart = time.dwLowDateTime;
    convert.HighPart = time.dwHighDateTime;
    timeStamp = DateTime(convert.QuadPart);
    return timeStamp == DateTime::Zero ? ErrorCodeValue::StoreUnexpectedError : ErrorCodeValue::Success;
}

ErrorCode SharableProxy::OnSetGlobalTicket(Common::DateTime expiryTime)
{
    return SetTicket(true, expiryTime);
}
 
ErrorCode SharableProxy::OnSetSuperTicket(Common::DateTime expiryTime)
{
    return SetTicket(false, expiryTime);
}

AsyncOperationSPtr SharableProxy::BeginArbitrate(
    SiteNode & siteNode,
    ArbitrationRequestBody const & request,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    UNREFERENCED_PARAMETER(siteNode);

    return AsyncOperation::CreateAndStart<SharableProxy::AribtrateAsyncOperation>(
        timeout, 
        callback,
        parent,
        request.SubjectTTL,
        request.Monitor,
        request.Subject,
        ProxyId,
        VoteId,
        store_);
}

ErrorCode SharableProxy::EndArbitrate(
    AsyncOperationSPtr const & operation,
    SiteNode & /*siteNode*/,
    __out ArbitrationReplyBody & result)
{
    SharableProxy::AribtrateAsyncOperation *op = AsyncOperation::Get<SharableProxy::AribtrateAsyncOperation>(operation);
    ErrorCode error = op->End(operation);
    if (error.IsSuccess())
    {
        result = op->Result;
    }
    else
    {
        result = ArbitrationReplyBody(TimeSpan::MaxValue, false);
    }

    return error;
}

bool SharableProxy::RecordVector::IsReverted() const
{
    return (records_.size() > 0 && records_[0].Monitor.InstanceId == 0);
}

void SharableProxy::RecordVector::RevertMonitor()
{
    records_.clear();
    records_.push_back(ArbitrationRecord(LeaseAgentInstance(L"", 0), DateTime::Now()));
    updated_ = true;
}

void SharableProxy::RecordVector::Add(LeaseWrapper::LeaseAgentInstance const & monitor, DateTime expireTime)
{
    bool found = false;

    for (auto record = records_.begin(); record != records_.end(); ++record)
    {
        if (record->Monitor == monitor)
        {
            if (record->ExpireTime > expireTime)
            {
                record->ExpireTime = expireTime;
                updated_ = true;
            }

            found = true;
        }
    }

    if (!found)
    {
        records_.push_back(ArbitrationRecord(monitor, expireTime));
        updated_ = true;
    }
}

bool SharableProxy::RecordVector::Contains(LeaseAgentInstance const & monitor)
{
    auto it = find_if(records_.begin(), records_.end(),
        [&monitor](ArbitrationRecord const & x) { return (x.Monitor == monitor); });

    return (it != records_.end());
}

void SharableProxy::RecordVector::RevertSubject(LeaseAgentInstance const & monitor)
{
    auto end = remove_if(records_.begin(), records_.end(),
        [&monitor](ArbitrationRecord const & x) { return (x.Monitor == monitor); });

    if (end != records_.end())
    {
        records_.erase(end, records_.end());
        updated_ = true;
    }
}

void SharableProxy::RecordVector::Reset(int64 instance)
{
    instance_ = instance;
    records_.clear();
    updated_ = true;
}

void SharableProxy::RecordVector::WriteTo(TextWriter& w, FormatOptions const&) const
{
    w.Write("{0}\r\n", instance_);

    for (ArbitrationRecord const & record : records_)
    {
        w.Write("{0}:{1}\r\n", record.Monitor, record.ExpireTime);
    }
}

ErrorCode SharableProxy::AribtrateAsyncOperation::GetRecords(
    Store::ILocalStore::TransactionSPtr const & trans,
    LeaseWrapper::LeaseAgentInstance const & instance,
    RecordVector & records)
{
    ILocalStore::EnumerationSPtr enumPtr;
    ErrorCode error = store_->CreateEnumerationByTypeAndKey(trans, ArbitrationRecordType, instance.Id, enumPtr);

    if (error.IsSuccess())
    {
        error = enumPtr->MoveNext();
    }

    if (error.IsSuccess())
    {
        vector<byte> bytes;
        bytes.reserve(4096);
        if (!(error = enumPtr->CurrentValue(bytes)).IsSuccess())
        {
            return error;
        }

        error = FabricSerializer::Deserialize(records, static_cast<ULONG>(bytes.size()), bytes.data());

        if (error.IsSuccess())
        {
            records.PostRead();

            if (records.Instance < instance.InstanceId)
            {
                records.Reset(instance.InstanceId);
            }
        }
    }
    else if (error.IsError(ErrorCodeValue::EnumerationCompleted))
    {
        error = ErrorCode::Success();
    }

    return error;
}

ErrorCode SharableProxy::AribtrateAsyncOperation::StoreRecords(
    Store::ILocalStore::TransactionSPtr const & trans,
    LeaseWrapper::LeaseAgentInstance const & instance,
    RecordVector const & records)
{
    if (!records.IsUpdated())
    {
        return ErrorCode::Success();
    }

    std::vector<byte> buffer;
    buffer.reserve(4096);
    ErrorCode error = FabricSerializer::Serialize(&records, buffer);
    if (!error.IsSuccess())
    {
        return error;
    }

    if (records.IsNew())
    {
        if (records.IsEmpty())
        {
            return ErrorCode::Success();
        }
        else
        {
            return store_->Insert(
                trans,
                ArbitrationRecordType,
                instance.Id,
                &(*buffer.begin()),
                buffer.size(),
                ILocalStore::OperationNumberUnspecified);
        }
    }
    else
    {
        if (records.IsEmpty())
        {
            return store_->Delete(
                trans,
                ArbitrationRecordType,
                instance.Id,
                ILocalStore::SequenceNumberIgnore);
        }
        else
        {
            return store_->Update(
                trans,
                ArbitrationRecordType,
                instance.Id,
                ILocalStore::SequenceNumberIgnore,
                instance.Id,
                &(*buffer.begin()),
                buffer.size(),
                ILocalStore::OperationNumberUnspecified);
        }
    }
}

ArbitrationReplyBody SharableProxy::AribtrateAsyncOperation::ProcessRequest(
    RecordVector & monitorRecords,
    RecordVector & subjectRecords,
    Common::DateTime expireTime)
{
    if (IsRevert())
    {
        if (monitorRecords.Instance == monitor_.InstanceId)
        {
            monitorRecords.RevertMonitor();
        }

        subjectRecords.RevertSubject(monitor_);

        WriteInfo("Revert",
            "{0} processed revert request {1}",
            voteId_, *this);

        return ArbitrationReplyBody(TimeSpan::Zero, false);
    }

    if (!monitorRecords.IsEmpty())
    {
        bool isDirectReject = monitorRecords.Contains(subject_);
        WriteInfo("Reject",
            "{0} rejected request {1}, isDirect {2}",
            voteId_, *this, isDirectReject);

        return ArbitrationReplyBody(TimeSpan::MaxValue, isDirectReject);
    }

    TimeSpan result;
    if (subjectRecords.Instance > subject_.InstanceId)
    {
        result = TimeSpan::MinValue;
    }
    else if (subjectRecords.IsReverted())
    {
        result = ttl_;
    }
    else
    {
        subjectRecords.Add(monitor_, expireTime);
        result = ttl_;
    }

    WriteInfo("Grant",
        "{0} granted request {1} with TTL {2}",
        voteId_, *this, result);

    return ArbitrationReplyBody(result, false);
}

ErrorCode SharableProxy::AribtrateAsyncOperation::ProcessRequest(Store::ILocalStore::TransactionSPtr const & trans)
{
    ErrorCode error = PreventDeadlock(trans);
    if (!error.IsSuccess())
    {
        WriteWarning(TraceFailure, "{0} failed to get lock for {1}: {2}", 
            voteId_, *this, error);
        return error;
    }

    DateTime now;
    error = GetProxyTime(now);
    if (!error.IsSuccess())
    {
        WriteWarning(TraceFailure, "{0} failed to get time for {1}: {2}", 
            voteId_, *this, error);
        return error;
    }

    DateTime expireTime = now + ttl_;

    RecordVector monitorRecords(monitor_.InstanceId);
    error = GetRecords(trans, monitor_, monitorRecords);
    if (!error.IsSuccess())
    {
        WriteWarning(TraceFailure, "{0} failed to get monitor record for {1}: {2}", 
            voteId_, *this, error);
        return error;
    }

    RecordVector subjectRecords(subject_.InstanceId);
    error = GetRecords(trans, subject_, subjectRecords);
    if (!error.IsSuccess())
    {
        WriteWarning(TraceFailure, "{0} failed to get subject record for {1}: {2}", 
            voteId_, *this, error);
        return error;
    }
    
    result_ = ProcessRequest(monitorRecords, subjectRecords, expireTime);

    error = StoreRecords(trans, monitor_, monitorRecords);
    if (!error.IsSuccess())
    {
        WriteWarning(TraceFailure, "{0} failed to save monitor record for {1}: {2}", 
            voteId_, *this, error);
        return error;
    }

    error =  StoreRecords(trans, subject_, subjectRecords);
    if (!error.IsSuccess())
    {
        WriteWarning(TraceFailure, "{0} failed to save subject record for {1}: {2}", 
            voteId_, *this, error);
    }

    return error;
}

ErrorCode SharableProxy::AribtrateAsyncOperation::ProcessRequest()
{
    ILocalStore::TransactionSPtr transPtr;
    ErrorCode result = store_->CreateTransaction(transPtr);

    if (!result.IsSuccess())
    {
        return result;
    }

    result = ProcessRequest(transPtr);
    if (result.IsSuccess())
    {
        result = transPtr->Commit();
    }
    else
    {
        transPtr->Rollback();
    }

    return result;
}

void SharableProxy::AribtrateAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    TimedAsyncOperation::OnStart(thisSPtr);

    AsyncOperationSPtr operation = thisSPtr;

    if (!UnreliableTransportConfig::GetConfig().IsDisabled())
    {
        UnreliableTransportConfig & config = UnreliableTransportConfig::GetConfig();
        TimeSpan delay = config.GetDelay(proxyId_.ToString(), voteId_.ToString(), FederationMessage::GetArbitrateRequest().Action);
    
        if (delay > TimeSpan::Zero)
        {
            if (delay == TimeSpan::MaxValue)
            {
                WriteInfo("Drop", "Drop arbitration request {0} from {1} to {2}",
                    *this, proxyId_, voteId_);
            }
            else
            {
                WriteInfo("Delay", "Drop arbitration request {0} from {1} to {2} by {3}",
                    *this, proxyId_, voteId_, delay);

                Threadpool::Post([operation, this]()
                { 
                    this->StartProcessing(operation);
                },
                delay);
            }

            return;
        }
    }

    Threadpool::Post([operation, this]()
    { 
        this->StartProcessing(operation);
    });
}

void SharableProxy::AribtrateAsyncOperation::StartProcessing(AsyncOperationSPtr const & operation)
{
    ErrorCode error = ProcessRequest();
    operation->TryComplete(operation, error);
}

ErrorCode SharableProxy::AribtrateAsyncOperation::PreventDeadlock(ILocalStore::TransactionSPtr const & transPtr)
{
    wstring type(ArbitrationRecordType);
    wstring key(LockKey);

    return store_->Update(transPtr, type, key, ILocalStore::SequenceNumberIgnore, key, nullptr, 0, ILocalStore::OperationNumberUnspecified);
}

ErrorCode SharableProxy::AribtrateAsyncOperation::GetProxyTime(DateTime &timeStamp)
{
    FILETIME time = store_->GetStoreUtcFILETIME();
    ULARGE_INTEGER convert;
    convert.LowPart = time.dwLowDateTime;
    convert.HighPart = time.dwHighDateTime;
    timeStamp = Common::DateTime(convert.QuadPart);
    return timeStamp == DateTime::Zero ? ErrorCodeValue::StoreUnexpectedError : ErrorCodeValue::Success;
}

void SharableProxy::AribtrateAsyncOperation::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.Write("{0}-{1}:{2}", monitor_, subject_, ttl_);
}
