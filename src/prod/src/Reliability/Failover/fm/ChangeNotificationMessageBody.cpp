// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Federation;
using namespace Reliability;

void ChangeNotificationMessageBody::WriteTo(Common::TextWriter& w, Common::FormatOptions const& options) const
{
    token_.WriteTo(w, options);
    
    for (NodeInstance const & shutdownNode : shutdownNodes_)
    {
        w.Write(" ");
        shutdownNode.WriteTo(w, options);
    }
}
