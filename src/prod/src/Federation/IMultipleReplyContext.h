// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    struct IMultipleReplyContext
    {
        virtual Common::AsyncOperationSPtr BeginReceive(__in Common::TimeSpan timeout, __in Common::AsyncCallback const & callback, __in Common::AsyncOperationSPtr const & parent) = 0;

        // empty message if all replies have been received
        virtual Common::ErrorCode EndReceive(__in Common::AsyncOperationSPtr const & operation, __out Transport::MessageUPtr & message, __out NodeInstance & replySender) = 0;

        // Success or broadcast failed
        virtual void Close() = 0;

        virtual ~IMultipleReplyContext() {};
    };
}
