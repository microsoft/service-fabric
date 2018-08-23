// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

// This macro is to make sure that session expiration code generates a fault
// that will be ignored by RequestReply, since the connection will be kept
// alive to receive pending reply. The alternative is not reporting session
// expiration fault until connection is closed, this is bad for FabricClient,
// which wants to create a new connection immediately after the current one
// is disabled for sending due to session expiration.
#define SESSION_EXPIRATION_FAULT Common::ErrorCodeValue::SecuritySessionExpired

namespace Transport
{
    struct IConnection
    {
        virtual ~IConnection() = 0;

        virtual std::wstring const & TraceId() const = 0;
        virtual IConnectionWPtr GetWPtr() = 0;

        virtual TransportPriority::Enum GetPriority() const = 0;
        virtual TransportFlags & GetTransportFlags() = 0;

        virtual void SetSecurityContext(TransportSecuritySPtr const & transportSecurity, std::wstring const & sspiTarget) = 0;
        virtual void ScheduleSessionExpiration(Common::TimeSpan expiration, bool securitySettingsUpdated = false) = 0;
        virtual void OnSessionExpired() = 0;
        virtual void SetConnectionAuthStatus(Common::ErrorCode const & authStatus, RoleMask::Enum roleGranted) = 0;
        virtual void CompleteClaimsRetrieval(Common::ErrorCode const &, std::wstring const & localClaims) = 0;

        virtual bool Open() = 0;
        using ReadyCallback = std::function<void(IConnection*)>; //called when opened successfully
        virtual void SetReadyCallback(ReadyCallback && readyCallback) = 0;
        virtual void OnConnectionReady() = 0;

        virtual void SuspendReceive(Common::ThreadpoolCallback const & action) = 0; // !!! can only be called in receive complete callback
        virtual void ResumeReceive(bool incomingDataAllowed = true) = 0;

        virtual void AddPendingSend() = 0;
        virtual Common::ErrorCode SendOneWay(MessageUPtr && message, Common::TimeSpan expiration) = 0;
        // Send operation dedicated for this connection, not chosen from SendTarget connection list
        virtual Common::ErrorCode SendOneWay_Dedicated(MessageUPtr && message, Common::TimeSpan expiration) = 0;

        virtual void Close() = 0;
        virtual void Abort(Common::ErrorCode const & fault) = 0;
        virtual void StopSendAndScheduleClose(
            Common::ErrorCode const & fault,
            bool notifyRemote,
            Common::TimeSpan delay) = 0;

        virtual void ReportFault(Common::ErrorCode const & fault) = 0;
        virtual Common::ErrorCode GetFault() const = 0;

        virtual ISendTarget::SPtr SetTarget(ISendTarget::SPtr const & target) = 0;

        virtual bool CanSend() const = 0;

        virtual bool IsInbound() const = 0;
        virtual bool IsLoopback() const = 0; // Connected to loopback or local address?

        virtual void UpdateInstance(ListenInstance const & remoteListenInstance) = 0;

        // For duplicate connection elimination
        virtual Common::Guid const & ListenSideNonce() const = 0;

        virtual bool IsConfirmed() const = 0;

        virtual bool Validate(uint64 instanceLowerBound, Common::StopwatchTime now) = 0;

        virtual void SetRecvLimit(ULONG messageSizeLimit) = 0;
        virtual void SetSendLimits(ULONG messageSizeLimit, ULONG sendQueueLimit) = 0;

        virtual ULONG BytesPendingForSend() const = 0;
        virtual size_t MessagesPendingForSend() const = 0;

        virtual size_t IncomingFrameSizeLimit() const = 0;
        virtual size_t OutgoingFrameSizeLimit() const = 0;
        virtual void OnRemoteFrameSizeLimit(size_t remoteIncomingMax) = 0;

        virtual void PurgeExpiredOutgoingMessages(Common::StopwatchTime now) = 0;

        virtual std::wstring ToString() const = 0;
    };
}
