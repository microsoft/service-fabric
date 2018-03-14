// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

RuntimeRegistration::RuntimeRegistration(
    FabricRuntimeContext const & runtimeContext)
    : runtimeContext_(runtimeContext)
{
}

void RuntimeRegistration::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("RuntimeRegistration { ");
    w.Write("RuntimeContext={0}", RuntimeContext);
    w.Write("}");
}
