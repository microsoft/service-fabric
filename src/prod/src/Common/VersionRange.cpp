// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

INITIALIZE_SIZE_ESTIMATION( VersionRange )

void VersionRange::WriteTo(TextWriter& w, FormatOptions const&) const
{
    w.Write("[{0}, {1})", startVersion_, endVersion_);
}

void VersionRange::WriteToEtw(uint16 contextSequenceId) const
{
    CommonEventSource::Events->VersionRange(contextSequenceId, startVersion_, endVersion_);
}
