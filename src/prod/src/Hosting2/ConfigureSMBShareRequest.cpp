// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

ConfigureSMBShareRequest::ConfigureSMBShareRequest()
    : sids_()
    , accessMask_()
    , localFullPath_()
    , shareName_()
    , ticks_()
{
}

ConfigureSMBShareRequest::ConfigureSMBShareRequest(
    std::vector<std::wstring> const & sids,
    DWORD accessMask,
    std::wstring const & localFullPath,
    std::wstring const & shareName,
    int64 timeoutTicks)
    : sids_(sids)
    , accessMask_(accessMask)
    , localFullPath_(localFullPath)
    , shareName_(shareName)
    , ticks_(timeoutTicks)
{
}

void ConfigureSMBShareRequest::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ConfigureSMBShareRequest { ");
    w.Write("Sids {");

    for(auto iter = sids_.begin(); iter != sids_.end(); iter++)
    {
        w.Write("{0}, ", *iter);
    }

    w.Write("}, ");
    w.Write("AccessMark {0}, ", accessMask_);
    w.Write("LocalFullPath {0}, ", localFullPath_);
    w.Write("ShareName {0}, ", shareName_);
    w.Write("TimeOut ticks {0}", ticks_);
    w.Write("}");
}
