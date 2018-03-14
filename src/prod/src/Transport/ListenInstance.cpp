// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;

ListenInstance::ListenInstance() : instance_(0)
{
}

ListenInstance::ListenInstance(wstring const & address, uint64 instance, Guid const & nonce)
    : address_(address), instance_(instance), nonce_(nonce)
{
}

void ListenInstance::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{ 
    w.Write("{0}/{1}/{2}", address_, instance_, nonce_);
}

string ListenInstance::AddField(Common::TraceEvent & traceEvent, std::string const & name)
{
    traceEvent.AddField<wstring>(name + ".address");
    traceEvent.AddField<uint64>(name + ".instance");
    traceEvent.AddField<Guid>(name + ".nonce");

    return "{0}/{1}/{2}";
}

void ListenInstance::FillEventData(Common::TraceEventContext & context) const
{
    context.Write<wstring>(address_);
    context.Write<uint64>(instance_);
    context.Write<Guid>(nonce_);
}
