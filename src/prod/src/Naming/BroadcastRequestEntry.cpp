// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Naming
{
    using namespace std;
    using namespace Common;

    BroadcastRequestEntry::BroadcastRequestEntry( 
        NamingUri const & name,
        Reliability::ConsistencyUnitId cuid)
        : entry_(name, cuid)
    {
    }

    bool BroadcastRequestEntry::CoversEntry(NamingUri const & name, Reliability::ConsistencyUnitId const cuid) const
    {
        return (this->Name == name) &&
                (this->Cuid == cuid || this->Cuid.IsInvalid);
    }

    void BroadcastRequestEntry::WriteTo(__in Common::TextWriter & writer, Common::FormatOptions const &) const
    {
        if (entry_.second.IsInvalid)
        {
            // Write the name of the service only
            writer << "'" << entry_.first.Name << "' ";
        }
        else
        {
            // Write the CUID only
            writer << entry_.second.Guid << " ";
        }
    }

    void BroadcastRequestEntry::WriteToEtw(uint16 contextSequenceId) const
    {
        if (entry_.second.IsInvalid)
        {
            // Write the name of the service only
            NamingCommonEventSource::EventSource->NamingUriTrace(
                contextSequenceId, 
                entry_.first.Name);
        }
        else
        {
            // Write the CUID only
            NamingCommonEventSource::EventSource->CuidTrace(
                contextSequenceId, 
                entry_.second.Guid);
        }           
    }
}
