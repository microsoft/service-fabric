// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace Federation;

StringLiteral TraceStore("Store");
StringLiteral TraceOperation("Operation");

VoterStoreSequenceEntry::VoterStoreSequenceEntry()
    : VoterStoreSequenceEntry(-1)
{
}

VoterStoreSequenceEntry::VoterStoreSequenceEntry(int64 value)
    : SerializedVoterStoreEntry(SerializableWithActivationTypes::Sequence), value_(value)
{
}


SerializedVoterStoreEntryUPtr VoterStoreSequenceEntry::Serialize() const
{
    return Clone();
}

ErrorCode VoterStoreSequenceEntry::Update(SerializedVoterStoreEntry const & value)
{
    ErrorCode result;

    VoterStoreSequenceEntry const* input = Convert(value);
    if (!input)
    {
        return ErrorCodeValue::InvalidArgument;
    }
    else if (input->value_ < value_)
    {
        result = ErrorCodeValue::StaleRequest;
    }
    else if (input->value_ == value_)
    {
        result = ErrorCodeValue::AlreadyExists;
    }
    else
    {
        value_ = input->value_;
    }

    return result;
}

SerializedVoterStoreEntryUPtr VoterStoreSequenceEntry::Clone() const
{
    return make_unique<VoterStoreSequenceEntry>(value_);
}

VoterStoreEntryUPtr VoterStoreSequenceEntry::Deserialize() const
{
    return make_unique<VoterStoreSequenceEntry>(value_);
}

void VoterStoreSequenceEntry::WriteTo(TextWriter& w, FormatOptions const&) const
{
    w.Write(value_);
}

VoterStoreSequenceEntry const* VoterStoreSequenceEntry::Convert(SerializedVoterStoreEntry const & value)
{
    if (value.TypeId != SerializableWithActivationTypes::Sequence)
    {
        return nullptr;
    }

    return dynamic_cast<VoterStoreSequenceEntry const *>(&value);
}

class VoterStoreSequenceEntryFactory : public SerializableActivationFactory
{
public:
    static VoterStoreSequenceEntryFactory* CreateSingleton()
    {
        VoterStoreSequenceEntryFactory* result = new VoterStoreSequenceEntryFactory();
        SerializableActivationFactory::Register(SerializableWithActivationTypes::Sequence, result);

        return result;
    }

    virtual SerializableWithActivation* CreateNew()
    {
        return new VoterStoreSequenceEntry();
    }

private:
    VoterStoreSequenceEntryFactory()
    {
    }
};

Global<VoterStoreSequenceEntryFactory> SequenceEntryFactorySingleton(VoterStoreSequenceEntryFactory::CreateSingleton());

SerializedVoterStoreEntryUPtr VoterStoreWStringEntry::Serialize() const
{
    return Clone();
}

ErrorCode VoterStoreWStringEntry::Update(SerializedVoterStoreEntry const & value)
{
    ErrorCode result;

    VoterStoreWStringEntry const* input = Convert(value);
    if (!input)
    {
        result = ErrorCodeValue::InvalidArgument;
    }
    else
    {
        value_ = input->value_;
    }

    return result;
}

SerializedVoterStoreEntryUPtr VoterStoreWStringEntry::Clone() const
{
    return make_unique<VoterStoreWStringEntry>(value_);
}

VoterStoreEntryUPtr VoterStoreWStringEntry::Deserialize() const
{
    return make_unique<VoterStoreWStringEntry>(value_);
}

void VoterStoreWStringEntry::WriteTo(TextWriter& w, FormatOptions const&) const
{
    w.Write(value_);
}

VoterStoreWStringEntry const* VoterStoreWStringEntry::Convert(SerializedVoterStoreEntry const & value)
{
    if (value.TypeId != SerializableWithActivationTypes::WString)
    {
        return nullptr;
    }

    return dynamic_cast<VoterStoreWStringEntry const *>(&value);
}

class VoterStoreWStringEntryFactory : public SerializableActivationFactory
{
public:
    static VoterStoreWStringEntryFactory* CreateSingleton()
    {
        VoterStoreWStringEntryFactory* result = new VoterStoreWStringEntryFactory();
        SerializableActivationFactory::Register(SerializableWithActivationTypes::WString, result);

        return result;
    }

    virtual SerializableWithActivation* CreateNew()
    {
        return new VoterStoreWStringEntry(L"");
    }

private:
    VoterStoreWStringEntryFactory()
    {
    }
};

Global<VoterStoreWStringEntryFactory> WStringEntryFactorySingleton(VoterStoreWStringEntryFactory::CreateSingleton());


struct IntroduceReplyBody : public Serialization::FabricSerializable
{
    DENY_COPY(IntroduceReplyBody);

public:
    IntroduceReplyBody() {}

    IntroduceReplyBody & operator = (IntroduceReplyBody && other)
    {
        if (this != &other)
        {
            this->Config = move(other.Config);
            this->Voters = move(other.Voters);
            this->LeaderInstance = other.LeaderInstance;
        }

        return *this;
    }

    FABRIC_FIELDS_03(Config, Voters, LeaderInstance);

    VoterStore::ReplicaSetConfiguration Config;
    vector<NodeInstance> Voters;
    int64 LeaderInstance;
};

struct BootstrapRequestBody : public Serialization::FabricSerializable
{
    DENY_COPY(BootstrapRequestBody);

public:
    BootstrapRequestBody() {}

    BootstrapRequestBody & operator = (BootstrapRequestBody && other)
    {
        if (this != &other)
        {
            this->LeaderInstance = other.LeaderInstance;
        }

        return *this;
    }

    FABRIC_FIELDS_01(LeaderInstance);

    int64 LeaderInstance;
};

struct BootstrapReplyBody : public Serialization::FabricSerializable
{
    DENY_COPY(BootstrapReplyBody);

public:
    BootstrapReplyBody() {}

    BootstrapReplyBody & operator = (BootstrapReplyBody && other)
    {
        if (this != &other)
        {
            this->LeaderInstance = other.LeaderInstance;
            this->Config = move(other.Config);
            this->Accepted = other.Accepted;
        }

        return *this;
    }

    FABRIC_FIELDS_03(LeaderInstance, Config, Accepted);

    int64 LeaderInstance;
    VoterStore::ReplicaSetConfiguration Config;
    bool Accepted;
};

struct LeaderProbeRequestBody : public Serialization::FabricSerializable
{
    DENY_COPY(LeaderProbeRequestBody);

public:
    LeaderProbeRequestBody() {}

    LeaderProbeRequestBody(int64 leaderInstance)
        : LeaderInstance(leaderInstance)
    {
    }

    LeaderProbeRequestBody & operator = (LeaderProbeRequestBody && other)
    {
        if (this != &other)
        {
            this->LeaderInstance = other.LeaderInstance;
        }

        return *this;
    }

    FABRIC_FIELDS_01(LeaderInstance);

    int64 LeaderInstance;
};

struct LeaderProbeReplyBody : public Serialization::FabricSerializable
{
    DENY_COPY(LeaderProbeReplyBody);

public:
    LeaderProbeReplyBody() {}

    LeaderProbeReplyBody & operator = (LeaderProbeReplyBody && other)
    {
        if (this != &other)
        {
            this->LeaderInstance = other.LeaderInstance;
            this->IsLeader = other.IsLeader;
        }

        return *this;
    }

    FABRIC_FIELDS_02(LeaderInstance, IsLeader);

    int64 LeaderInstance;
    bool IsLeader;
};

struct SyncEntry : public Serialization::FabricSerializable
{
    DENY_COPY(SyncEntry);

public:
    SyncEntry() {}

    SyncEntry(wstring const & key, int64 sequence, SerializedVoterStoreEntryUPtr && value)
        : Key(key), Sequence(sequence), Value(move(value))
    {
    }

    SyncEntry(SyncEntry && other)
        : Key(move(other.Key)), Sequence(other.Sequence), Value(move(other.Value))
    {
    }

    SyncEntry & operator = (SyncEntry && other)
    {
        if (this != &other)
        {
            this->Key = move(other.Key);
            this->Sequence = other.Sequence;
            this->Value = move(other.Value);
        }

        return *this;
    }

    FABRIC_FIELDS_03(Key, Sequence, Value);

    wstring Key;
    int64 Sequence;
    SerializedVoterStoreEntryUPtr Value;
};

DEFINE_USER_ARRAY_UTILITY(SyncEntry);

struct SyncBody : public Serialization::FabricSerializable
{
    DENY_COPY(SyncBody);

public:
    SyncBody() {}

    SyncBody & operator = (SyncBody && other)
    {
        if (this != &other)
        {
            this->Config = move(other.Config);
            this->StartSequence = other.StartSequence;
            this->EndSequence = other.EndSequence;
            this->Entries = move(other.Entries);
        }

        return *this;
    }

    FABRIC_FIELDS_04(Config, StartSequence, EndSequence, Entries);

    VoterStore::ReplicaSetConfiguration Config;
    int64 StartSequence;
    int64 EndSequence;
    vector<SyncEntry> Entries;
};

struct SyncReplyBody : public Serialization::FabricSerializable
{
    DENY_COPY(SyncReplyBody);

public:
    SyncReplyBody() {}

    SyncReplyBody & operator = (SyncReplyBody && other)
    {
        if (this != &other)
        {
            this->Sequence = move(other.Sequence);
        }

        return *this;
    }

    FABRIC_FIELDS_01(Sequence);

    int64 Sequence;
};

struct ProgressRequestBody : public Serialization::FabricSerializable
{
    DENY_COPY(ProgressRequestBody);

public:
    ProgressRequestBody() {}

    ProgressRequestBody & operator = (ProgressRequestBody && other)
    {
        if (this != &other)
        {
            this->LeaderInstance = other.LeaderInstance;
            this->Sequence = other.Sequence;
        }

        return *this;
    }

    FABRIC_FIELDS_02(LeaderInstance, Sequence);

    int64 LeaderInstance;
    int64 Sequence;
};

struct ProgressReplyBody : public Serialization::FabricSerializable
{
    DENY_COPY(ProgressReplyBody);

public:
    ProgressReplyBody() {}

    ProgressReplyBody & operator = (ProgressReplyBody && other)
    {
        if (this != &other)
        {
            this->EndSequence = other.EndSequence;
            this->Entries = move(other.Entries);
        }

        return *this;
    }

    FABRIC_FIELDS_02(EndSequence, Entries);

    int64 EndSequence;
    vector<SyncEntry> Entries;
};

struct VoterStoreReadRequestBody : public Serialization::FabricSerializable
{
    DENY_COPY(VoterStoreReadRequestBody);

public:
    VoterStoreReadRequestBody()
    {
    }

    VoterStoreReadRequestBody(wstring const & key)
        : Key(key)
    {
    }

    VoterStoreReadRequestBody & operator = (VoterStoreReadRequestBody && other)
    {
        if (this != &other)
        {
            this->Key = move(other.Key);
        }

        return *this;
    }

    FABRIC_FIELDS_01(Key);

    wstring Key;
};

struct VoterStoreReadReplyBody : public Serialization::FabricSerializable
{
    DENY_COPY(VoterStoreReadReplyBody);

public:
    VoterStoreReadReplyBody()
    {
    }

    VoterStoreReadReplyBody(int64 generation)
        : Generation(generation), Sequence(0)
    {
    }

    VoterStoreReadReplyBody & operator = (VoterStoreReadReplyBody && other)
    {
        if (this != &other)
        {
            this->Error = other.Error;
            this->Value = move(other.Value);
            this->Sequence = other.Sequence;
            this->Generation = other.Generation;
        }

        return *this;
    }

    FABRIC_FIELDS_04(Error, Value, Sequence, Generation);

    ErrorCode Error;
    SerializedVoterStoreEntryUPtr Value;
    int64 Sequence;
    int64 Generation;
};

struct VoterStoreWriteRequestBody : public Serialization::FabricSerializable
{
    DENY_COPY(VoterStoreWriteRequestBody);

public:
    VoterStoreWriteRequestBody()
    {
    }
    
    VoterStoreWriteRequestBody(wstring const & key, int64 checkSequence, SerializedVoterStoreEntryUPtr && value)
        : Key(key), CheckSequence(checkSequence), Value(move(value))
    {
    }

    VoterStoreWriteRequestBody & operator = (VoterStoreWriteRequestBody && other)
    {
        if (this != &other)
        {
            this->Key = other.Key;
            this->CheckSequence = other.CheckSequence;
            this->Value = move(other.Value);
        }

        return *this;
    }

    FABRIC_FIELDS_03(Key, Value, CheckSequence);

    wstring Key;
    int64 CheckSequence;
    SerializedVoterStoreEntryUPtr Value;
};

struct VoterStoreWriteReplyBody : public Serialization::FabricSerializable
{
    DENY_COPY(VoterStoreWriteReplyBody);

public:
    VoterStoreWriteReplyBody()
        : Sequence(0)
    {
    }

    VoterStoreWriteReplyBody & operator = (VoterStoreWriteReplyBody && other)
    {
        if (this != &other)
        {
            this->Error = other.Error;
            this->Sequence = other.Sequence;
            this->Value = move(other.Value);
        }

        return *this;
    }

    FABRIC_FIELDS_03(Error, Sequence, Value);

    ErrorCode Error;
    int64 Sequence;
    SerializedVoterStoreEntryUPtr Value;
};

VoterStoreRequestAsyncOperation::VoterStoreRequestAsyncOperation(
    SiteNode & site,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
    : site_(site), TimedAsyncOperation(timeout, callback, parent)
{
}

void VoterStoreRequestAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    TimedAsyncOperation::OnStart(thisSPtr);
    Resolve(dynamic_pointer_cast<VoterStoreRequestAsyncOperation>(thisSPtr));
}

void VoterStoreRequestAsyncOperation::Resolve(shared_ptr<VoterStoreRequestAsyncOperation> const & thisSPtr)
{
    site_.GetVoterStore().Resolve(thisSPtr);
}

void VoterStoreRequestAsyncOperation::SendRequest(MessageUPtr && request, NodeInstance const & target, shared_ptr<VoterStoreRequestAsyncOperation> const & thisSPtr)
{
    site_.BeginRouteRequest(move(request), target.Id, target.InstanceId, true, RemainingTime,
        [this, target](AsyncOperationSPtr const & operation)
    {
        this->RequestCallback(operation, target);
    },
        thisSPtr);
}

void VoterStoreRequestAsyncOperation::RequestCallback(AsyncOperationSPtr const & operation, NodeInstance const & target)
{
    VoterStoreRequestAsyncOperationSPtr const & thisSPtr = dynamic_pointer_cast<VoterStoreRequestAsyncOperation>(operation->Parent);

    MessageUPtr reply;
    ErrorCode error = site_.EndRouteRequest(operation, reply);

    if (error.IsSuccess())
    {
        ProcessReply(move(reply), target, thisSPtr);
    }
    else if (site_.IsShutdown)
    {
        TryComplete(thisSPtr, ErrorCodeValue::OperationCanceled);
    }
    else if (error.IsError(ErrorCodeValue::RoutingNodeDoesNotMatchFault))
    {
        site_.GetVoterStore().ReportDownVoter(target);
        Resolve(thisSPtr);
    }
    else if (!error.IsError(ErrorCodeValue::Timeout))
    {
        Threadpool::Post([thisSPtr]
        {
            thisSPtr->Resolve(thisSPtr);
        },
        FederationConfig::GetConfig().VoterStoreRetryInterval);
    }
}

class VoterStoreReadAsyncOperation : public VoterStoreRequestAsyncOperation
{
    DENY_COPY(VoterStoreReadAsyncOperation);

public:
    VoterStoreReadAsyncOperation(
        SiteNode & site,
        std::wstring const & key,
        Common::TimeSpan timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
        : VoterStoreRequestAsyncOperation(site, timeout, callback, parent), key_(key), sequence_(0), generation_(0)
    {
    }

    int64 GetSequence() const
    {
        return sequence_;
    }

    int64 GetGeneration() const
    {
        return generation_;
    }

    SerializedVoterStoreEntryUPtr const & GetResult() const
    { 
        return result_; 
    }

    SerializedVoterStoreEntryUPtr MoveResult()
    {
        return move(result_);
    }

    virtual void OnPrimaryResolved(NodeInstance const & primary, shared_ptr<VoterStoreRequestAsyncOperation> const & thisSPtr)
    {
        MessageUPtr request = FederationMessage::GetVoterStoreReadRequest().CreateMessage(VoterStoreReadRequestBody(key_));
        request->Idempotent = true;
        SendRequest(move(request), primary, thisSPtr);
    }

    virtual void ProcessReply(MessageUPtr && reply, NodeInstance const &, shared_ptr<VoterStoreRequestAsyncOperation> const & thisSPtr)
    {
        VoterStoreReadReplyBody body;
        if (!reply->GetBody(body))
        {
            TryComplete(thisSPtr, reply->FaultErrorCodeValue);
        }
        else
        {
            generation_ = body.Generation;

            if (body.Error.IsSuccess())
            {
                result_ = move(body.Value);
                sequence_ = body.Sequence;
            }

            TryComplete(thisSPtr, body.Error);
        }
    }

private:
    wstring key_;
    int64 sequence_;
    int64 generation_;
    SerializedVoterStoreEntryUPtr result_;
};

class VoterStoreWriteAsyncOperation : public VoterStoreRequestAsyncOperation
{
    DENY_COPY(VoterStoreWriteAsyncOperation);

public:
    VoterStoreWriteAsyncOperation(
        SiteNode & site,
        std::wstring const & key,
        int64 checkSequence,
        SerializedVoterStoreEntryUPtr && value,
        bool isIdempotent,
        Common::TimeSpan timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
        :   VoterStoreRequestAsyncOperation(site, timeout, callback, parent),
            key_(key),
            checkSequence_(checkSequence),
            value_(move(value)),
            isIdempotent_(isIdempotent),
            sequence_(0)
    {
    }

    int64 GetSequence() const
    {
        return sequence_;
    }

    SerializedVoterStoreEntryUPtr const & GetResult() const
    {
        return result_;
    }

    SerializedVoterStoreEntryUPtr MoveResult()
    {
        return move(result_);
    }

    virtual void OnPrimaryResolved(NodeInstance const & primary, shared_ptr<VoterStoreRequestAsyncOperation> const & thisSPtr)
    {
        MessageUPtr request = FederationMessage::GetVoterStoreWriteRequest().CreateMessage(VoterStoreWriteRequestBody(key_, checkSequence_, value_->Clone()));
        if (isIdempotent_)
        {
            request->Idempotent = true;
        }
        SendRequest(move(request), primary, thisSPtr);
    }

    virtual void ProcessReply(MessageUPtr && reply, NodeInstance const &, shared_ptr<VoterStoreRequestAsyncOperation> const & thisSPtr)
    {
        VoterStoreWriteReplyBody body;
        if (!reply->GetBody(body))
        {
            TryComplete(thisSPtr, reply->FaultErrorCodeValue);
        }
        else
        {
            sequence_ = body.Sequence;
            result_ = move(body.Value);

            TryComplete(thisSPtr, body.Error);
        }
    }

private:
    wstring key_;
    int64 checkSequence_;
    SerializedVoterStoreEntryUPtr value_;
    bool isIdempotent_;
    int64 sequence_;
    SerializedVoterStoreEntryUPtr result_;
};

VoterStoreReadWriteAsyncOperation::VoterStoreReadWriteAsyncOperation(
    SiteNode & site,
    wstring const & key,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
    : TimedAsyncOperation(timeout, callback, parent), site_(site), key_(key), checkSequence_(-1)
{
}

void VoterStoreReadWriteAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    TimedAsyncOperation::OnStart(thisSPtr);
    StartRead(thisSPtr);
}

void VoterStoreReadWriteAsyncOperation::StartRead(AsyncOperationSPtr const & thisSPtr)
{
    AsyncOperation::CreateAndStart<VoterStoreReadAsyncOperation>(
        site_,
        key_,
        RemainingTime,
        [this](AsyncOperationSPtr const & operation) { this->OnReadCompleted(operation); },
        thisSPtr);
}

void VoterStoreReadWriteAsyncOperation::StartWrite(AsyncOperationSPtr const & thisSPtr, SerializedVoterStoreEntryUPtr && value)
{
    AsyncOperation::CreateAndStart<VoterStoreWriteAsyncOperation>(
        site_,
        key_,
        checkSequence_,
        move(value),
        true,
        RemainingTime,
        [this](AsyncOperationSPtr const & operation) { this->OnWriteCompleted(operation); },
        thisSPtr);
}

void VoterStoreReadWriteAsyncOperation::OnReadCompleted(AsyncOperationSPtr const & operation)
{
    AsyncOperationSPtr const & thisSPtr = operation->Parent;
    auto readOperationPtr = AsyncOperation::End<VoterStoreReadAsyncOperation>(operation);

    VoteManager::WriteInfo(TraceOperation, site_.IdString,
        "Completed read for {0} with {1} result {2} seq {3}",
        key_, readOperationPtr->Error, readOperationPtr->GetResult(), readOperationPtr->GetSequence());

    if (readOperationPtr->Error.IsSuccess())
    {
        checkSequence_ = readOperationPtr->GetSequence();
        result_ = GenerateValue(readOperationPtr->GetResult());
        if (!result_)
        {
            TryComplete(thisSPtr, ErrorCodeValue::OperationCanceled);
        }
        else
        {
            VoteManager::WriteInfo(TraceOperation, site_.IdString,
                "Start write for {0} with {1} check sequence {2}",
                key_, *result_, checkSequence_);

            StartWrite(thisSPtr, result_->Clone());
        }
    }
    else if (site_.IsShutdown)
    {
        TryComplete(thisSPtr, ErrorCodeValue::OperationCanceled);
    }
    else if (!readOperationPtr->Error.IsError(ErrorCodeValue::Timeout) && ShouldRetry())
    {
        Threadpool::Post([this, thisSPtr] { this->StartRead(thisSPtr); }, FederationConfig::GetConfig().VoterStoreRetryInterval);
    }
    else
    {
        TryComplete(thisSPtr, readOperationPtr->Error);
    }
}

void VoterStoreReadWriteAsyncOperation::OnWriteCompleted(AsyncOperationSPtr const & operation)
{
    AsyncOperationSPtr const & thisSPtr = operation->Parent;
    auto writeOperationPtr = AsyncOperation::End<VoterStoreWriteAsyncOperation>(operation);

    VoteManager::WriteInfo(TraceOperation, site_.IdString,
        "Completed write for {0} with {1} result {2}",
        key_, *result_, writeOperationPtr->Error);

    if (writeOperationPtr->Error.IsSuccess())
    {
        TryComplete(thisSPtr);
    }
    else if (site_.IsShutdown)
    {
        TryComplete(thisSPtr, ErrorCodeValue::OperationCanceled);
    }
    else if (!writeOperationPtr->Error.IsError(ErrorCodeValue::Timeout) && ShouldRetry())
    {
        TimeSpan delay;
        if (writeOperationPtr->Error.IsError(ErrorCodeValue::StoreWriteConflict))
        {
            checkSequence_ = writeOperationPtr->GetSequence();
            SerializedVoterStoreEntryUPtr const & conflict = writeOperationPtr->GetResult();
            result_ = GenerateValue(conflict);
            if (!result_)
            {
                TryComplete(thisSPtr, ErrorCodeValue::OperationCanceled);
                return;
            }

            VoteManager::WriteInfo(TraceOperation, site_.IdString,
                "Updated write value for {0} with conflict {1} to {2}",
                key_, *conflict, *result_);

            delay = TimeSpan::Zero;
        }
        else
        {
            delay = FederationConfig::GetConfig().VoterStoreRetryInterval;
        }

        Threadpool::Post([this, thisSPtr] {this->StartWrite(thisSPtr, this->result_->Clone()); }, delay);
    }
    else
    {
        TryComplete(thisSPtr, writeOperationPtr->Error);
    }
}

bool VoterStoreReadWriteAsyncOperation::ShouldRetry()
{
    return false;
}

class SendVoterStoreMessageAction : public StateMachineAction
{
    DENY_COPY(SendVoterStoreMessageAction);

public:
    SendVoterStoreMessageAction(MessageUPtr && message, VoterStore & store, NodeInstance const & target, bool isRequest)
        : message_(move(message)), target_(target), isRequest_(isRequest)
    {
        if (message_)
        {
            store.AddHeader(*message_);
        }
    }

    void Execute(SiteNode & siteNode)
    {
        if (!message_)
        {
            message_ = FederationMessage::GetEmptyRequest().CreateMessage(siteNode.GetVoteManager().GenerateGlobalLease(target_), true);
        }

        siteNode.GetVoterStore().SendMessage(move(message_), target_, isRequest_);
    }

    wstring ToString()
    {
        return wformatString("{0}:{1}", message_ ? message_->Action : FederationMessage::GetEmptyRequest().Action, target_);
    }

private:
    MessageUPtr message_;
    NodeInstance target_;
    bool isRequest_;
};

class ReplyAction : public StateMachineAction
{
    DENY_COPY(ReplyAction);

public:
    ReplyAction(RequestReceiverContextUPtr && requestContext, MessageUPtr && reply)
        : requestContext_(move(requestContext)), reply_(move(reply))
    {
    }

    void Execute(SiteNode &)
    {
        requestContext_->Reply(move(reply_));
    }

    wstring ToString()
    {
        return wformatString("{0}:{1}", reply_->Action, requestContext_->FromInstance);
    }

private:
    RequestReceiverContextUPtr requestContext_;
    MessageUPtr reply_;
};

class SendRequestsAfterResolveAction : public StateMachineAction
{
    DENY_COPY(SendRequestsAfterResolveAction);

public:
    SendRequestsAfterResolveAction(vector<VoterStoreRequestAsyncOperationSPtr> && requests, NodeInstance const & primary)
        : requests_(move(requests)), primary_(primary)
    {
    }

    void Execute(SiteNode &)
    {
        for (auto it = requests_.begin(); it != requests_.end(); ++it)
        {
            (*it)->OnPrimaryResolved(primary_, *it);
        }
    }

    wstring ToString()
    {
        return wformatString("{0} requests to {1}", requests_.size(), primary_);
    }

private:
    vector<VoterStoreRequestAsyncOperationSPtr> requests_;
    NodeInstance primary_;
};

SerializedVoterStoreEntry::SerializedVoterStoreEntry(SerializableWithActivationTypes::Enum typeId)
    : SerializableWithActivation(typeId)
{
}

VoterStore::ReplicaSetConfiguration & VoterStore::ReplicaSetConfiguration::operator = (ReplicaSetConfiguration && other)
{
    if (this != &other)
    {
        this->Generation = other.Generation;
        this->Epoch = other.Epoch;
        this->Replicas = move(other.Replicas);
    }

    return *this;
}

VoterStore::Voter::Voter(NodeInstance const & instance)
    : Instance(instance), IsDown(false), LastSend(DateTime::Zero), LastActionSequence(0)
{
}

void VoterStore::Voter::AddSendMessageAction(StateMachineActionCollection & actions, MessageUPtr && message, VoterStore & store, bool isRequest) const
{
    FederationConfig const & config = FederationConfig::GetConfig();
    TimeSpan interval = (message ? config.VoterStoreRetryInterval : config.VoterStoreLivenessCheckInterval);

    DateTime now = DateTime::Now();
    if (store.highestSequence_ > LastActionSequence || LastSend + interval < now)
    {
        LastSend = now;
        LastActionSequence = store.highestSequence_;

        NodeInstance target = (IsDown ? NodeInstance(Instance.Id, 0) : Instance);
        actions.Add(make_unique<SendVoterStoreMessageAction>(move(message), store, target, isRequest));
    }
}

VoterStore::VoterInfo::VoterInfo(NodeInstance const & instance)
    : Voter(instance), IsAcked(false), IsIntroduced(false), IsStale(false)
{
}

VoterStore::Replica::Replica(NodeInstance const & instance)
    : Voter(instance), Sequence(-1)
{
}

VoterStore::ReplicaSet::ReplicaSet()
    : generation_(0), epoch_(0)
{
}

VoterStore::Replica* VoterStore::ReplicaSet::GetReplica(NodeId const & nodeId)
{
    for (auto it = replicas_.begin(); it != replicas_.end(); ++it)
    {
        if (it->Instance.Id == nodeId)
        {
            return &(*it);
        }
    }

    return nullptr;
}

VoterStore::Replica* VoterStore::ReplicaSet::GetReplica(NodeInstance const & instance)
{
    Replica* replica = GetReplica(instance.Id);
    if (replica && replica->Instance.InstanceId == instance.InstanceId)
    {
        return replica;
    }

    return nullptr;
}

VoterStore::Replica* VoterStore::ReplicaSet::GetLeader()
{
    for (auto it = replicas_.begin(); it != replicas_.end(); ++it)
    {
        if (!it->IsDown)
        {
            return &(*it);
        }
    }

    return nullptr;
}

void VoterStore::ReplicaSet::GetDownReplicas(vector<NodeInstance> & downReplicas)
{
    for (Replica const & replica : replicas_)
    {
        if (replica.IsDown)
        {
            downReplicas.push_back(replica.Instance);
        }
    }
}

bool VoterStore::ReplicaSet::IsValid() const
{
    return (replicas_.size() > 0);
}

VoterStore::Replica* VoterStore::ReplicaSet::GetPrimary()
{
    if (!IsValid() || replicas_[0].IsDown)
    {
        return nullptr;
    }

    return replicas_.data();
}

bool VoterStore::ReplicaSet::IsActive() const
{
    for (auto it = replicas_.begin(); it != replicas_.end(); ++it)
    {
        if (!it->IsDown)
        {
            return true;
        }
    }

    return false;
}

bool VoterStore::ReplicaSet::HasSecondary() const
{
    for (size_t i = 1; i < replicas_.size(); i++)
    {
        if (!replicas_[i].IsDown)
        {
            return true;
        }
    }

    return false;
}

int64 VoterStore::ReplicaSet::GetCommitedSequence(int64 highestSequence) const
{
    int64 result = highestSequence;

    for (size_t i = 1; i < replicas_.size(); i++)
    {
        if (!replicas_[i].IsDown && replicas_[i].Sequence < result)
        {
            result = replicas_[i].Sequence;
        }
    }

    return result;
}

void VoterStore::ReplicaSet::ResetLastSend()
{
    for (auto it = replicas_.begin(); it != replicas_.end(); ++it)
    {
        it->LastSend = DateTime::Zero;
    }
}

bool VoterStore::ReplicaSet::Update(ReplicaSetConfiguration const & config)
{
    if (config.Generation == generation_ && config.Epoch <= epoch_)
    {
        return false;
    }

    replicas_.clear();
    for (NodeInstance const & instance : config.Replicas)
    {
        replicas_.push_back(Replica(instance));
    }

    generation_ = config.Generation;
    epoch_ = config.Epoch;

    return true;
}

void VoterStore::ReplicaSet::GetConfiguration(ReplicaSetConfiguration & config) const
{
    for (Replica const & replica : replicas_)
    {
        config.Replicas.push_back(replica.Instance);
    }

    config.Generation = generation_;
    config.Epoch = epoch_;
}

size_t VoterStore::ReplicaSet::GetUpReplicaCount() const
{
    size_t result = 0;
    for (Replica const & replica : replicas_)
    {
        if (!replica.IsDown)
        {
            result++;
        }
    }

    return result;
}

bool VoterStore::ReplicaSet::Add(NodeInstance const & target)
{
    for (auto it = replicas_.begin(); it != replicas_.end(); ++it)
    {
        if (it->Instance.Id == target.Id)
        {
            if (it->Instance.InstanceId >= target.InstanceId)
            {
                return false;
            }

            replicas_.erase(it);
            break;
        }
    }

    replicas_.push_back(Replica(target));
    return true;
}

void VoterStore::ReplicaSet::RemoveDownReplicas()
{
    for (auto it = replicas_.begin(); it != replicas_.end();)
    {
        if (it->IsDown)
        {
            it = replicas_.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void VoterStore::ReplicaSet::Clear()
{
    replicas_.clear();
    epoch_ = 0;
    generation_ = 0;
}

VoterStore::PendingWriteRequest::PendingWriteRequest(int64 checkSequence, SerializedVoterStoreEntryUPtr && value, RequestReceiverContextUPtr && context)
    : CheckSequence(checkSequence), Value(move(value)), Context(move(context))
{
}

VoterStore::PendingWriteRequest::PendingWriteRequest(PendingWriteRequest && other)
    : CheckSequence(other.CheckSequence), Value(move(other.Value)), Context(move(other.Context))
{
}

VoterStore::PendingWriteRequest & VoterStore::PendingWriteRequest::operator = (PendingWriteRequest && other)
{
    if (this != &other)
    {
        this->CheckSequence = other.CheckSequence;
        this->Value = move(other.Value);
        this->Context = move(other.Context);
    }

    return *this;
}

VoterStore::StoreEntry::StoreEntry()
    : Sequence(0), PendingSequence(0)
{
}

VoterStore::StoreEntry::StoreEntry(StoreEntry && other)
    : Sequence(other.Sequence),
      Current(move(other.Current)),
      PendingSequence(other.PendingSequence),
      Pending(move(other.Pending)),
      ActiveRequest(move(other.ActiveRequest)),
      PendingRequests(move(other.PendingRequests))
{
}

VoterStore::StoreEntry & VoterStore::StoreEntry::operator = (StoreEntry && other)
{
    if (this != &other)
    {
        this->Sequence = other.Sequence;
        this->Current = move(other.Current);
        this->PendingSequence = other.PendingSequence;
        this->Pending = move(other.Pending);
        this->ActiveRequest = move(other.ActiveRequest);
        this->PendingRequests = move(other.PendingRequests);
    }

    return *this;
}

SerializedVoterStoreEntryUPtr VoterStore::StoreEntry::Serialize() const
{
    if (Pending)
    {
        return Pending->Serialize();
    }

    if (Current)
    {
        return Current->Serialize();
    }

    return nullptr;
}

VoterStore::VoterStore(SiteNode & site)
    : site_(site),
      phase_(StorePhase::Uninitialized),
      leader_(site.Instance),
      leaderInstance_(0),
      highestSequence_(0),
      configurationSequence_(0),
      bootstrapStartTime_(StopwatchTime::MaxValue),
      lastTraceTime_(DateTime::Zero)
{
}

void VoterStore::Start()
{
    vector<NodeId> seedNodes;
    size_t count = static_cast<size_t>(site_.GetVoteManager().GetSeedNodes(seedNodes));
    vector<VoterStoreRequestAsyncOperationSPtr> pendingRequests;
    {
        AcquireWriteLock grab(this->lock_);

        VoteManager::WriteInfo(TraceStore, site_.IdString, "Start VoterStore");

        InitializeVoters(seedNodes, count > seedNodes.size());

        if (phase_ != StorePhase::Invalid)
        {
            auto rootSPtr = site_.GetSiteNodeSPtr();
            timer_ = Timer::Create(
                "VoterStore",
                [this, rootSPtr](TimerSPtr const &)
            {
                AcquireWriteLock grab(this->lock_);
                this->RunStateMachine();
            },
                true);

            if (leader_.Id == site_.Id)
            {
                leader_ = site_.Instance;
            }

            if (site_.GetVoteManager().IsSeedNode(site_.Id, site_.RingName))
            {
                phase_ = StorePhase::Introduce;
            }
            else
            {
                phase_ = StorePhase::None;
            }

            RunStateMachine();
        }
        else
        {
            pendingRequests.swap(pendingRequests_);
        }
    }

    CompletePendingRequests(pendingRequests, ErrorCodeValue::InvalidConfiguration);
}

void VoterStore::InitializeVoters(vector<NodeId> const & seedNodes, bool hasSharedVoter)
{
    if (hasSharedVoter)
    {
        phase_ = StorePhase::Invalid;
    }

    for (NodeId nodeId : seedNodes)
    {
        if (nodeId != site_.Id)
        {
            voters_.push_back(VoterInfo(NodeInstance(nodeId, 0)));
        }
    }
}

void VoterStore::UpdateVoterConfig()
{
    vector<NodeId> seedNodes;
    site_.GetVoteManager().GetSeedNodes(seedNodes);

    AcquireWriteLock grab(lock_);

    for (auto it = voters_.begin(); it != voters_.end(); ++it)
    {
        it->IsStale = true;
    }

    bool isSeedNode = false;
    for (NodeId nodeId : seedNodes)
    {
        if (nodeId != site_.Id)
        {
            VoterInfo* voter = GetVoterInfo(nodeId);
            if (voter)
            {
                voter->IsStale = false;
            }
            else
            {
                voters_.push_back(VoterInfo(NodeInstance(nodeId, 0)));
            }
        }
        else
        {
            isSeedNode = true;
        }
    }

    if (isSeedNode && phase_ == StorePhase::None)
    {
        phase_ = StorePhase::Introduce;
        RunStateMachine();
    }
}

void VoterStore::Stop()
{
    vector<VoterStoreRequestAsyncOperationSPtr> pendingRequests;
    {
        AcquireWriteLock grab(this->lock_);

        VoteManager::WriteInfo(TraceStore, site_.IdString, "Stop VoterStore");

        if (timer_)
        {
            timer_->Cancel();
        }

        phase_ = StorePhase::Invalid;

        pendingRequests.swap(pendingRequests_);
    }

    CompletePendingRequests(pendingRequests, ErrorCodeValue::OperationCanceled);
}

void VoterStore::CompletePendingRequests(vector<VoterStoreRequestAsyncOperationSPtr> const & pendingRequests, ErrorCode error)
{
    for (VoterStoreRequestAsyncOperationSPtr const & request : pendingRequests)
    {
        request->TryComplete(request, error);
    }
}

void VoterStore::Resolve(VoterStoreRequestAsyncOperationSPtr const & operation)
{
    if (phase_ == StorePhase::Invalid)
    {
        operation->TryComplete(operation, ErrorCodeValue::InvalidConfiguration);
        return;
    }

    NodeInstance primaryInstance;
    {
        AcquireWriteLock grab(lock_);
        Replica* primary = replicaSet_.GetPrimary();
        if (primary && site_.Table.IsDown(primary->Instance))
        {
            UpdateDownVoter(primary->Instance);
            primary = nullptr;
        }

        if (!primary)
        {
            pendingRequests_.push_back(operation);
            if (pendingRequests_.size() == 1)
            {
                RunStateMachine();
            }
            return;
        }

        primaryInstance = primary->Instance;
    }

    operation->OnPrimaryResolved(primaryInstance, operation);
}

bool VoterStore::IsActiveReplica() const
{
    return (phase_ == StorePhase::Primary || phase_ == StorePhase::BecomePrimary || phase_ == StorePhase::Secondary);
}

VoterStore::VoterInfo* VoterStore::GetVoterInfo(NodeId const & nodeId)
{
    for (auto it = voters_.begin(); it != voters_.end(); ++it)
    {
        if (it->Instance.Id == nodeId)
        {
            return &(*it);
        }
    }

    return nullptr;
}

VoterStore::VoterInfo* VoterStore::GetVoterInfo(NodeInstance const & instance)
{
    VoterInfo* result = GetVoterInfo(instance.Id);
    if (result && result->Instance.InstanceId == instance.InstanceId)
    {
        return result;
    }

    return nullptr;
}

bool VoterStore::IsVoterDown(NodeInstance const & instance)
{
    VoterInfo* voter = GetVoterInfo(instance.Id);
    if (voter)
    {
        if (voter->Instance.InstanceId == instance.InstanceId)
        {
            return voter->IsDown;
        }

        return (voter->Instance.InstanceId > instance.InstanceId);
    }

    return false;
}

void VoterStore::GetVoters(vector<NodeInstance> & voters) const
{
    for (auto it = voters_.begin(); it != voters_.end(); ++it)
    {
        if (!it->IsDown)
        {
            voters.push_back(it->Instance);
        }
    }
}

void VoterStore::UpdateDownVoter(NodeInstance const & target)
{
    Replica* replica = replicaSet_.GetReplica(target.Id);
    if (replica && replica->Instance.InstanceId <= target.InstanceId)
    {
        replica->IsDown = true;
    }

    VoterInfo* voter = GetVoterInfo(target.Id);
    if (voter && voter->Instance.InstanceId <= target.InstanceId)
    {
        voter->IsDown = true;
    }

    if (leader_.Id == target.Id && leader_.InstanceId <= target.InstanceId)
    {
        leader_ = site_.Instance;
    }
}

void VoterStore::ReportDownVoter(NodeInstance const & target)
{
    AcquireWriteLock grab(lock_);

    UpdateDownVoter(target);
    RunStateMachine();
}

void VoterStore::UpdateVoterInstance(NodeInstance const & target, bool addNew)
{
    VoterInfo* voter = GetVoterInfo(target.Id);
    if (voter)
    {
        if (voter->Instance.InstanceId < target.InstanceId)
        {
            if (voter->IsDown || voter->Instance.InstanceId > 0)
            {
                voter->LastSend = DateTime::Zero;
            }
            voter->Instance = target;
            voter->IsDown = false;
            voter->IsAcked = false;
            voter->IsIntroduced = false;
        }
    }
    else if (addNew && target != site_.Instance)
    {
        voters_.push_back(VoterInfo(target));
    }

    if (leader_.Id == target.Id && leader_.InstanceId < target.InstanceId)
    {
        leader_ = site_.Instance;
    }

    Replica* replica = replicaSet_.GetReplica(target.Id);
    if (replica && replica->Instance.InstanceId < target.InstanceId)
    {
        replica->IsDown = true;
    }
}

void VoterStore::UpdateReplicaSet(ReplicaSetConfiguration const & config)
{
    bool foundUpReplica = false;
    for (NodeInstance const & replica : config.Replicas)
    {
        UpdateVoterInstance(replica, true);
        if (!IsVoterDown(replica))
        {
            foundUpReplica = true;
        }
    }

    if (config.Generation != replicaSet_.Generation)
    {
        if (replicaSet_.IsActive() == foundUpReplica)
        {
            if (config.Generation < replicaSet_.Generation)
            {
                return;
            }
        }
        else if (!foundUpReplica)
        {
            return;
        }
    }

    replicaSet_.Update(config);

    for (auto it = replicaSet_.Replicas.begin(); it != replicaSet_.Replicas.end(); ++it)
    {
        if (IsVoterDown(it->Instance))
        {
            it->IsDown = true;
        }
    }
}

void VoterStore::ResetVoterInfo()
{
    for (auto it = voters_.begin(); it != voters_.end(); ++it)
    {
        it->IsAcked = false;
        it->LastSend = DateTime::Zero;
    }
}

void VoterStore::GenerateLeaderInstance()
{
    leaderInstance_ = Stopwatch::Now().Ticks;
}

void VoterStore::GetConfiguration(ReplicaSetConfiguration & config) const
{
    AcquireReadLock grab(lock_);
    replicaSet_.GetConfiguration(config);
}

SerializedVoterStoreEntryUPtr VoterStore::GetEntry(wstring const & key) const
{
    AcquireReadLock grab(lock_);

    auto it = entries_.find(key);
    if (it != entries_.end() && it->second.Current)
    {
        return it->second.Current->Serialize();
    }

    return nullptr;
}

AsyncOperationSPtr VoterStore::BeginRead(
    wstring const & key,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<VoterStoreReadAsyncOperation>(site_, key, timeout, callback, parent);
}

ErrorCode VoterStore::EndRead(AsyncOperationSPtr const & operation, __out SerializedVoterStoreEntryUPtr & value, __out int64 & sequence)
{
    VoterStoreReadAsyncOperation* readOperation = AsyncOperation::End<VoterStoreReadAsyncOperation>(operation);
    value = readOperation->MoveResult();
    sequence = readOperation->GetSequence();

    return readOperation->Error;
}

AsyncOperationSPtr VoterStore::BeginWrite(
    wstring const & key,
    int64 checkSequence,
    SerializedVoterStoreEntryUPtr && value,
    bool isIdempotent,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<VoterStoreWriteAsyncOperation>(site_, key, checkSequence, move(value), isIdempotent, timeout, callback, parent);
}

ErrorCode VoterStore::EndWrite(AsyncOperationSPtr const & operation, __out SerializedVoterStoreEntryUPtr & value, __out int64 & sequence)
{
    VoterStoreWriteAsyncOperation* writeOperation = AsyncOperation::End<VoterStoreWriteAsyncOperation>(operation);
    value = writeOperation->MoveResult();
    sequence = writeOperation->GetSequence();

    return writeOperation->Error;
}

MessageUPtr VoterStore::CreateSyncRequestMessage(int64 sequence)
{
    SyncBody body;
    for (auto it = entries_.begin(); it != entries_.end(); ++it)
    {
        int64 entrySequence = (it->second.Pending ? it->second.PendingSequence : it->second.Sequence);
        if (entrySequence > sequence)
        {
            body.Entries.push_back(SyncEntry(it->first, entrySequence, it->second.Serialize()));
        }
    }

    body.StartSequence = sequence + 1;
    body.EndSequence = highestSequence_;
    replicaSet_.GetConfiguration(body.Config);

    return FederationMessage::GetVoterStoreSyncRequest().CreateMessage(body);
}

void VoterStore::SendMessage(MessageUPtr && request, NodeInstance const & target, bool isRequest)
{
    request->Idempotent = true;

    TimeSpan timeout = FederationConfig::GetConfig().MessageTimeout;
    if (isRequest)
    {
        site_.BeginRouteRequest(move(request), target.Id, target.InstanceId, true, timeout,
            [this, target](AsyncOperationSPtr const & operation)
        {
            this->SendCallback(operation, target, true);
        },
            site_.CreateAsyncOperationRoot());
    }
    else
    {
        site_.BeginRoute(move(request), target.Id, target.InstanceId, true, timeout,
            [this, target](AsyncOperationSPtr const & operation)
        {
            this->SendCallback(operation, target, false);
        },
            site_.CreateAsyncOperationRoot());
    }
}

void VoterStore::SendCallback(AsyncOperationSPtr const & operation, NodeInstance const & target, bool isRequest)
{
    MessageUPtr reply;
    ErrorCode error;

    if (isRequest)
    {
        error = site_.EndRouteRequest(operation, reply);
    }
    else
    {
        error = site_.EndRoute(operation);
    }

    if (error.IsError(ErrorCodeValue::RoutingNodeDoesNotMatchFault))
    {
        ReportDownVoter(target);
    }
    else if (reply)
    {
        NodeInstance targetInstance(target);
        if (target.InstanceId == 0)
        {
            PointToPointManager::GetFromInstance(*reply, targetInstance);
        }

        ProcessMessage(*reply, targetInstance);
    }
}

void VoterStore::ProcessMessageCommon(Message & message, NodeInstance const & target)
{
    UpdateVoterInstance(target, false);

    VoterStoreHeader header;
    if (message.Headers.TryReadFirst(header))
    {
        for (NodeInstance const & downVoter : header.DownVoters)
        {
            UpdateDownVoter(downVoter);
        }
    }
}

void VoterStore::ProcessOneWayMessage(Message & message, OneWayReceiverContext & oneWayReceiverContext)
{
    oneWayReceiverContext.Accept();

    if (phase_ != StorePhase::Invalid && phase_ != StorePhase::Uninitialized)
    {
        ProcessMessage(message, oneWayReceiverContext.FromInstance);
    }
}


void VoterStore::ProcessMessage(Message & message, NodeInstance const & target)
{
    AcquireWriteLock grab(lock_);

    ProcessMessageCommon(message, target);

    if (message.Action == FederationMessage::GetVoterStoreConfigQueryReply().Action)
    {
        ProcessConfigQueryReply(message);
    }
    else if (message.Action == FederationMessage::GetVoterStoreIntroduceReply().Action)
    {
        ProcessIntroduceReply(message, target);
    }
    else if (message.Action == FederationMessage::GetVoterStoreBootstrapReply().Action)
    {
        ProcessBootstrapReply(message, target);
    }
    else if (message.Action == FederationMessage::GetVoterStoreLeaderProbeReply().Action)
    {
        ProcessLeaderProbeReply(message, target);
    }
    else if (message.Action == FederationMessage::GetVoterStoreProgressReply().Action)
    {
        ProcessProgressReply(message, target);
    }
    else if (message.Action == FederationMessage::GetVoterStoreSyncReply().Action)
    {
        ProcessSyncReply(message, target);
    }
    else if (message.Action == FederationMessage::GetVoterStoreJoin().Action)
    {
        ProcessJoin(message, target);
    }
    else if (message.Action == FederationMessage::GetEmptyReply().Action)
    {
        site_.GetVoteManager().UpdateGlobalTickets(message, target);
    }

    RunStateMachine();
}

void VoterStore::ProcessRequestMessage(Message & request, RequestReceiverContextUPtr & requestReceiverContext)
{
    if (phase_ == StorePhase::Invalid || phase_ == StorePhase::Uninitialized)
    {
        requestReceiverContext->Ignore();
        return;
    }

    StateMachineActionCollection actions;

    AcquireWriteLock grab(lock_);

    ProcessMessageCommon(request, requestReceiverContext->FromInstance);
 
	MessageUPtr reply;
    if (request.Action == FederationMessage::GetVoterStoreConfigQueryRequest().Action)
    {
        reply = ProcessConfigQueryRequest();
    }
    else if (request.Action == FederationMessage::GetVoterStoreIntroduceRequest().Action)
    {
        reply = ProcessIntroduceRequest(request, requestReceiverContext->FromInstance);
    }
    else if (request.Action == FederationMessage::GetVoterStoreBootstrapRequest().Action)
    {
        reply = ProcessBootstrapRequest(request, requestReceiverContext->FromInstance);
    }
    else if (request.Action == FederationMessage::GetVoterStoreLeaderProbeRequest().Action)
    {
        reply = ProcessLeaderProbeRequest(request, requestReceiverContext->FromInstance);
    }
    else if (request.Action == FederationMessage::GetVoterStoreProgressRequest().Action)
    {
        reply = ProcessProgressRequest(request, requestReceiverContext->FromInstance, actions);
    }
    else if (request.Action == FederationMessage::GetVoterStoreSyncRequest().Action)
    {
        reply = ProcessSyncRequest(request, requestReceiverContext->FromInstance, actions);
    }
    else if (request.Action == FederationMessage::GetVoterStoreReadRequest().Action)
	{
		reply = ProcessReadRequest(request);
	}
	else if (request.Action == FederationMessage::GetVoterStoreWriteRequest().Action)
	{
        reply = ProcessWriteRequest(request, requestReceiverContext);
	}

    actions.Execute(site_);

    RunStateMachine();

	if (reply)
	{
        AddHeader(*reply);
		requestReceiverContext->Reply(move(reply));
	}
    else if (requestReceiverContext)
	{
		requestReceiverContext->Ignore();
	}
}

void VoterStore::AddHeader(Message & message)
{
    vector<NodeInstance> downReplicas;
    replicaSet_.GetDownReplicas(downReplicas);

    if (downReplicas.size() > 0)
    {
        message.Headers.Add(VoterStoreHeader(move(downReplicas)));
    }
}

MessageUPtr VoterStore::ProcessReadRequest(Message & request)
{
    VoterStoreReadReplyBody reply(replicaSet_.Generation);

    if (phase_ == StorePhase::BecomePrimary)
	{
        reply.Error = ErrorCodeValue::NotReady;
	}
    else if (phase_ != StorePhase::Primary)
    {
        reply.Error = ErrorCodeValue::NotPrimary;
    }
    else
    {
        VoterStoreReadRequestBody body;
        if (!request.GetBody(body))
        {
            reply.Error = request.FaultErrorCodeValue;
        }
        else
        {
            auto it = entries_.find(body.Key);
            if (it != entries_.end() && it->second.Current)
            {
                reply.Value = it->second.Current->Serialize();
                reply.Sequence = it->second.Sequence;
            }
            else
            {
                reply.Sequence = 0;
            }
        }
    }

	return FederationMessage::GetVoterStoreReadReply().CreateMessage(reply);
}

MessageUPtr VoterStore::ProcessWriteRequest(Message & request, RequestReceiverContextUPtr & requestReceiverContext)
{
	VoterStoreWriteReplyBody reply;

    if (phase_ == StorePhase::BecomePrimary)
    {
        reply.Error = ErrorCodeValue::NotReady;
    }
    else if (phase_ != StorePhase::Primary)
    {
        reply.Error = ErrorCodeValue::NotPrimary;
    }
    else
    {
        VoterStoreWriteRequestBody body;
        if (!request.GetBody(body))
        {
            reply.Error = request.FaultErrorCodeValue;
        }
        else
        {
            return ProcessWriteRequest(body.Key, body.CheckSequence, move(body.Value), requestReceiverContext);
        }
    }

	return FederationMessage::GetVoterStoreWriteReply().CreateMessage(reply);
}

MessageUPtr VoterStore::ProcessWriteRequest(wstring const & key, int64 checkSequence, SerializedVoterStoreEntryUPtr && entry, RequestReceiverContextUPtr & requestReceiverContext)
{
    VoterStoreWriteReplyBody reply;
    reply.Error = Write(key, highestSequence_ + 1, checkSequence, *entry);

    auto it = entries_.find(key);

    if (reply.Error.IsSuccess())
    {
        highestSequence_++;

        if (it->second.Pending)
        {
            it->second.ActiveRequest = move(requestReceiverContext);
            replicaSet_.ResetLastSend();
            return nullptr;
        }
    }
    else if (reply.Error.IsError(ErrorCodeValue::UpdatePending))
    {
        it->second.PendingRequests.push(PendingWriteRequest(checkSequence, move(entry), move(requestReceiverContext)));
        return nullptr;
    }

    if (it != entries_.end())
    {
        reply.Sequence = it->second.Sequence;
        reply.Value = it->second.Serialize();
    }

    return FederationMessage::GetVoterStoreWriteReply().CreateMessage(reply);
}

ErrorCode VoterStore::Write(wstring const & key, int64 sequence, int64 checkSequence, SerializedVoterStoreEntry const & value)
{
    ErrorCode error;

    if (phase_ == StorePhase::Primary && entries_.size() == 0 && replicaSet_.GetUpReplicaCount() < (voters_.size() + 1) / 2 + 1)
    {
        return ErrorCodeValue::NoWriteQuorum;
    }

    auto it = entries_.find(key);
    if (it != entries_.end())
    {
        if (it->second.Pending)
        {
            error = ErrorCodeValue::UpdatePending;
        }
        else if (checkSequence >= 0 && checkSequence != it->second.Sequence)
        {
            error = ErrorCodeValue::StoreWriteConflict;
        }
    }
    else
    {
        if (checkSequence > 0)
        {
            error = ErrorCodeValue::StoreWriteConflict;
        }
        else
        {
            it = entries_.insert(make_pair(key, StoreEntry())).first;
        }
    }

    if (error.IsSuccess())
    {
        if (phase_ == StorePhase::Primary && replicaSet_.HasSecondary())
        {
            if (it->second.Current)
            {
                it->second.Pending = it->second.Current->Serialize()->Deserialize();
                error = it->second.Pending->Update(value);
                if (!error.IsSuccess())
                {
                    it->second.Pending = nullptr;
                }
            }
            else
            {
                it->second.Pending = value.Deserialize();
            }
        }
        else
        {
            if (it->second.Current)
            {
                error = it->second.Current->Update(value);
            }
            else
            {
                it->second.Current = value.Deserialize();
            }
        }
    }

    if (error.IsSuccess())
    {
        VoteManager::WriteInfo(TraceStore, site_.IdString, "Write {0}={1} sequence {2}", key, it->second.Pending ? it->second.Pending : it->second.Current, sequence);
        if (it->second.Pending)
        {
            it->second.PendingSequence = sequence;
        }
        else
        {
            it->second.Sequence = sequence;
        }

        replicaSet_.Replicas.begin()->Sequence = sequence;
    }
    else
    {
        VoteManager::WriteInfo(TraceStore, site_.IdString, "Write {0}={1} sequence {2} check seq {3} result {4}", key, value, sequence, checkSequence, error);
        ASSERT_IF(phase_ != StorePhase::Primary, "Write rejected on secondary {0}", site_.IdString);
    }

    return error;
}

MessageUPtr VoterStore::ProcessConfigQueryRequest()
{
    ReplicaSetConfiguration config;
    replicaSet_.GetConfiguration(config);

    return FederationMessage::GetVoterStoreConfigQueryReply().CreateMessage(config);
}

void VoterStore::ProcessConfigQueryReply(Message & reply)
{
    ReplicaSetConfiguration config;
    if (reply.GetBody(config))
    {
        UpdateReplicaSet(config);
    }
}

MessageUPtr VoterStore::ProcessIntroduceRequest(Message &, NodeInstance const & target)
{
    bool isVoter = site_.GetVoteManager().IsSeedNode(target.Id, site_.RingName);
    if (!isVoter)
    {
        return ProcessConfigQueryRequest();
    }

    UpdateVoterInstance(target, true);

    IntroduceReplyBody body;
    body.LeaderInstance = (phase_ == StorePhase::Bootstrap ? leaderInstance_ : 0);
    replicaSet_.GetConfiguration(body.Config);
    GetVoters(body.Voters);

    return FederationMessage::GetVoterStoreIntroduceReply().CreateMessage(body);
}

void VoterStore::ProcessIntroduceReply(Message & request, NodeInstance const & target)
{
    IntroduceReplyBody body;
    if (phase_ == StorePhase::Introduce && request.GetBody(body))
    {
        VoterInfo* voter = GetVoterInfo(target);
        if (voter)
        {
            voter->IsAcked = true;

            for (NodeInstance const & instance : body.Voters)
            {
                if (instance.Id != site_.Id)
                {
                    UpdateVoterInstance(instance, true);
                }
            }

            UpdateReplicaSet(body.Config);

            if (body.LeaderInstance > 0 && !voter->IsDown)
            {
                if (leader_ != site_.Instance)
                {
                    VoterInfo* leader = GetVoterInfo(leader_.Id);
                    leader->IsAcked = false;
                }

                leader_ = target;
                leaderInstance_ = body.LeaderInstance;
            }
        }
    }
}

MessageUPtr VoterStore::ProcessBootstrapRequest(Message & request, NodeInstance const & target)
{
    BootstrapRequestBody body;
    if (!request.GetBody(body))
    {
        return nullptr;
    }

    BootstrapReplyBody reply;
    reply.LeaderInstance = body.LeaderInstance;
    replicaSet_.GetConfiguration(reply.Config);
    reply.Accepted = false;

    if (!IsActiveReplica() && phase_ != StorePhase::Bootstrap)
    {
        VoterInfo* voter = GetVoterInfo(target);
        if (voter && !voter->IsDown)
        {
            if (leader_ == target)
            {
                leaderInstance_ = max(leaderInstance_, body.LeaderInstance);
                reply.Accepted = true;
            }
            else if (leader_ == site_.Instance)
            {
                leader_ = target;
                leaderInstance_ = body.LeaderInstance;
                reply.Accepted = true;
            }
        }
    }

    return FederationMessage::GetVoterStoreBootstrapReply().CreateMessage(reply);
}

void VoterStore::ProcessBootstrapReply(Message & reply, NodeInstance const & target)
{
    BootstrapReplyBody body;
    if (phase_ == StorePhase::Bootstrap && reply.GetBody(body) && body.LeaderInstance == leaderInstance_)
    {
        UpdateReplicaSet(body.Config);

        if (body.Accepted)
        {
            VoterInfo* voter = GetVoterInfo(target);
            if (voter)
            {
                voter->IsAcked = true;
            }
        }
        else
        {
            ResetVoterInfo();
            phase_ = StorePhase::BecomeSecondary;
        }
    }
}

MessageUPtr VoterStore::ProcessLeaderProbeRequest(Message & request, NodeInstance const &)
{
    LeaderProbeRequestBody body;
    if (!request.GetBody(body))
    {
        return nullptr;
    }

    LeaderProbeReplyBody reply;
    reply.LeaderInstance = body.LeaderInstance;
    reply.IsLeader = (phase_ == StorePhase::Bootstrap && body.LeaderInstance == leaderInstance_);

    return FederationMessage::GetVoterStoreLeaderProbeReply().CreateMessage(reply);
}

void VoterStore::ProcessLeaderProbeReply(Message & reply, NodeInstance const & target)
{
    LeaderProbeReplyBody body;
    if (reply.GetBody(body) && leader_ == target)
    {
        if (body.LeaderInstance == leaderInstance_ && !body.IsLeader &&
            (phase_ == StorePhase::BecomeSecondary || phase_ == StorePhase::Introduce))
        {
            leader_ = site_.Instance;
            leaderInstance_ = 0;
        }
    }
}

void VoterStore::ProcessJoin(Message &, NodeInstance const & target)
{
    VoterInfo* voter = GetVoterInfo(target);
    if (voter)
    {
        voter->IsIntroduced = true;
    }
}

MessageUPtr VoterStore::ProcessProgressRequest(Message & request, NodeInstance const & target, StateMachineActionCollection & actions)
{
    ProgressRequestBody body;
    if (!request.GetBody(body))
    {
        return nullptr;
    }

    if (phase_ == StorePhase::Secondary)
    {
        Replica* leader = replicaSet_.GetLeader();
        if (!leader || leader->Instance != target)
        {
            return nullptr;
        }
    }
    else if (phase_ == StorePhase::BecomeSecondary)
    {
        if (leader_ != site_.Instance)
        {
            actions.Add(make_unique<SendVoterStoreMessageAction>(FederationMessage::GetVoterStoreLeaderProbeRequest().CreateMessage(LeaderProbeRequestBody(leaderInstance_)), *this, leader_, true));
            return nullptr;
        }
    }
    else
    {
        return nullptr;
    }

    leader_ = target;
    leaderInstance_ = body.LeaderInstance;

    ProgressReplyBody reply;
    reply.EndSequence = highestSequence_;

    if (body.Sequence < highestSequence_)
    {
        for (auto it = entries_.begin(); it != entries_.end(); ++it)
        {
            if (it->second.Sequence > body.Sequence)
            {
                reply.Entries.push_back(SyncEntry(it->first, it->second.Sequence, it->second.Serialize()));
            }
        }
    }

    return FederationMessage::GetVoterStoreProgressReply().CreateMessage(reply);
}

void VoterStore::ProcessProgressReply(Message & reply, NodeInstance const & target)
{
    ProgressReplyBody body;
    if (reply.GetBody(body) && phase_ == StorePhase::BecomePrimary)
    {
        Replica* replica = replicaSet_.GetReplica(target);
        if (replica && replica->Sequence < body.EndSequence)
        {
            replica->Sequence = body.EndSequence;

            if (highestSequence_ < body.EndSequence)
            {
                for (auto it = body.Entries.begin(); it != body.Entries.end(); ++it)
                {
                    Write(it->Key, it->Sequence, -1, *(it->Value));
                }

                highestSequence_ = body.EndSequence;
            }
        }
    }
}

MessageUPtr VoterStore::ProcessSyncRequest(Message & request, NodeInstance const & target, StateMachineActionCollection & actions)
{
    SyncBody body;
    if (!request.GetBody(body))
    {
        return nullptr;
    }

    SyncReplyBody reply;

    if (phase_ == StorePhase::Secondary)
    {
        if (target != leader_)
        {
            return nullptr;
        }
    }
    else if (phase_ == StorePhase::BecomeSecondary)
    {
        if (leader_ != site_.Instance && leader_ != target)
        {
            actions.Add(make_unique<SendVoterStoreMessageAction>(FederationMessage::GetVoterStoreLeaderProbeRequest().CreateMessage(LeaderProbeRequestBody(leaderInstance_)), *this, leader_, true));
            return nullptr;
        }
    }
    else if (phase_ != StorePhase::Bootstrap)
    {
        return nullptr;
    }

    UpdateReplicaSet(body.Config);

    ASSERT_IF(replicaSet_.GetReplica(site_.Instance) == nullptr,
        "Replica {0} not in configuration {1}",
        site_.Id, *this);

    ASSERT_IF(body.StartSequence > highestSequence_ + 1,
        "Received sync {0}-{1}: {2}",
        body.StartSequence, body.EndSequence, *this);

    if (highestSequence_ < body.EndSequence)
    {
        for (auto it = body.Entries.begin(); it != body.Entries.end(); ++it)
        {
            if (it->Sequence > highestSequence_)
            {
                Write(it->Key, it->Sequence, -1, *(it->Value));
            }
        }

        highestSequence_ = body.EndSequence;
    }

    phase_ = StorePhase::Secondary;
    leader_ = target;
    leaderInstance_ = 0;

    reply.Sequence = highestSequence_;

    return FederationMessage::GetVoterStoreSyncReply().CreateMessage(reply);
}

void VoterStore::ProcessSyncReply(Message & reply, NodeInstance const & target)
{
    SyncReplyBody body;
    if (reply.GetBody(body))
    {
        Replica* replica = replicaSet_.GetReplica(target);
        if (replica && replica->Sequence < body.Sequence)
        {
            replica->Sequence = body.Sequence;
        }
    }
}

void VoterStore::RunStateMachine()
{
    StateMachineActionCollection actions;

    if (phase_ == StorePhase::Primary)
    {
        CheckStablePrimary(actions);
    }
    else if (phase_ == StorePhase::BecomePrimary)
    {
        CheckPrimaryPromotion(actions);
    }
    else if (phase_ == StorePhase::Secondary)
    {
        CheckReadySecondary(actions);
    }
    else if (phase_ == StorePhase::BecomeSecondary)
    {
        CheckSecondaryPromotion(actions);
    }
    else if (phase_ == StorePhase::Bootstrap)
    {
        CheckBootstrap(actions);
    }
    else if (phase_ == StorePhase::Introduce)
    {
        CheckIntroduce(actions);
    }
    else if (phase_ == StorePhase::None)
    {
        CheckNonVoter(actions);
    }

    if (pendingRequests_.size() > 0)
    {
        Replica* primary = replicaSet_.GetPrimary();
        if (primary)
        {
            actions.Add(make_unique<SendRequestsAfterResolveAction>(move(pendingRequests_), primary->Instance));
        }
    }

    for (auto it = voters_.begin(); it != voters_.end();)
    {
        if (it->IsStale && it->IsDown && !replicaSet_.GetReplica(it->Instance))
        {
            it = voters_.erase(it);
        }
        else
        {
            ++it;
        }
    }

    wstring stateString = wformatString(*this);
    if (!actions.IsEmpty() || lastTracedState_ != stateString || lastTraceTime_ + TimeSpan::FromMinutes(10) < DateTime::Now())
    {
        VoteManager::WriteInfo(TraceStore, site_.IdString, "{0}\r\nActions: {1}", stateString, actions);
        lastTracedState_ = stateString;
        lastTraceTime_ = DateTime::Now();
    }

    actions.Execute(site_);

    if (timer_)
    {
        timer_->Change(FederationConfig::GetConfig().VoterStoreRetryInterval);
    }
}

void VoterStore::CheckStablePrimary(StateMachineActionCollection & actions)
{
    int64 committed = replicaSet_.GetCommitedSequence(highestSequence_);
    for (auto it = entries_.begin(); it != entries_.end(); ++it)
    {
        StoreEntry & entry = it->second;
        if (entry.Pending && entry.PendingSequence <= committed)
        {
            entry.Current = move(entry.Pending);
            entry.Sequence = entry.PendingSequence;

            VoterStoreWriteReplyBody body;
            body.Sequence = entry.Sequence;
            body.Value = entry.Current->Serialize();
            actions.Add(make_unique<ReplyAction>(move(entry.ActiveRequest), FederationMessage::GetVoterStoreWriteReply().CreateMessage(body)));
        }

        while (!entry.Pending && entry.PendingRequests.size() > 0)
        {
            PendingWriteRequest request = move(entry.PendingRequests.front());
            entry.PendingRequests.pop();

            MessageUPtr reply = ProcessWriteRequest(it->first, request.CheckSequence, move(request.Value), request.Context);
            if (reply)
            {
                actions.Add(make_unique<ReplyAction>(move(request.Context), move(reply)));
            }
        }
    }

    bool found = false;
    for (auto it = replicaSet_.Replicas.begin(); it != replicaSet_.Replicas.end(); ++it)
    {
        if (!it->IsDown && it->Sequence <= 0)
        {
            found = true;
        }
    }

    if (found)
    {
        if (configurationSequence_ == 0)
        {
            configurationSequence_ = highestSequence_;
        }
    }
    else
    {
        if (configurationSequence_ > 0)
        {
            configurationSequence_ = 0;
        }
    }

    if (configurationSequence_ == 0)
    {
        bool addReplica = false;
        for (auto it = voters_.begin(); it != voters_.end(); ++it)
        {
            if (!it->IsDown && it->IsIntroduced && replicaSet_.Add(it->Instance))
            {
                addReplica = true;
            }
        }

        if (addReplica)
        {
            highestSequence_++;
            configurationSequence_ = highestSequence_;
            replicaSet_.Replicas.begin()->Sequence = highestSequence_;

            replicaSet_.RemoveDownReplicas();
            replicaSet_.Epoch++;
        }
    }

    bool syncNewReplica = (configurationSequence_ > 0);
    for (auto it = replicaSet_.Replicas.begin(); it != replicaSet_.Replicas.end(); ++it)
    {
        if (!it->IsDown && it->Sequence > 0 && it->Sequence < highestSequence_)
        {
            if (it->Sequence < configurationSequence_)
            {
                syncNewReplica = false;
            }

            it->AddSendMessageAction(actions, CreateSyncRequestMessage(it->Sequence), *this, true);
        }
    }

    if (syncNewReplica)
    {
        for (auto it = replicaSet_.Replicas.begin(); it != replicaSet_.Replicas.end(); ++it)
        {
            if (!it->IsDown && it->Sequence <= 0)
            {
                it->AddSendMessageAction(actions, CreateSyncRequestMessage(it->Sequence), *this, true);
                break;
            }
        }
    }
}

void VoterStore::CheckPrimaryPromotion(StateMachineActionCollection & actions)
{
    ProgressRequestBody body;
    body.LeaderInstance = leaderInstance_;
    body.Sequence = highestSequence_;

    bool completed = true;

    size_t localIndex = 0;
    for (size_t i = 0; i < replicaSet_.Replicas.size(); i++)
    {
        Replica & replica = replicaSet_.Replicas[i];
        if (replica.Instance == site_.Instance)
        {
            localIndex = i;
        }
        else if (!replica.IsDown && replica.Sequence < 0)
        {
            replica.AddSendMessageAction(actions, FederationMessage::GetVoterStoreProgressRequest().CreateMessage(body), *this, true);
            completed = false;
        }
    }

    ASSERT_IF(localIndex == replicaSet_.Replicas.size(), 
        "Configuration {0} does not include local secondary: {1}",
        *this, site_.Id);

    if (completed)
    {
        BecomePrimary();

        replicaSet_.Epoch += (localIndex * 0x100000000 + 1);
        replicaSet_.RemoveDownReplicas();
        highestSequence_++;
        replicaSet_.Replicas[0].Sequence = highestSequence_;

        CheckStablePrimary(actions);
    }
}

void VoterStore::CheckReadySecondary(StateMachineActionCollection & actions)
{
    auto previousUpReplica = replicaSet_.Replicas.end();
    for (auto it = replicaSet_.Replicas.begin(); it != replicaSet_.Replicas.end() && it->Instance != site_.Instance; ++it)
    {
        if (!it->IsDown)
        {
            if (site_.Table.IsDown(it->Instance))
            {
                UpdateDownVoter(it->Instance);
            }
            else
            {
                previousUpReplica = it;
            }
        }
    }

    if (previousUpReplica != replicaSet_.Replicas.end())
    {
        previousUpReplica->AddSendMessageAction(actions, nullptr, *this, true);
        return;
    }

    phase_ = StorePhase::BecomePrimary;

    GenerateLeaderInstance();

    CheckPrimaryPromotion(actions);
}

void VoterStore::BecomePrimary()
{
    phase_ = StorePhase::Primary;

    auto site = site_.CreateComponentRoot();
    Threadpool::Post([this, site]
    {
        this->site_.Table.BecomeLeader();
    }
    );
}

void VoterStore::Bootstrap()
{
    BecomePrimary();

    int64 oldGeneration = replicaSet_.Generation;
    replicaSet_.Clear();
    replicaSet_.Add(site_.Instance);
    replicaSet_.Generation = max(leaderInstance_, oldGeneration + 1);
    replicaSet_.Epoch = 1;
    highestSequence_ = 0;
}

void VoterStore::CheckSecondaryPromotion(StateMachineActionCollection & actions)
{
    Voter* leader = replicaSet_.GetLeader();
    if (leader)
    {
        if (leader->Instance != site_.Instance)
        {
            leader->AddSendMessageAction(actions, FederationMessage::GetVoterStoreJoin().CreateMessage(), *this, false);
        }
        else
        {
            if (bootstrapStartTime_ == StopwatchTime::MaxValue)
            {
                bootstrapStartTime_ = Stopwatch::Now() + FederationConfig::GetConfig().VoterStoreBootstrapWaitInterval;
            }

            if (!site_.Table.HasGlobalTime() && bootstrapStartTime_ < Stopwatch::Now())
            {
                GenerateLeaderInstance();
                Bootstrap();

                CheckStablePrimary(actions);
            }
        }
    }
    else
    {
        if (bootstrapStartTime_ == StopwatchTime::MaxValue)
        {
            bootstrapStartTime_ = Stopwatch::Now() + FederationConfig::GetConfig().VoterStoreBootstrapWaitInterval;
        }

        for (auto it = voters_.begin(); it != voters_.end(); ++it)
        {
            if (!it->IsDown)
            {
                if ((it->Instance == leader_) || 
                    (it->Instance.Id < site_.Id && (!leader || it->Instance.Id < leader->Instance.Id)))
                {
                    leader = &(*it);
                }
            }
        }

        if (leader)
        {
            leader->AddSendMessageAction(actions, FederationMessage::GetVoterStoreConfigQueryRequest().CreateMessage(), *this, true);
        }
        else if (!site_.Table.HasGlobalTime() && bootstrapStartTime_ < Stopwatch::Now())
        {
            phase_ = StorePhase::Bootstrap;
            GenerateLeaderInstance();

            ResetVoterInfo();

            CheckBootstrap(actions);
        }
    }
}

void VoterStore::CheckBootstrap(StateMachineActionCollection & actions)
{
    bool completed = true;
    BootstrapRequestBody body;
    body.LeaderInstance = leaderInstance_;

    for (auto it = voters_.begin(); it != voters_.end(); ++it)
    {
        if (!it->IsDown && !it->IsAcked)
        {
            it->AddSendMessageAction(actions, FederationMessage::GetVoterStoreBootstrapRequest().CreateMessage(body), *this, true);
            completed = false;
        }
    }

    if (completed)
    {
        Bootstrap();

        CheckStablePrimary(actions);
    }
}

void VoterStore::CheckIntroduce(StateMachineActionCollection & actions)
{
    size_t introducedCount = 0;
    bool completed = true;

    for (auto it = voters_.begin(); it != voters_.end(); ++it)
    {
        if (it->IsAcked)
        {
            introducedCount++;
        }
        else if (!it->IsDown)
        {
            it->AddSendMessageAction(actions, FederationMessage::GetVoterStoreIntroduceRequest().CreateMessage(), *this, true);
            completed = false;
        }
    }

    if (completed && (introducedCount >= (voters_.size() + 1) / 2 || site_.Table.HasGlobalTime()))
    {
        phase_ = StorePhase::BecomeSecondary;
        CheckSecondaryPromotion(actions);
    }
}

void VoterStore::CheckNonVoter(StateMachineActionCollection & actions)
{
    if (pendingRequests_.size() > 0 && !replicaSet_.GetPrimary())
    {
        Voter* leader = replicaSet_.GetLeader();
        if (!leader)
        {
            for (auto it = voters_.begin(); it != voters_.end(); ++it)
            {
                if (!leader || it->LastSend < leader->LastSend)
                {
                    leader = &(*it);
                }
            }
        }

        if (leader)
        {
            leader->AddSendMessageAction(actions, FederationMessage::GetVoterStoreConfigQueryRequest().CreateMessage(), *this, true);
        }
    }
}

void VoterStore::WriteTo(TextWriter& w, FormatOptions const& option) const
{
    if (phase_ == StorePhase::Invalid)
    {
        return;
    }

    if (option.formatString != "e")
    {
        char phase;
        switch (phase_)
        {
        case StorePhase::Uninitialized:
            phase = 'U';
            break;
        case StorePhase::Introduce:
            phase = 'I';
            break;
        case StorePhase::Bootstrap:
            phase = 'B';
            break;
        case StorePhase::BecomeSecondary:
            phase = 's';
            break;
        case StorePhase::Secondary:
            phase = 'S';
            break;
        case StorePhase::BecomePrimary:
            phase = 'p';
            break;
        case StorePhase::Primary:
            phase = 'P';
            break;
        default:
            phase = 'N';
            break;
        }

        w.Write("{0} {1}:{2:x} seq={3}/{4}, leader={5}:{6}",
            phase,
            replicaSet_.Generation,
            replicaSet_.Epoch,
            highestSequence_,
            configurationSequence_,
            leader_,
            leaderInstance_);

        if ((bootstrapStartTime_ != StopwatchTime::MaxValue) && 
            (phase_ == StorePhase::BecomeSecondary || phase_ == StorePhase::Introduce))
        {
            w.Write(" BootstrapTime {0}", bootstrapStartTime_);
        }

        if (pendingRequests_.size() > 0)
        {
            w.Write(" pending requests={0}", pendingRequests_.size());
        }

        w.WriteLine();

        for (Replica const & replica : replicaSet_.Replicas)
        {
            w.Write("{0}{1} {2} {3}\r\n", replica.Instance, replica.IsDown ? " D" : "", replica.Sequence, replica.LastSend);
        }
        w.WriteLine();

        for (VoterInfo const & voter : voters_)
        {
            w.Write("{0}{1}{2} {3} {4}\r\n", voter.Instance, (voter.IsDown ? " D" : voter.IsIntroduced ? " I" : ""), voter.IsStale ? "S" : "", voter.IsAcked, voter.LastSend);
        }
        w.WriteLine();
    }

    for (auto it = entries_.begin(); it != entries_.end(); ++it)
	  {
        if (it != entries_.begin())
        {
            w.Write(",");
        }

        w.Write("{0}: {1}({2})", it->first, it->second.Current, it->second.Sequence);
        if (it->second.Pending)
        {
            w.Write(" P");
        }
        if (it->second.PendingRequests.size() > 0)
        {
            w.Write(" {0}", it->second.PendingRequests.size());
        }
	  }
}
