// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    // Abstract representation of a resolved remote target.  Address resolution
    // is allowed to be expensive (though not blocking), so this object represents
    // the results of that resolution.
    class ISendTarget
    {
    public:
        typedef std::shared_ptr<ISendTarget> SPtr; 
        typedef std::weak_ptr<ISendTarget> WPtr; 

        virtual ~ISendTarget() = default;

        virtual Common::ErrorCode SendOneWay(
                MessageUPtr && message,
                Common::TimeSpan expiration,
                TransportPriority::Enum priority = TransportPriority::Normal);

        virtual std::wstring const & Address() const = 0;

        virtual std::wstring const & LocalAddress() const = 0;

        virtual std::wstring const & Id() const = 0;
        virtual std::wstring const & TraceId() const = 0; 

        virtual bool IsAnonymous() const = 0;

        virtual void TargetDown(uint64 instance = 0);

        // Release cached resources, e.g. TCP connections
        virtual void Reset();

        virtual size_t MaxOutgoingMessageSize() const;
        virtual size_t BytesPendingForSend();
        virtual size_t MessagesPendingForSend();

        virtual size_t ConnectionCount() const = 0;

        virtual bool TryEnableDuplicateDetection();

        virtual TransportFlags GetTransportFlags() const;

        //LINUXTODO, remove this when lease layer supports UnreliableTransport
        virtual void Test_Block();
        virtual void Test_Unblock();
    };
}
