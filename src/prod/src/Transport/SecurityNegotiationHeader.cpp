// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Transport;
using namespace Common;

SecurityNegotiationHeader::SecurityNegotiationHeader()
#ifdef PLATFORM_UNIX
    : framingProtectionEnabled_ (true)
#else
    : framingProtectionEnabled_(false)
#endif
{
}

SecurityNegotiationHeader::SecurityNegotiationHeader(
    bool framingProtectionEnabled,
    ListenInstance const & listenInstance,
    size_t maxIncomingFrameSize)
    : framingProtectionEnabled_(framingProtectionEnabled)
    , listenInstance_(listenInstance)
    , maxIncomingFrameSize_(maxIncomingFrameSize)
{
}

bool SecurityNegotiationHeader::FramingProtectionEnabled() const
{
    return framingProtectionEnabled_;
}

bool SecurityNegotiationHeader::operator==(SecurityNegotiationHeader const & other) const
{
    return
        (x509ExtraFramingEnabled_ == other.x509ExtraFramingEnabled_) &&
        (framingProtectionEnabled_ == other.framingProtectionEnabled_) &&
        (listenInstance_ == other.listenInstance_) &&
        (maxIncomingFrameSize_ == other.maxIncomingFrameSize_);
}

void SecurityNegotiationHeader::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write(
        "framingProtectionEnabled_={0}, listenInstance_={1}, maxIncomingFrameSize_={2}/0x{2:x}",
        framingProtectionEnabled_, listenInstance_, maxIncomingFrameSize_);
}
