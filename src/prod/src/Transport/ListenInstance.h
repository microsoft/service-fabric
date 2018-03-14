// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    class ListenInstance : public Serialization::FabricSerializable
    {
    public:
        ListenInstance();
        ListenInstance(std::wstring const & address, uint64 instance, Common::Guid const & nonce);

        void SetAddress(std::wstring const & address) { address_ = address; }
        std::wstring const & Address() const { return address_; }

        uint64 Instance() const { return instance_; }
        void SetInstance(uint64 instance);

        // For duplicate connection elimination
        Common::Guid const & Nonce() const { return nonce_; }

        bool operator == (const ListenInstance & rhs) const
        {
            return
                (address_ == rhs.address_) &&
                (instance_ == rhs.instance_) &&
                (nonce_ == rhs.nonce_);
        }

        bool operator != (const ListenInstance & rhs) const
        {
            return !(*this == rhs);
        }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;
        static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name);
        void FillEventData(Common::TraceEventContext & context) const;

        FABRIC_FIELDS_03(address_, instance_, nonce_);

    private:
        std::wstring address_;
        uint64 instance_;
        Common::Guid nonce_;
    };
}
