// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class RepartitionDescription : public Common::IFabricJsonSerializable, public Serialization::FabricSerializable
    {
        KIND_ACTIVATOR_BASE_CLASS(RepartitionDescription, PartitionKind)

    public:

        void WriteToEtw(uint16 contextSequenceId) const;
        virtual void WriteTo(__in Common::TextWriter& writer, Common::FormatOptions const&) const;

    protected:

        virtual void OnWriteToEtw(uint16 contextSequenceId) const;
    };
}
