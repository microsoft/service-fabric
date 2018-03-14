// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#define PERF_COUNTER_OBJECT_PROCESS L"Process"
#define PERF_COUNTER_THREAD_COUNT L"Thread Count"

using namespace Transport;
using namespace Common;
using namespace std;

static StringLiteral const TraceType("AcceptThrottle");

AcceptThrottle::AcceptThrottle()
    : incomingConnectionCount_(0)
    , connectionDropCount_(0)
    , acceptSuspended_(false)
    , throttle_(TransportConfig::GetConfig().IncomingConnectionThrottle)
{
    // This singleton will never be destucted, thus no need to capture shared_ptr to keep it alive
    TransportConfig::GetConfig().IncomingConnectionThrottleEntry.AddHandler([this](EventArgs const&)
    {
        vector<TcpConnectionSPtr> abortedConnections; // avoid destruction under lock
        {
            AcquireWriteLock grab(lock_);

            throttle_ = TransportConfig::GetConfig().IncomingConnectionThrottle;
            WriteInfo(TraceType, "throttle config updated, new value = {0}", throttle_);

            if (ShouldThrottle_CallerHoldingLock())
            {
                ReduceIncomingConnections_CallerHoldingLock(abortedConnections);
            }
            ResumeAcceptIfNeeded_CallerHoldingLock();
        }
    });
}

bool AcceptThrottle::OnConnectionAccepted(
    TcpDatagramTransport & transport,
    IListenSocket & listenSocket,
    Common::Socket & socket,
    Common::Endpoint const & remoteEndpoint,
    u_short localListenPort)
{
    Endpoint connectionGroupId = remoteEndpoint;
    connectionGroupId.Port = localListenPort; // remote port is an ephemeral port,
    // replace it with localListenPort to distinguish different listeners in this process

    // avoid destruction under lock
    vector<TcpConnectionSPtr> abortedConnections;
    bool shouldCloseAcceptedSocket = false;
    bool canAcceptMore = true;
    TcpConnectionSPtr connection;
    {
        AcquireWriteLock grab(lock_);

        if (ShouldSuspendAccept_CallerHoldingLock())
        {
            canAcceptMore = false;
            listenSocket.SuspendAccept(); // Suspend under lock to avoid racing with resuming
            trace.SuspendAccept(transport.TraceId(), transport.ListenAddress(), CheckedIncomingConnectionCount_CallerHoldingLock());
            acceptSuspended_ = true;

            // socket will be closed outside lock_
            shouldCloseAcceptedSocket = true;
            ++connectionDropCount_;

            ReduceIncomingConnections_CallerHoldingLock(abortedConnections);
        }
        else
        {
            if (ShouldThrottle_CallerHoldingLock(1))
            {
                Endpoint const* topGroupIdPtr;
                auto topGroup = GetTopConnectionGroup_CallerHoldingLock(topGroupIdPtr);
                if (topGroup && (*topGroupIdPtr == connectionGroupId))
                {
                    // socket will be closed outside lock_
                    shouldCloseAcceptedSocket = true;
                    ++connectionDropCount_;
                }

                ReduceIncomingConnections_CallerHoldingLock(abortedConnections, topGroup, topGroupIdPtr);
            }

            if (!shouldCloseAcceptedSocket)
            {
                connection = transport.OnConnectionAccepted(socket, remoteEndpoint);
                if (connection)
                {
                    ++incomingConnectionCount_;
                    CheckedIncomingConnectionCount_CallerHoldingLock();

                    connection->SetGroupId(connectionGroupId);
                    auto groupIter = connectionGroups_.find(connectionGroupId);
                    if (groupIter == connectionGroups_.cend())
                    {
                        ConnectionGroup group;
                        group.emplace(make_pair(connection.get(), connection));
                        connectionGroups_.emplace(make_pair(connectionGroupId, group));
                        WriteNoise(
                            TraceType,
                            "add connection {0} to group {1}, group count grew to {2}",
                            connection->TraceId(),
                            connectionGroupId,
                            connectionGroups_.size());
                    }
                    else
                    {
                        groupIter->second.emplace(make_pair(connection.get(), connection));
                        WriteNoise(TraceType, "add connection {0} to group {1}", connection->TraceId(), connectionGroupId);
                    }

                    WriteNoise(TraceType, "after adding, this = {0}", *this);
                }
                else
                {
                    // socket will be closed outside lock_
                    shouldCloseAcceptedSocket = true;
                    ++connectionDropCount_;
                }
            }
        }
    }

    if (shouldCloseAcceptedSocket)
    {
        socket.Shutdown(SocketShutdown::None);
    }

    return canAcceptMore;
}

bool AcceptThrottle::ShouldSuspendAccept_CallerHoldingLock() const
{
    auto throttle = throttle_;
    // Suspend accept and drop newly accepted socket when incoming connection total reaches limit
    return (0 < throttle) && (ThrottleWithMargin(throttle) <= CheckedIncomingConnectionCount_CallerHoldingLock());
}

size_t AcceptThrottle::Test_IncomingConnectionUpperBound()
{
    auto throttle = TransportConfig::GetConfig().IncomingConnectionThrottle;
    return ThrottleWithMargin(throttle) + 1; 
}

bool AcceptThrottle::ShouldThrottle_CallerHoldingLock(uint newConnectionAccepted) const
{
    auto throttle = throttle_;
    return (0 < throttle) && (throttle <= (CheckedIncomingConnectionCount_CallerHoldingLock() + newConnectionAccepted));
}

_Use_decl_annotations_ AcceptThrottle::ConnectionGroup* AcceptThrottle::GetTopConnectionGroup_CallerHoldingLock(Common::Endpoint const* & groupIdPtr)
{
    ConnectionGroup* largestGroup = nullptr;
    groupIdPtr = nullptr;

    size_t maxGroupSize = 0;
    for (auto & group : connectionGroups_)
    {
        if (group.second.size() > maxGroupSize)
        {
            maxGroupSize = group.second.size();
            largestGroup = &(group.second);
            groupIdPtr = &(group.first);
        }
    }

    return largestGroup;
}

_Use_decl_annotations_ void AcceptThrottle::ReduceIncomingConnections_CallerHoldingLock(
    vector<TcpConnectionSPtr> & abortedConnections,
    ConnectionGroup* topGroup,
    Endpoint const* topGroupId)
{
    abortedConnections.clear();

    if (!topGroup)
    {
        topGroup = GetTopConnectionGroup_CallerHoldingLock(topGroupId);
    }

    if (!topGroup)
    {
        WriteNoise(TraceType, "All groups are empty");
        return;
    }

    int toAbort = (static_cast<int>(topGroup->size()) + 1) / 2;
    WriteInfo(
        TraceType,
        "remove {0} connections out of {1} from group {2}, incomingConnectionCount={3}",
        toAbort,
        topGroup->size(),
        *topGroupId,
        CheckedIncomingConnectionCount_CallerHoldingLock());

    abortedConnections.reserve(toAbort);
    for (auto iter = topGroup->begin(); (iter != topGroup->end()) && (toAbort-- > 0);)
    {
        auto connectionSPtr = iter->second.lock();
        if (connectionSPtr)
        {
            connectionSPtr->Abort(ErrorCodeValue::IncomingConnectionThrottled);
            ++connectionDropCount_;
            abortedConnections.emplace_back(move(connectionSPtr));
        }

        iter = topGroup->erase(iter);
    }

    WriteNoise(TraceType, "after reducing, this = {0}", *this);
}

size_t AcceptThrottle::OnConnectionDestructed(
    TcpConnection const * connection,
    Common::Endpoint const & groupId)
{
    AcquireWriteLock grab(lock_);

    --incomingConnectionCount_;

    WriteNoise(TraceType, "after connection destructed, this = {0}", *this);

    auto groupIter = connectionGroups_.find(groupId);
    if (groupIter != connectionGroups_.cend())
    {
        WriteNoise(TraceType, "remove connection {0} from group {1}", connection->TraceId(), groupId);
        groupIter->second.erase(connection);

        if (groupIter->second.empty() && (connectionGroups_.size() > 1000))
        {
            WriteNoise(TraceType, "remove connection group {0}, {1} groups remained", groupId, connectionGroups_.size());
            connectionGroups_.erase(groupIter);
        }
    }

    ResumeAcceptIfNeeded_CallerHoldingLock();
    return incomingConnectionCount_;
}

void AcceptThrottle::ResumeAcceptIfNeeded_CallerHoldingLock()
{
    if (!acceptSuspended_ || ShouldThrottle_CallerHoldingLock())
        return;

    trace.ResumeAccept(incomingConnectionCount_);
    for (auto const & listener : listeners_)
    {
        auto sptr = listener.second.lock();
        if (sptr)
        {
            sptr->ResumeAcceptIfNeeded();
        }
    }

    acceptSuspended_ = false;
}

void AcceptThrottle::AddListener(TcpDatagramTransportSPtr const & newListener)
{
    if (newListener->IsClientOnly()) return;

    AcquireWriteLock grab(lock_);

#ifdef DBG
    for (auto const & existing : listeners_)
    {
        Invariant(existing.first != newListener.get());
    }
#endif

    WriteInfo(TraceType, "add listener {0} {1}", newListener->TraceId(), newListener->ListenAddress());
    listeners_.emplace(make_pair(newListener.get(), newListener));
}

void AcceptThrottle::RemoveListener(TcpDatagramTransport const * listener)
{
    if (listener->IsClientOnly()) return;

    AcquireWriteLock grab(lock_);

    for (auto iter = listeners_.cbegin(); iter != listeners_.cend(); ++iter)
    {
        if (iter->first == listener)
        {
            WriteInfo(TraceType, "remove listener {0} {1}", listener->TraceId(), listener->ListenAddress());
            listeners_.erase(iter);
            return;
        }
    }

    Assert::CodingError("{0} not found", listener->TraceId());
}

AcceptThrottle::ConnectionGroupMap AcceptThrottle::Test_ConnectionGroups() const
{
    AcquireReadLock grab(lock_);
    return connectionGroups_;
}

size_t AcceptThrottle::IncomingConnectionCount() const
{
    AcquireReadLock grab(lock_);
    return CheckedIncomingConnectionCount_CallerHoldingLock();
}

size_t AcceptThrottle::ConnectionDropCount() const
{
    AcquireReadLock grab(lock_);
    return connectionDropCount_;
}

void AcceptThrottle::Test_Reset()
{
    WriteInfo(TraceType, "Test_Reset");

    AcquireWriteLock grab(lock_);
    connectionGroups_.clear();
    connectionDropCount_ = 0;
    acceptSuspended_ = false;
}

uint AcceptThrottle::Test_GetThrottle() const
{
    AcquireReadLock grab(lock_);
    return throttle_;
}

void AcceptThrottle::Test_SetThrottle(uint throttle)
{
    AcquireWriteLock grab(lock_);

    throttle_ =  throttle? throttle : TransportConfig::GetConfig().IncomingConnectionThrottle;
    WriteInfo(TraceType, "throttle set to {0}", throttle_);
}

void AcceptThrottle::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w << '[';
    for (auto const & group : connectionGroups_)
    {
        w.Write("({0},{1})", group.first, group.second.size());
    }
    w.Write(", incomingConnectionCount = {0}, dropCount = {1}, acceptSuspended = {2}]", CheckedIncomingConnectionCount_CallerHoldingLock(), connectionDropCount_, acceptSuspended_);
}

//
// Singleton initilization routine
//

INIT_ONCE AcceptThrottle::initOnce_ = INIT_ONCE_STATIC_INIT;
AcceptThrottle* AcceptThrottle::singleton_ = nullptr;

BOOL CALLBACK AcceptThrottle::InitFunction(PINIT_ONCE, PVOID, PVOID *)
{
    singleton_ = new AcceptThrottle();
    return TRUE;
}

AcceptThrottle * AcceptThrottle::GetThrottle()
{
    PVOID lpContext = NULL;
    BOOL  bStatus = FALSE;

    bStatus = ::InitOnceExecuteOnce(
        &AcceptThrottle::initOnce_,
        AcceptThrottle::InitFunction,
        NULL,
        &lpContext);

    ASSERT_IF(!bStatus, "Failed to initialize AcceptThrottle singleton");
    return AcceptThrottle::singleton_;
}
