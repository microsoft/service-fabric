// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Transport;

MessageIdHeader::MessageIdHeader()
{
}

MessageIdHeader::MessageIdHeader(Transport::MessageId const & messageId) : messageId_(messageId)
{
}

Transport::MessageId const & MessageIdHeader::get_MessageId() const
{
    return messageId_;
}

void MessageIdHeader::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const 
{ 
    w << messageId_;
}
