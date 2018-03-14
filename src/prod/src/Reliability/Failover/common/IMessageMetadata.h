// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    // A generic container for metadata about messages
    class IMessageMetadata
    {
    public:
        virtual ~IMessageMetadata() {}
    };
}
