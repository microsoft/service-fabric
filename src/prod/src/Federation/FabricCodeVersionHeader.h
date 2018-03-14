// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    // Structure represents information about a particular instance of a node.
    struct FabricCodeVersionHeader : public Transport::MessageHeader<Transport::MessageHeaderId::FabricCodeVersion>, public Serialization::FabricSerializable
    {
    public:
        FabricCodeVersionHeader()
        {
        }

        FabricCodeVersionHeader(Common::FabricCodeVersion const& codeVersion)
             : codeVersion_(codeVersion)
        {
        }

        FABRIC_FIELDS_01(codeVersion_);

        static void ReadFromMessage(Transport::Message & message, Common::FabricCodeVersion & codeVersion)
        {
            FabricCodeVersionHeader header;
            if (message.Headers.TryReadFirst(header))
            {
                codeVersion = header.codeVersion_;
            }
            else
            {
                codeVersion = *Common::FabricCodeVersion::Default;
            }
        }

        void WriteTo (Common::TextWriter& w, Common::FormatOptions const&) const
        {
            w.Write(codeVersion_);
        }

    private:       
        Common::FabricCodeVersion codeVersion_;
    };
}
