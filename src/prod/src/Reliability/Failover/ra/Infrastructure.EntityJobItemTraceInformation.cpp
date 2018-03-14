// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;
using namespace Infrastructure;
using namespace std;

namespace
{
    static const wstring Empty;
}

EntityJobItemTraceInformation::EntityJobItemTraceInformation(
    std::wstring const * activityId,
    std::wstring const * traceMetadata,
    JobItemName::Enum name) :
    activityId_(activityId),
    traceMetadata_(traceMetadata),
    name_(name)
{
}

EntityJobItemTraceInformation EntityJobItemTraceInformation::CreateWithName(
    std::wstring const & activityId,
    JobItemName::Enum name)
{
    return EntityJobItemTraceInformation(&activityId, &Empty, name);
}

EntityJobItemTraceInformation EntityJobItemTraceInformation::CreateWithMetadata(
    std::wstring const & activityId,
    std::wstring const & traceMetadata)
{
    return EntityJobItemTraceInformation(&activityId, &traceMetadata, JobItemName::Empty);
}

void EntityJobItemTraceInformation::WriteTo(Common::TextWriter & w, Common::FormatOptions const & ) const
{
    w << wformatString("{0}{1} {2}\r\n", *traceMetadata_, name_, *activityId_);
}

void EntityJobItemTraceInformation::WriteToEtw(uint16 contextSequenceId) const
{
    RAEventSource::Events->EntityJobItemTraceInformation(contextSequenceId, *traceMetadata_, name_, *activityId_);
}
