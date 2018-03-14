// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

ConfigureCrashDumpRequest::ConfigureCrashDumpRequest() 
    : enable_(),
    nodeId_(),
    servicePackageId_(),
    exeNames_()
{
}

ConfigureCrashDumpRequest::ConfigureCrashDumpRequest(
    bool enable,
    wstring const & nodeId,
    wstring const & servicePackageId,
    vector<wstring> const & exeNames)
    : enable_(enable),
    nodeId_(nodeId),
    servicePackageId_(servicePackageId),
    exeNames_(exeNames)
{
}

void ConfigureCrashDumpRequest::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ConfigureCrashDumpRequest { ");
    w.Write("Enable = {0}", enable_);
    w.Write("NodeId = {0}", nodeId_);
    w.Write("ServicePackageId = {0}", servicePackageId_);
    w.Write("ExeNames = {0}", exeNames_);
    w.Write("}");
}
