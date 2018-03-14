// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    class IpcHeader : public MessageHeader<MessageHeaderId::Ipc>, public Serialization::FabricSerializable
    {
        DENY_COPY_ASSIGNMENT(IpcHeader);

    public:
        IpcHeader();
        IpcHeader(std::wstring const & from, DWORD fromProcessId);
        IpcHeader(IpcHeader && rhs);

        IpcHeader & operator=(IpcHeader && rhs);

        __declspec(property(get=getFrom)) std::wstring const & From;
        std::wstring const & getFrom() const { return from_; }

        __declspec(property(get = getFromProcessId)) DWORD FromProcessId;
        DWORD getFromProcessId() const { return fromProcessId_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_02(from_, fromProcessId_);

    private:
        std::wstring from_;
        DWORD fromProcessId_;
    };
}
