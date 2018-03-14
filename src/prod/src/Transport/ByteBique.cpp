// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Transport
{
    ByteBique EmptyByteBique;
    const ByteBiqueRange EmptyByteBiqueRange(EmptyByteBique.begin(), EmptyByteBique.end(), false);
}
