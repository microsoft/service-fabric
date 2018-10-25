// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Transport;
using namespace Common;
using namespace std;

namespace
{
    StringLiteral const TraceType("SendTarget");
    const Global<wstring> ScheduleCloseAction = make_global<wstring>(L"ScheduleClose");

    wstring CreateTraceId(TcpSendTarget const* target, wstring const & targetId, wstring const & ownerId)
    {
        if (!targetId.empty() && !ownerId.empty())
            return wformatString("{0}-{1}-{2}", TextTracePtrAs(target, ISendTarget), targetId, ownerId); 

        if (!targetId.empty())
            return wformatString("{0}-{1}", TextTracePtrAs(target, ISendTarget), targetId);

        if (!ownerId.empty()) 
            return wformatString("{0}-{1}", TextTracePtrAs(target, ISendTarget), ownerId);

        return wformatString("{0}", TextTracePtrAs(target, ISendTarget));
    }
}

atomic_uint64  TcpSendTarget::objCount_(0);

TcpSendTarget::TcpSendTarget(
    TcpDatagramTransport & owner,
    IDatagramTransport::MessageHandlerSPtr const& msgHandler,
    std::wstring const & address,
    std::wstring const & targetId,
    std::wstring const & sspiTarget,
    uint64 instance,
    bool anonymous,
    TransportSecuritySPtr const & security)
    : owner_(owner)
    , msgHandler_(msgHandler)
    , ownerWPtr_(TcpDatagramTransportWPtr(owner.shared_from_this()))
    , address_(address)
    , localAddress_(owner.ListenAddress())
    , id_((targetId.empty() ? address : targetId))
    , hasId_(!targetId.empty())
    , instance_(instance)
    , anonymous_(anonymous)
    , traceId_(CreateTraceId(this, targetId, owner.IdString))
    , connectionIdleTimeout_(owner.ConnectionIdleTimeout())
    , security_(security)
    , flags_()
{
    ++objCount_;

    if (security_->UsingWindowsCredential())
    {
        sspiTarget_ = sspiTarget;
        sspiTargetToBeSet_ = sspiTarget_.empty();
    }

    if (msgHandler)
    {
        Invariant(*msgHandler);
    }
    else
    {
        TcpDatagramTransport::WriteInfo(TraceType, traceId_, "empty mesage handler at construction");
    }
}

TcpSendTarget::~TcpSendTarget()
{
    // This will attempt to remove the weak_ptr entry so we don't leak
    auto owner = this->ownerWPtr_.lock();
    if (owner)
    {
        owner->RemoveSendTargetEntry(*this);
    }

    --objCount_;
}

void TcpSendTarget::SetMessageHandler(IDatagramTransport::MessageHandlerSPtr const & msgHandler)
{
    TcpDatagramTransport::WriteInfo(TraceType, traceId_, "SetMessageHandler");
    Invariant(msgHandler);
    Invariant(*msgHandler);

    AcquireWriteLock grab(lock_);
    msgHandler_ = msgHandler;
}

void TcpSendTarget::UpdateId(std::wstring const & id)
{
    id_ = id;
    hasId_ = true;
    traceId_ = CreateTraceId(this, id_, owner_.IdString);
}

Common::ErrorCode TcpSendTarget::SendOneWay(MessageUPtr && message, TimeSpan expiration, TransportPriority::Enum priority)
{
    if (test_blocked_) return ErrorCode();

    Invariant(message);
    MessageUPtr messageLocal(move(message)); // Move message to local for isolation

    IConnectionSPtr connection;
    ErrorCode error =  GetConnectionForSend(connection, priority);
    if (!error.IsSuccess())
    {
        trace.CannotSend(traceId_, localAddress_, address_, error, messageLocal->TraceId(), messageLocal->Actor, messageLocal->Action);
        messageLocal->OnSendStatus(error, move(messageLocal));
        return  error;
    }

    return connection->SendOneWay(std::move(messageLocal), expiration);
}

void TcpSendTarget::TargetDown(uint64 instance)
{
    AcquireWriteLock grab(lock_);

    trace.TargetDown(traceId_, localAddress_, address_, instance_, isDown_, instance);

    if (instance < instance_)
    {
        return;
    }

    isDown_ = true;
    ValidateConnectionsChl(instance, Stopwatch::Now());
}

void TcpSendTarget::Abort()
{
    trace.TargetClose(traceId_, localAddress_, address_, true);

    vector<IConnectionSPtr> activeConnections;
    vector<IConnectionSPtr> inactiveConnections;
    {
        AcquireWriteLock grab(lock_);

        destructing_ = true;

        if (!connections_.empty())
        {
            activeConnections = move(connections_);
            TraceConnectionsChl();
        }

        if (newConnection_)
        {
            activeConnections.emplace_back(move(newConnection_));
        }

        inactiveConnections = move(scheduledClose_);
    }

    CloseConnections(activeConnections, true);
    CloseConnections(inactiveConnections, true);
}

void TcpSendTarget::Close()
{
    trace.TargetClose(traceId_, localAddress_, address_, false);
    this->CloseConnections(true);
}

void TcpSendTarget::Reset()
{
    trace.TargetReset(traceId_, localAddress_, address_);
    this->CloseConnections(false);
}

void TcpSendTarget::CloseConnections(bool destructing)
{
    vector<IConnectionSPtr> activeConnections;
    vector<IConnectionSPtr> inactiveConnections;
    {
        AcquireWriteLock grab(lock_);

        if (destructing)
        {
            destructing_ = true;
        }

        if (!connections_.empty())
        {
            activeConnections = move(connections_);
            TraceConnectionsChl();
        }

        if (newConnection_)
        {
            activeConnections.emplace_back(move(newConnection_));
        }

        inactiveConnections = move(scheduledClose_);
    }

    CloseConnections(activeConnections, false);
    CloseConnections(inactiveConnections, false);
}

void TcpSendTarget::CloseConnections(vector<IConnectionSPtr> const & connections, bool abortConnection)
{
    for (auto const & connectionSPtr : connections)
    {
        trace.CloseConnection(traceId_, localAddress_, address_, connectionSPtr->TraceId(), abortConnection);
        if (abortConnection)
        {
            connectionSPtr->Abort(ErrorCodeValue::OperationCanceled);
        }
        else
        {
            connectionSPtr->Close();
        }
    }
}

void TcpSendTarget::StartConnectionRefresh(IConnection* expiringConnection)
{
    Invariant(!expiringConnection->IsInbound());

    auto transport = ownerWPtr_.lock();
    if (!transport) return;

    if (!transport->Security()->Settings().ReadyNewSessionBeforeExpiration())
    {
        TcpDatagramTransport::WriteInfo(TraceType, traceId_, "skipping connection refresh per settings");
        RetireExpiredConnection(*expiringConnection);
        return;
    }

    TcpDatagramTransport::WriteInfo(TraceType, traceId_, "StartConnectionRefresh: replacing connection {0}", TextTracePtr(expiringConnection));
    Invariant(expiringConnection);

    TcpConnectionSPtr newConnection;
    {
        AcquireWriteLock grab(lock_);

        if (destructing_)
        {
            TcpDatagramTransport::WriteInfo(TraceType, traceId_, "StartConnectionRefresh: destructing");
            return;
        }

        auto match = std::find_if(
            connections_.cbegin(),
            connections_.cend(),
            [expiringConnection](IConnectionSPtr const & connection) { return connection.get() == expiringConnection;  });

        if (match == connections_.cend())
        {
            TcpDatagramTransport::WriteInfo(TraceType, traceId_, "StartConnectionRefresh: {0} is already removed", TextTracePtr(expiringConnection));
            return;
        }

        if (newConnection_)
        {
            TcpDatagramTransport::WriteInfo(TraceType, traceId_, "StartConnectionRefresh: already refreshing with {0}", TextTracePtrAs(newConnection_.get(), IConnection));
            return;
        }

        expiringConnection_ = expiringConnection;

        auto error = TcpConnection::Create(
            transport,
            shared_from_this(),
            nullptr,
            msgHandler_,
            security_,
            sspiTarget_,
            expiringConnection->GetPriority(),
            flags_,
            SecurityConfig::GetConfig().SessionRefreshTimeout,
            newConnection);

        if (!error.IsSuccess()) return;

        newConnection->SetReadyCallback(
            [thisWPtr = (TcpSendTargetWPtr)shared_from_this()] (IConnection* connection)
            {
                if (auto thisSPtr = thisWPtr.lock())
                {
                    thisSPtr->ConnectionRefreshCompleted(connection);
                }
            });

        newConnection_ = newConnection;
    }

    newConnection->Open();
}

void TcpSendTarget::ConnectionRefreshCompleted(IConnection* connection)
{
    TcpDatagramTransport::WriteInfo(TraceType, traceId_, "ConnectionRefreshCompleted: {0}", TextTracePtr(connection));
    IConnectionSPtr expiringConnection;
    {
        AcquireWriteLock grab(lock_);

        if (destructing_) return;

        if (connection != newConnection_.get())
        {
            if (newConnection_)
            {
                TcpDatagramTransport::WriteWarning(
                    TraceType, traceId_,
                    "ConnectionRefreshCompleted: racing refresh detected: ready connection = {0}, newConnection_ = {1}",
                    TextTracePtr(connection), TextTracePtr(newConnection_.get()));

                connection->Abort(ErrorCodeValue::OperationCanceled);
            }
            return;
        }

        for (auto & entry : connections_)
        {
            if (entry.get() == expiringConnection_)
            {
                TcpDatagramTransport::WriteInfo(
                    TraceType, traceId_,
                    "ConnectionRefreshCompleted: replacing {0} with {1}",
                    TextTracePtr(entry.get()), TextTracePtr(newConnection_.get()));

                expiringConnection = move(entry);
                entry = move(newConnection_);
                break;
            }
        }

        if (!expiringConnection)
        {
            // add to the front of connection list to avoid idle timeout on the new connection
            connections_.emplace(connections_.begin(), move(newConnection_));
        }

        TraceConnectionsChl();
    }

    if (expiringConnection)
    {
        trace.SecureSessionExpired(expiringConnection->TraceId());

        // Reporting fault here will not cause send failure as the connection is already removed
        expiringConnection->ReportFault(SESSION_EXPIRATION_FAULT);
        SendScheduleCloseMessage(*expiringConnection, SecurityConfig::GetConfig().SessionExpirationCloseDelay);
    }
}

void TcpSendTarget::RetireExpiredConnection(IConnection & connection)
{
    trace.SecureSessionExpired(connection.TraceId());
    connection.StopSendAndScheduleClose(
        SESSION_EXPIRATION_FAULT,
        true,
        //todo, leikong, consider using shorter delay on server side, as server side has already waited after sending Reconnect message in TcpConnection::OnSessionExpired
        SecurityConfig::GetConfig().SessionExpirationCloseDelay);
}

void TcpSendTarget::Start()
{
}

std::wstring const & TcpSendTarget::LocalAddress() const
{
    return this->localAddress_;
}

TcpConnectionSPtr TcpSendTarget::AddAcceptedConnection(Socket & socket)
{
    TcpConnectionSPtr connection = nullptr;
    {
        AcquireWriteLock grab(lock_);

        if (destructing_)
        {
            return connection;
        }

        ErrorCode error = AddConnection_CallerHoldingLock(&socket, TransportPriority::Normal, connection);
        if (!error.IsSuccess())
        {
            return connection;
        }
    }

    // Start outside of lock scope
    if (!connection->Open())
    {
        TcpDatagramTransport::WriteWarning(
            TraceType, traceId_,
            "{0}-{1}: Unable to start accepted connection {2}",
            localAddress_,
            address_,
            connection->TraceId());
    }

    return connection;
}

void TcpSendTarget::AcquireConnection(
    TcpSendTarget & src,
    IConnection & connection,
    ListenInstance const & remoteListenInstance)
{
    ISendTarget::SPtr oldTarget;
    vector<IConnectionSPtr> connectionsInvalidated;
    {
        AcquireWriteLock grab(lock_);
        AcquireWriteLock grabFrom(src.lock_);

        if (destructing_ || src.destructing_) return;

        if (src.connections_.size() > 1)
        {
            TcpDatagramTransport::WriteWarning(
                TraceType, src.TraceId(),
                "{0}-{1}: anonymous target should not have more than one connections: {2}",
                src.localAddress_, src.address_, src.connections_.size());
        }

        IConnectionSPtr connectionSPtr;
        for (auto iter = src.connections_.begin(); iter != src.connections_.end(); ++ iter)
        {
            if ((*iter).get() == (&connection))
            {
                connectionSPtr = move(*iter);
                src.connections_.erase(iter);
                break;
            }
        }

        if (!connectionSPtr)
        {
            TcpDatagramTransport::WriteInfo(
                TraceType, src.TraceId(),
                "{0}-{1}: cannot find connection {2} to move, can happen during aborting",
                src.localAddress_, src.address_, connection.TraceId());

            return;
        }

        auto self = this->shared_from_this();
        oldTarget = connectionSPtr->SetTarget(self); // move to avoid destruction under lock

        connections_.push_back(move(connectionSPtr));
        connection.UpdateInstance(remoteListenInstance);
        TraceConnectionsChl();

        ValidateConnectionsChl(instance_, Stopwatch::Now());
    }
}

void TcpSendTarget::TraceConnectionsChl()
{
    trace.TargetConnectionsChanged(
        traceId_,
        localAddress_,
        address_,
        anonymous_,
        instance_,
        connections_.size(),
        ConnectionStatesToStringChl());
}

size_t TcpSendTarget::ConnectionCount() const
{
    AcquireReadLock grab(lock_);
    return connections_.size();
}

bool TcpSendTarget::TryEnableDuplicateDetection()
{
    AcquireWriteLock grab(lock_);

    *flags_ |= TransportFlags::DuplicateDetection;

    for (auto const & connection : connections_)
    {
        *(connection->GetTransportFlags()) |= TransportFlags::DuplicateDetection;
    }

    return true;
}

TransportFlags TcpSendTarget::GetTransportFlags() const
{
    AcquireReadLock grab(lock_);

    return flags_;
}

// When a connection dies itself, it can report its failure and be removed from the list
void TcpSendTarget::OnConnectionDisposed(IConnectionSPtr const & connection)
{
    if (RemoveConnection(*connection, false))
    {
        return;
    }

    // This must be a scheduled close
    if (!PruneScheduledCloseList(connection))
    {
        TcpDatagramTransport::WriteNoise(
            TraceType, traceId_,
            "{0}-{1}: OnConnectionDisposed: connection {2} not found in connections_ or scheduledClose_",
            localAddress_, address_, connection->TraceId());
    }
}

IConnectionSPtr TcpSendTarget::PruneScheduledCloseList(IConnectionSPtr const& connection)
{
    IConnectionSPtr toRemove;
    AcquireWriteLock grab(lock_);

    for (auto iter = scheduledClose_.begin(); iter != scheduledClose_.end(); ++iter)
    {
        if (*iter == connection)
        {
            toRemove = move(*iter);
            TcpDatagramTransport::WriteInfo(
                TraceType, traceId_,
                "{0}-{1}: removing connection {2} from scheduledClose_",
                localAddress_, address_, connection->TraceId());

            scheduledClose_.erase(iter);
            break;
        }
    }

    return toRemove;
}

void TcpSendTarget::OnConnectionFault(ErrorCode fault)
{
    if (auto owner = ownerWPtr_.lock())
    {
        owner->OnConnectionFault(*this, fault);
    }
}

void TcpSendTarget::OnMessageReceived(
    MessageUPtr && message,
    IConnection & connection,
    ISendTarget::SPtr const & target)
{
    if (destructing_)
    {
        TcpDatagramTransport::WriteWarning(
            TraceType,
            traceId_,
            "{0}-{1}: destructing, dropping message {2}, Actor = {3}, Action = '{4}'",
            localAddress_,
            address_,
            message->TraceId(),
            message->Actor,
            message->Action);

        return;
    }

    auto owner = ownerWPtr_.lock();
    if (!owner)
    {
        TcpDatagramTransport::WriteWarning(
            TraceType,
            traceId_,
            "{0}-{1}: owner transport has destructed, dropping message {2}, Actor = {3}, Action = '{4}'",
            localAddress_,
            address_,
            message->TraceId(),
            message->Actor,
            message->Action);

        return;
    }

    if (message->Actor == Actor::TransportSendTarget)
    {
        TcpDatagramTransport::WriteInfo(
            TraceType,
            traceId_,
            "{0}-{1}: received {2} message {3} for connection {4}",
            localAddress_,
            address_,
            message->Action,
            message->TraceId(),
            connection.TraceId());

        if (message->Action == *Constants::ReconnectAction)
        {
            StartConnectionRefresh(&connection);
            return;
        }

        if (message->Action == *ScheduleCloseAction)
        {
            // Although this is the last incoming message on this connection, we cannot simply
            // close the connection yet, there may be a reply message later destined to the remote side
            // so we need to delay ScheduleConnectionClose call, which disables sending on "connection".
            auto delayBeforeStopSend = SecurityConfig::GetConfig().SessionExpirationCloseDelay;
            TimeoutHeader th;
            if (message->Headers.TryReadFirst(th))
            {
                delayBeforeStopSend = th.Timeout;
                TcpDatagramTransport::WriteInfo(TraceType, traceId_, "close delay found in TimeoutHeader: {0}", th.Timeout);
            }

            delayBeforeStopSend = delayBeforeStopSend - TransportConfig::GetConfig().CloseDrainTimeout;
            if (delayBeforeStopSend < TimeSpan::Zero)
            {
                delayBeforeStopSend = TimeSpan::Zero;
            }

            auto fault = message->FaultErrorCodeValue;
            TcpDatagramTransport::WriteTrace(
                ((SecurityConfig::GetConfig().SessionExpirationCloseDelay != TimeSpan::Zero) && (delayBeforeStopSend == TimeSpan::Zero))? LogLevel::Warning : LogLevel::Info,
                TraceType,
                traceId_,
                "{0}-{1}: incoming fault = {2}, delay StopSendAndScheduleClose() on connection {3} by {4}",
                localAddress_,
                address_,
                fault,
                connection.TraceId(),
                delayBeforeStopSend);

            IConnectionWPtr connectionWPtr = connection.GetWPtr();
            if (fault == SESSION_EXPIRATION_FAULT)
            {
                fault = ErrorCodeValue::SecuritySessionExpiredByRemoteEnd;
            }

            ISendTarget::WPtr targetWPtr = target;
            Threadpool::Post(
                [this, targetWPtr = move(targetWPtr), connectionWPtr = move(connectionWPtr), fault]()
                {
                    if (auto target = targetWPtr.lock())
                    {
                        if (auto connectionSPtr = connectionWPtr.lock())
                        {
                            StopSendAndScheduleClose(*connectionSPtr, fault, false, TimeSpan::Zero);
                        }
                    }
                },
                delayBeforeStopSend);

            return;
        }

        if (message->Action == *Constants::ClaimsTokenError)
        {
            ConnectionAuthMessage connectionAuthMessage;
            if (message->GetBody<ConnectionAuthMessage>(connectionAuthMessage))
            {
                connection.Abort(ErrorCode(ErrorCodeValue::InvalidCredentials, connectionAuthMessage.TakeMessage()));
            }
            else
            {
                connection.Abort(ErrorCodeValue::InvalidCredentials);
            }

            return;
        }

        if (message->Action == *Constants::ConnectionAuth)
        {
            ConnectionAuthMessage connectionAuthMessage;
            if (message->HasFaultBody)
            {
                if (!message->GetBody<ConnectionAuthMessage>(connectionAuthMessage))
                {
                    connection.Abort(ErrorCodeValue::OperationFailed);
                    return;
                }
            }

            ErrorCode authStatus(message->FaultErrorCodeValue, wstring(connectionAuthMessage.ErrorMessage()));
            textTrace.WriteTrace(
                authStatus.ToLogLevel(),
                TraceType,
                traceId_,
                "{0}-{1}: connection auth status received for {2}: {3}:{4}",
                localAddress_,
                address_,
                connection.TraceId(),
                authStatus,
                connectionAuthMessage);

            connection.SetConnectionAuthStatus(authStatus, connectionAuthMessage.RoleGranted());
            return;
        }

        TcpDatagramTransport::WriteWarning(
            TraceType,
            traceId_,
            "{0}-{1}: dropping message {2}, Actor = {3}, unexpected Action = '{4}'",
            localAddress_,
            address_,
            message->TraceId(),
            message->Actor,
            message->Action);

        return;
    }

    owner->OnMessageReceived(std::move(message), connection, target);
}

IConnectionSPtr TcpSendTarget::ChooseExistingConnectionForSendChl(TransportPriority::Enum priority)
{
    auto now = Stopwatch::Now();
    for (auto const & connection : connections_)
    {
        bool canSend = connection->CanSend();
        bool priorityMatch = (connection->GetPriority() == priority);

        if (canSend)
        {
            if (priorityMatch) return connection;
        }
    }

    return nullptr;
}

_Use_decl_annotations_
ErrorCode TcpSendTarget::GetConnectionForSend(IConnectionSPtr & connection, TransportPriority::Enum priority)
{
    connection = nullptr;
    bool shouldStartConnection = false;
    {
        AcquireWriteLock grab(lock_);

        if (destructing_)
        {
            TcpDatagramTransport::WriteInfo(
                TraceType,
                traceId_,
                "{0}-{1}: failed to get connection as target is being cleaned up",
                localAddress_,
                address_);

            return ErrorCodeValue::ObjectClosed;
        }

        connection = ChooseExistingConnectionForSendChl(priority);
        if (!connection)
        {
            if (anonymous_)
            {
                return ErrorCodeValue::CannotConnectToAnonymousTarget;
            }

            TcpConnectionSPtr tcpConnection;
            auto error = AddConnection_CallerHoldingLock(nullptr, priority, tcpConnection);
            if (!error.IsSuccess())
            {
                return error;
            }

            connection = move(tcpConnection);
            shouldStartConnection = true;
        }

        connection->AddPendingSend();
    }

    if (shouldStartConnection)
    {
        // Start outside of lock scope
        if (!connection->Open())
        {
            TcpDatagramTransport::WriteWarning(
                TraceType,
                traceId_,
                "{0}-{1}: Unable to start created connection {2}",
                localAddress_,
                address_,
                connection->TraceId());

            return ErrorCodeValue::OperationCanceled; // retry-able error
        }
    }

    return ErrorCodeValue::Success;
}

_Use_decl_annotations_ ErrorCode TcpSendTarget::AddConnection_CallerHoldingLock(
    Socket * acceptedSocket,
    TransportPriority::Enum priority,
    TcpConnectionSPtr & connection)
{
    auto transport = ownerWPtr_.lock();
    if (!transport) return ErrorCodeValue::OperationCanceled;

    auto error = TcpConnection::Create(
        transport,
        shared_from_this(),
        acceptedSocket,
        msgHandler_,
        security_,
        sspiTarget_,
        priority,
        flags_,
        transport->ConnectionOpenTimeout(),
        connection);

    if (!error.IsSuccess())
    {
        TcpDatagramTransport::WriteWarning(
            TraceType,
            traceId_,
            "{0}-{1}: TcpConnection::Create(acceptedSocket = {2}) failed: {3}",
            localAddress_,
            address_,
            acceptedSocket != nullptr,
            error);

        return error;
    }

    connections_.push_back(connection);
    TraceConnectionsChl();
    return error;
}

uint64 TcpSendTarget::Instance() const
{
    return instance_;
}

void TcpSendTarget::UpdateInstanceIfNeeded(uint64 instance)
{
    vector<IConnectionSPtr> connectionsInvalidated;
    {
        AcquireWriteLock lockInScope(lock_);

        if ((instance == 0) || (instance == instance_))
        {
            return;
        }

        if (instance < instance_)
        {
            trace.TargetInstanceIngored(traceId_, localAddress_, address_, instance_, instance);
            return;
        }

        trace.TargetInstanceUpdated(traceId_, localAddress_, address_, instance_, instance);
        instance_ = instance;
        isDown_ = false;

        ValidateConnectionsChl(instance_, Stopwatch::Now());
    }
}

void TcpSendTarget::ValidateConnectionsChl(uint64 targetInstance, StopwatchTime now)
{
    uint64 instanceLowerBound = targetInstance;
    if (isDown_)
    {
        // When target is down, current target instance is also obsolete
        instanceLowerBound = targetInstance + 1;
    }

    size_t invalidatedCount = 0;

    // Prefer confirmed connection with smaller nonce value, will move it to the front
    size_t confirmedToMoveToFront = numeric_limits<size_t>::max();
    Guid nonceMin;
    size_t confirmedCount = 0;

    for (size_t i = 0; i < connections_.size(); ++ i)
    {
        if (!connections_[i]->Validate(instanceLowerBound, now))
        {
            connections_[i] = nullptr; // Already closed inside Validate() above
            ++ invalidatedCount;
        }
        else if (connections_[i]->IsConfirmed())
        {
            if (confirmedCount == 0)
            {
                nonceMin = connections_[i]->ListenSideNonce();
                confirmedToMoveToFront = i;
            }
            else if (connections_[i]->ListenSideNonce() < nonceMin)
            {
                nonceMin = connections_[i]->ListenSideNonce();
                confirmedToMoveToFront = i;
            }

            ++ confirmedCount;
        }
    }

    if (confirmedCount > 2)
    {
        TcpDatagramTransport::WriteInfo(
            TraceType, traceId_,
            "{0}-{1}: found {2} confirmed connections, greater than 2",
            localAddress_, address_, confirmedCount);
    }

    if ((invalidatedCount > 0) ||
        ((confirmedCount > 0) && (confirmedToMoveToFront != 0)))
    {
        // Need to update connection lists:
        // 1. Should remove invalidated connections
        // 2. Should move confirmed connection with min nonce to the front. It is better to pay the price of moving
        //    it here, instead of slowing down every send operation to decide which confirmed connection to use.

        vector<IConnectionSPtr> connectionsAdjusted;
        connectionsAdjusted.reserve(connections_.size() - invalidatedCount);

        if (confirmedCount > 0)
        {
            // Make sure the chosen confirmed connection is at the front,
            // in order to make connection selecting efficient for sending
            connectionsAdjusted.push_back(move(connections_[confirmedToMoveToFront]));
        }

        for (size_t i = 0; i < connections_.size(); ++ i)
        {
            if ((connections_[i]) && (i != confirmedToMoveToFront))
            {
                connectionsAdjusted.push_back(move(connections_[i]));
            }
        }

        connections_ = move(connectionsAdjusted);
        TraceConnectionsChl();
    }
}

void TcpSendTarget::UpdateConnectionInstance(
    IConnection & connection,
    ListenInstance const & remoteListenInstance)
{
    {
        AcquireWriteLock lockInScope(lock_);

        for (auto iter = connections_.begin(); iter != connections_.end(); ++ iter)
        {
            if ((*iter).get() == (&connection))
            {
                (*iter)->UpdateInstance(remoteListenInstance);
                TraceConnectionsChl();
                ValidateConnectionsChl(instance_, Stopwatch::Now());
                return;
            }
        }
    }

    TcpDatagramTransport::WriteWarning(
        TraceType, traceId_,
        "{0}-{1}: failed to locate connection {2} to update instance",
        localAddress_, address_, connection.TraceId());
}

void TcpSendTarget::SetRecvLimit(ULONG messageSizeLimit)
{
    AcquireWriteLock lockInScope(lock_);

    for (IConnectionSPtr & connection : connections_)
    {
        connection->SetRecvLimit(messageSizeLimit);
    }
}

void TcpSendTarget::SetSendLimits(ULONG messageSizeLimit, ULONG sendQueueLimit)
{
    AcquireWriteLock lockInScope(lock_);

    for (IConnectionSPtr & connection : connections_)
    {
        connection->SetSendLimits(messageSizeLimit, sendQueueLimit);
    }
}

void TcpSendTarget::UpdateSecurity(TransportSecuritySPtr const & value)
{
    SecurityConfig & securityConfig = SecurityConfig::GetConfig();
    TimeSpan refreshDelay = securityConfig.MaxSessionRefreshDelay;
    if (refreshDelay > TimeSpan::Zero)
    {
        if (TimeSpan::Zero < securityConfig.SessionExpiration && securityConfig.SessionExpiration < refreshDelay)
        {
            refreshDelay = securityConfig.SessionExpiration;
        }
    }

    TcpDatagramTransport::WriteInfo(
        TraceType, traceId_,
        "{0}-{1}: UpdateSecurity: closing current sessions in {2}",
        localAddress_, address_, refreshDelay);

    {
        AcquireWriteLock lockInScope(lock_);

        security_ = value;
        for (auto const & connection : connections_)
        {
            connection->ScheduleSessionExpiration(
                TimeSpan::FromMilliseconds(refreshDelay.TotalPositiveMilliseconds() * Random().NextDouble()),
                true);
        }
    }
}

wstring TcpSendTarget::ConnectionStatesToStringChl()
{
    wstring connections;
    StringWriter writer(connections);

    writer.WriteLine();
    for (auto iter = connections_.cbegin(); iter != connections_.cend(); ++ iter)
    {
        if (*iter)
        {
            writer.WriteLine("{0}", (*iter)->ToString());
        }
    }

    return connections;
}

void TcpSendTarget::ValidateConnections(StopwatchTime now)
{
    AcquireWriteLock lockInScope(lock_);

    ValidateConnectionsChl(instance_, now);
}

wstring const & TcpSendTarget::SspiTarget() const
{
    AcquireReadLock lockInScope(lock_);

    return sspiTarget_;
}

void TcpSendTarget::UpdateSspiTargetIfNeeded(wstring const & value)
{
    if (!sspiTargetToBeSet_ || value.empty())
    {
        return;
    }

    AcquireWriteLock lockInScope(lock_);

    sspiTarget_ = value;
    sspiTargetToBeSet_ = sspiTarget_.empty();
}

size_t TcpSendTarget::BytesPendingForSend()
{
    AcquireReadLock grab(lock_);

    size_t byteSTotal = 0;
    for (auto & iter : connections_)
    {
        byteSTotal += iter->BytesPendingForSend();
    }

    return byteSTotal;
}

size_t TcpSendTarget::MessagesPendingForSend()
{
    AcquireReadLock grab(lock_);

    size_t total = 0;
    for (auto & iter : connections_)
    {
        total += iter->MessagesPendingForSend();
    }

    return total;
}


void TcpSendTarget::PurgeExpiredOutgoingMessages(StopwatchTime now)
{
    vector<IConnectionSPtr> connections; //Avoid ~Message() under lock
    {
        AcquireReadLock lockInScope(lock_);
        connections = connections_;
    }

    for(IConnectionSPtr const & connection : connections)
    {
        connection->PurgeExpiredOutgoingMessages(now);
    }
}

TcpDatagramTransportSPtr TcpSendTarget::Owner()
{
    return ownerWPtr_.lock();
}

TcpDatagramTransportWPtr const & TcpSendTarget::OwnerWPtr() const
{
    return ownerWPtr_;
}

IConnectionSPtr TcpSendTarget::RemoveConnection(
    IConnection const & connection,
    bool saveToScheduledClose)
{
    IConnectionSPtr connectionRemoved;

    AcquireWriteLock lockInScope(lock_);

    if (expiringConnection_ == &connection)
    {
        TcpDatagramTransport::WriteInfo(TraceType, traceId_, "RemoveConnection: removing expiring connection {0}", TextTracePtr(expiringConnection_));
        expiringConnection_ = nullptr;
    }

    if (newConnection_.get() == &connection)
    {
        connectionRemoved = move(newConnection_);
        if (saveToScheduledClose)
        {
            scheduledClose_.push_back(connectionRemoved);
        }

        return connectionRemoved;
    }

    for (auto iter = connections_.begin(); iter != connections_.end(); ++iter)
    {
        if ((*iter).get() == (&connection))
        {
            connectionRemoved = move(*iter);
            TcpDatagramTransport::WriteInfo(
                TraceType, traceId_,
                "{0}-{1}: removing connection {2} from connections_",
                localAddress_, address_, connection.TraceId());

            connections_.erase(iter);
            TraceConnectionsChl();

            if (connectionRemoved->IsConfirmed())
            {
                // If there is another another confirmed connection, then move it to the front
                ValidateConnectionsChl(instance_, Stopwatch::Now());
            }

            if (saveToScheduledClose)
            {
                scheduledClose_.push_back(connectionRemoved);
            }

            break;
        }
    }

    return connectionRemoved;
}

void TcpSendTarget::SendScheduleCloseMessage(IConnection & connection, TimeSpan closeDelay)
{
    MessageUPtr scheduleCloseMessage = CreateScheduleCloseMessage(connection.GetFault(), closeDelay);
    TcpDatagramTransport::WriteInfo(
        TraceType, traceId_,
        "{0}-{1}: sending {2} message for connection {3}",
        localAddress_, address_, scheduleCloseMessage->Action, connection.TraceId());

    // scheduleCloseMessage is the last message sent on this connection, as it has been removed from connections_
    connection.SendOneWay_Dedicated(move(scheduleCloseMessage), TimeSpan::MaxValue);
}

MessageUPtr TcpSendTarget::CreateScheduleCloseMessage(ErrorCode fault, TimeSpan closeDelay)
{
    MessageUPtr message = make_unique<Message>();
    message->Headers.Add(MessageIdHeader());
    message->Headers.Add(ActorHeader(Actor::TransportSendTarget));
    message->Headers.Add(ActionHeader(*ScheduleCloseAction));
    message->Headers.Add(FaultHeader(fault.ReadValue()));
    message->Headers.Add(TimeoutHeader(closeDelay));
    return message;
}

void TcpSendTarget::StopSendAndScheduleClose(IConnection const & connection, ErrorCode const & fault, bool notifyRemote, TimeSpan delay)
{
    //Remove this connection so that ougtgoing messages will not be queued onto it.
    IConnectionSPtr connectionToClose = RemoveConnection(connection, true);
    if (!connectionToClose)
    {
        TcpDatagramTransport::WriteInfo(
            TraceType, traceId_,
            "{0}-{1}: ScheduleConnectionClose: connection {2} not found for removal",
            localAddress_, address_, connection.TraceId());

        return;
    }

    // Sending messages already queued after fault reported is allowed, but no more outgoing messages will
    // be queued for sending onto this connection as it has been "removed" from connections_. connection fault
    // must be reported after removal, otherwise, "sending"(queueing) outgoing messages onto this connection
    // will fail (with "Msg_InvalidStateForSend" traced).
    connectionToClose->ReportFault(fault);
    if (notifyRemote)
    {
        SendScheduleCloseMessage(*connectionToClose, delay);
    }

    TcpDatagramTransport::WriteInfo(
        TraceType, traceId_,
        "{0}-{1}: scheduling closing connection {2} in {3}",
        localAddress_, address_, connection.TraceId(), delay);

    IConnectionWPtr connectionWPtr = connectionToClose;
    Threadpool::Post(
        [connectionWPtr]()
        {
            auto connectionSPtr = connectionWPtr.lock();
            if (connectionSPtr)
            {
                connectionSPtr->Close();
            }
        },
        delay);
}

TransportSecuritySPtr const & TcpSendTarget::Security() const
{
    return security_;
}

wstring const & TcpSendTarget::Address() const
{
    return address_;
}

wstring const & TcpSendTarget::Id() const
{
    return id_;
}

bool TcpSendTarget::IsAnonymous() const
{
    return anonymous_;
}

wstring const & TcpSendTarget::TraceId() const
{
    return traceId_;
}

bool TcpSendTarget::HasId() const
{
    return hasId_;
}

uint64 TcpSendTarget::GetObjCount()
{
    return objCount_.load();
}

void TcpSendTarget::Test_SuspendReceive()
{
    AcquireWriteLock lockInScope(lock_);

    for (auto const & connection : connections_)
    {
        connection->SuspendReceive([]{});
    }
}

void TcpSendTarget::Test_ResumeReceive()
{
    AcquireWriteLock lockInScope(lock_);

    for (auto const & connection : connections_)
    {
        connection->ResumeReceive();
    }
}

void TcpSendTarget::SetMaxOutgoingMessageSize(size_t value)
{
    TcpDatagramTransport::WriteInfo(
        TraceType, traceId_,
        "SetMaxOutgoingMessageSize: {0}/0x{0:x} -> {1}/0x{1:x}",
        maxOutgoingMessageSize_, value);

    maxOutgoingMessageSize_ = value;
}

void TcpSendTarget::Test_Block()
{
    test_blocked_ = true;
}

void TcpSendTarget::Test_Unblock()
{
    test_blocked_ = false;
}

IConnection const* TcpSendTarget::Test_ActiveConnection() const
{
    AcquireReadLock grab(lock_);
    return connections_.front().get();
}
