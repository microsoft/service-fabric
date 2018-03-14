// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Common.Stdafx.h"

using namespace Common;
using namespace Reliability;

Reliability::PlbMovementIgnoredReasons::PlbMovementIgnoredReasons() : reason_(Invalid)
{
}

Reliability::PlbMovementIgnoredReasons::PlbMovementIgnoredReasons(Enum reason) : reason_(reason)
{
}

Reliability::PlbMovementIgnoredReasons::PlbMovementIgnoredReasons(const PlbMovementIgnoredReasons & other) : reason_(other.Reason)
{
}


Reliability::PlbMovementIgnoredReasons & Reliability::PlbMovementIgnoredReasons::operator=(PlbMovementIgnoredReasons const & other)
{
    reason_ = other.Reason;
    return *this;
}

std::wstring Reliability::PlbMovementIgnoredReasons::ToString(Enum const & val) const
{
    switch (val)
    {
    case Invalid:
        return L"None";
    case ClusterPaused:
        return L"Cluster paused";
    case DropAllPLBMovementsConfigTrue:
        return L"DropAllPLBMovements configuration set to True";
    case FailoverUnitNotFound:
        return L"FailoverUnit not found";
    case FailoverUnitIsToBeDeleted:
        return L"FailoverUnit to be deleted";
    case VersionMismatch:
        return L"Version mismatch";
    case FailoverUnitIsChanging:
        return L"FailoverUnit is changing";
    case NodePendingClose:
        return L"Node pending close";
    case NodeIsUploadingReplicas:
        return L"Node is uploading replicas";
    case ReplicaNotDeleted:
        return L"Replica not deleted";
    case NodePendingDeactivate:
        return L"Node pending deactivate";
    case InvalidReplicaState:
        return L"Invalid replica state";
    case CurrentConfigurationEmpty:
        return L"Current configuration is empty";
    case ReplicaConfigurationChanging:
        return L"Replica configuration is changing";
    case ToBePromotedReplicaAlreadyExists:
        return L"To be promoted replica already exists";
    case ReplicaToBeDropped:
        return L"Replica to be dropped";
    case ReplicaNotFound:
        return L"Replica not found";
    case PrimaryNotFound:
        return L"Primary not found";
    default:
        return L"PlbMovementIgnoredReasons(" + static_cast<uint>(val) + ')';
    }
}

std::string Reliability::PlbMovementIgnoredReasons::AddField(Common::TraceEvent & traceEvent, std::string const & name)
{
    return traceEvent.AddMapField(name, "PlbMovementIgnoredReasons");
}


void Reliability::PlbMovementIgnoredReasons::FillEventData(Common::TraceEventContext & context) const
{
    context.WriteCopy<uint>(static_cast<uint>(reason_));
}

void Reliability::PlbMovementIgnoredReasons::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w << ToString(this->reason_);
}

Common::Global<PlbMovementIgnoredReasons::Initializer> PlbMovementIgnoredReasons::GlobalInitializer = Common::make_global<PlbMovementIgnoredReasons::Initializer>();
PlbMovementIgnoredReasons::Initializer::Initializer()
{

    for (int i = PlbMovementIgnoredReasons::Invalid; i != PlbMovementIgnoredReasons::LastValidEnum; i++)
    {
        PlbMovementIgnoredReasons e = static_cast<PlbMovementIgnoredReasons::Enum>(i);
        std::string tmp;
        Common::StringWriterA(tmp).Write("{0}", e);
        Common::TraceProvider::StaticInit_Singleton()->AddMapValue("PlbMovementIgnoredReasons", i, tmp);
    }
}
