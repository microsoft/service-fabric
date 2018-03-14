// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Transport;

void IdempotentHeader::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w << idempotent_;
}
