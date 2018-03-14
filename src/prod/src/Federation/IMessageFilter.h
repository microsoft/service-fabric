// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    struct IMessageFilter
    {
    public:

        virtual ~IMessageFilter() = 0;

        virtual bool Match(Transport::Message & message) = 0;
    };

    typedef std::shared_ptr<IMessageFilter> IMessageFilterSPtr;
}
