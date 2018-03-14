// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class CacheModeHeader
        : public Transport::MessageHeader<Transport::MessageHeaderId::CacheMode>
        , public Serialization::FabricSerializable
    {
    public:
        CacheModeHeader ();

        explicit CacheModeHeader (Reliability::CacheMode::Enum cacheMode);

        explicit CacheModeHeader (ServiceLocationVersion const & previousVersion);

        CacheModeHeader (Reliability::CacheMode::Enum cacheMode, ServiceLocationVersion const & previousVersion);
        
        __declspec(property(get=get_CacheMode, put=put_CacheMode)) Reliability::CacheMode::Enum CacheMode;
        inline Reliability::CacheMode::Enum get_CacheMode() const { return cacheMode_; }
        inline void put_CacheMode(Reliability::CacheMode::Enum value) { cacheMode_ = value; }
        
        __declspec(property(get=get_PreviousVersion, put=put_PreviousVersion)) ServiceLocationVersion const & PreviousVersion;
        inline ServiceLocationVersion const & get_PreviousVersion() const { return previousVersion_; }
        inline void put_PreviousVersion(ServiceLocationVersion const & value) { previousVersion_ = value; }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const;

        std::wstring ToString() const;

        FABRIC_FIELDS_02(cacheMode_, previousVersion_);

    private:
        Reliability::CacheMode::Enum cacheMode_;
        ServiceLocationVersion previousVersion_;
    };
}

