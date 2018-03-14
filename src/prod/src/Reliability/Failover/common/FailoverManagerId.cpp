// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Common.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;

Global<FailoverManagerId> FailoverManagerId::Fmm = make_global<FailoverManagerId>(true);
Global<FailoverManagerId> FailoverManagerId::Fm = make_global<FailoverManagerId>(false);

FailoverManagerId const & FailoverManagerId::Get(Reliability::FailoverUnitId const & ftId)
{
    return Fmm->IsOwnerOf(ftId) ? *Fmm : *Fm;
}

bool FailoverManagerId::IsOwnerOf(Reliability::FailoverUnitId const & ftId) const
{
    return IsOwnerOf(ftId.Guid);
}

bool FailoverManagerId::IsOwnerOf(Common::Guid const & ftId) const
{
    if (isFmm_)
    {
        return ftId == Reliability::Constants::FMServiceGuid;
    }
    else
    {
        return ftId != Reliability::Constants::FMServiceGuid;
    }
}

std::string FailoverManagerId::AddField(Common::TraceEvent& traceEvent, std::string const& name)
{
    string format = "IsFmm = {0}";
    size_t index = 0;

    traceEvent.AddEventField<bool>(format, name + ".isFmm", index);

    return format;
}

void FailoverManagerId::FillEventData(Common::TraceEventContext& context) const
{
    context.Write(isFmm_);
}

void FailoverManagerId::WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const
{
    w << wformatString("IsFmm = {0}", isFmm_);
}

wstring FailoverManagerId::ToDisplayString() const
{
    return isFmm_ ? L"Fmm" : L"FM";
}
