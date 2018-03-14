// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    class AcceptThrottle : Common::TextTraceComponent<Common::TraceTaskCodes::Transport>
    {
        DENY_COPY(AcceptThrottle);
        friend class TcpConnection;

    public:
        typedef std::unordered_map<TcpConnection const *, TcpConnectionWPtr> ConnectionGroup;
        typedef std::unordered_map<Common::Endpoint, ConnectionGroup, Common::EndpointHash> ConnectionGroupMap;

        static AcceptThrottle * GetThrottle();

        void AddListener(TcpDatagramTransportSPtr const & listener);
        void RemoveListener(TcpDatagramTransport const * listener);

        bool OnConnectionAccepted(
            TcpDatagramTransport & transport,
            IListenSocket & listenSocket,
            Common::Socket & socket,
            Common::Endpoint const & remoteEndpoint,
            u_short localListenPort);

        size_t OnConnectionDestructed(
            TcpConnection const * connection,
            Common::Endpoint const & groupId);

        size_t IncomingConnectionCount() const;

        size_t ConnectionDropCount() const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        uint Test_GetThrottle() const;
        void Test_SetThrottle(uint throttle = 0);
        ConnectionGroupMap Test_ConnectionGroups() const;
        void Test_Reset();
        static size_t Test_IncomingConnectionUpperBound();

    private:
        AcceptThrottle();

        bool ShouldSuspendAccept_CallerHoldingLock() const;
        bool ShouldThrottle_CallerHoldingLock(uint newConnectionAccepted = 0) const;
        void ReduceIncomingConnections_CallerHoldingLock(
            _Out_ std::vector<TcpConnectionSPtr> & abortedConnections,
            _In_opt_ ConnectionGroup* topGroup = nullptr,
            _In_opt_ Common::Endpoint const* topGroupId = nullptr);

        ConnectionGroup* GetTopConnectionGroup_CallerHoldingLock(_Out_ Common::Endpoint const* & groupIdPtr);

        void ResumeAcceptIfNeeded_CallerHoldingLock();

        __forceinline size_t CheckedIncomingConnectionCount_CallerHoldingLock() const
        {
            CODING_ERROR_ASSERT(incomingConnectionCount_ < INT32_MAX);
            return incomingConnectionCount_;
        }

        __forceinline static size_t ThrottleWithMargin(size_t throttle)
        {
            return throttle + (throttle >> 3); // throttle + throttle/8
        }

        static BOOL CALLBACK InitFunction(PINIT_ONCE, PVOID, PVOID *);

        static INIT_ONCE initOnce_;
        static AcceptThrottle* singleton_;

        MUTABLE_RWLOCK(AcceptThrottle, lock_);

        // Incoming connections grouped by Endpoint(remoteIP, localListenPort), localListenPort is used to distinguish different listeners in this process
        ConnectionGroupMap connectionGroups_;

        size_t incomingConnectionCount_;
        size_t connectionDropCount_;
        bool acceptSuspended_;
        uint throttle_;

        std::unordered_map<TcpDatagramTransport const *, TcpDatagramTransportWPtr> listeners_;
    };
}
