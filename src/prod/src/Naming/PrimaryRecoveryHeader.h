// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class PrimaryRecoveryHeader
        : public Transport::MessageHeader<Transport::MessageHeaderId::PrimaryRecovery>
        , public Serialization::FabricSerializable
    {
    public:
        PrimaryRecoveryHeader() 
        { 
        }

        void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const
        {
            w << "PrimaryRecoveryHeader";
        }
    };
}
