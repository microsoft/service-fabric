// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;

void TraceCorrelatedEventBase::TraceConstantString(uint16 contextSequenceId, StringLiteral const & optionalText)
{
    CommonEventSource::Events->TraceConstantString(contextSequenceId, optionalText);
}
