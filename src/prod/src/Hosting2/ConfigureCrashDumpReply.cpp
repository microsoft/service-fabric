// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

using namespace Hosting2;

ConfigureCrashDumpReply::ConfigureCrashDumpReply() 
    : error_(ErrorCodeValue::Success)
{
}

ConfigureCrashDumpReply::ConfigureCrashDumpReply(
    ErrorCode error)
    : error_(error)
{
}

void ConfigureCrashDumpReply::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ConfigureCrashDumpReply { ");
    w.Write("ErrorCode = {0}", error_);
    w.Write("}");
}
