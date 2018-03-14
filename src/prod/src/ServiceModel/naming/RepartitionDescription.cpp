// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Naming;
using namespace std;

StringLiteral const TraceComponent("RepartitionDescription");

// JSON activation
//
shared_ptr<RepartitionDescription> RepartitionDescription::CreateSPtr(PartitionKind::Enum kind)
{
    switch (kind)
    {
    case PartitionKind::Named:
        return make_shared<NamedRepartitionDescription>();

    default:
        return make_shared<RepartitionDescription>(kind);
    }
}

// FabricSerializable activation
//
NTSTATUS RepartitionDescription::GetTypeInformation(__out Serialization::FabricTypeInformation & typeInformation) const
{
    typeInformation.buffer = reinterpret_cast<UCHAR const *>(&kind_);
    typeInformation.length = sizeof(kind_);
    return STATUS_SUCCESS;
}

Serialization::IFabricSerializable * RepartitionDescription::FabricSerializerActivator(Serialization::FabricTypeInformation typeInformation)
{
    if (typeInformation.buffer == nullptr || typeInformation.length != sizeof(PartitionKind::Enum))
    {
        return nullptr;
    }

    PartitionKind::Enum kind = *(reinterpret_cast<PartitionKind::Enum const *>(typeInformation.buffer));

    switch (kind)
    {
    case PartitionKind::Named:
        return new NamedRepartitionDescription();

    default:
        return new RepartitionDescription(kind);
    }
}

void RepartitionDescription::WriteToEtw(uint16 contextSequenceId) const
{
    this->OnWriteToEtw(contextSequenceId);
}

void RepartitionDescription::OnWriteToEtw(uint16 contextSequenceId) const
{
    ServiceModel::ServiceModelEventSource::Trace->RepartitionDescription(contextSequenceId, kind_);
}

void RepartitionDescription::WriteTo(__in Common::TextWriter& w, Common::FormatOptions const&) const
{
    TRACE_LEVEL_AND_TESTASSERT(Trace.WriteError, TraceComponent, "WriteTo not implemented by derived RepartitionDescription");

    w << "kind=" << kind_;
}
