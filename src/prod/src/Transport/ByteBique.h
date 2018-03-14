// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    typedef Common::bique<byte> ByteBique;
    typedef Common::bique<byte>::iterator ByteBiqueIterator;
    typedef Common::bique<byte>::const_iterator ByteBiqueConstIterator;
    typedef Common::BiqueRange<byte> ByteBiqueRange;

    extern ByteBique EmptyByteBique;
    extern const ByteBiqueRange EmptyByteBiqueRange;
}
