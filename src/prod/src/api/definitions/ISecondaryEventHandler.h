// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class ISecondaryEventHandler
    {
    public:
        virtual ~ISecondaryEventHandler() {};

        virtual Common::ErrorCode OnCopyComplete(IStoreEnumeratorPtr const &) = 0;

        virtual Common::ErrorCode OnReplicationOperation(IStoreNotificationEnumeratorPtr const &) = 0;
    };
}
