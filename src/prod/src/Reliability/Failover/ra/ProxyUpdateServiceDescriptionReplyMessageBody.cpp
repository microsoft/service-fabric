// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace Common;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace Infrastructure;

void ProxyUpdateServiceDescriptionReplyMessageBody::WriteTo(Common::TextWriter& w, Common::FormatOptions const& options) const
{
    fuDesc_.WriteTo(w, options);
    w.WriteLine();

    serviceDescription_.WriteTo(w, options);
    w.WriteLine();

    localReplica_.WriteTo(w, options);

    w.Write("EC: {0}", errorCode_);
}

void ProxyUpdateServiceDescriptionReplyMessageBody::WriteToEtw(uint16 contextSequenceId) const
{
    RAEventSource::Events->ProxyUpdateServiceDescriptionReplyMessageBody(
        contextSequenceId,
        fuDesc_,
        serviceDescription_,
        localReplica_,
        wformatString(errorCode_));
}
