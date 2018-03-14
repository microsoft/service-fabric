// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    class ComProxySecondaryEventHandler :
        public Common::ComponentRoot,
        public ISecondaryEventHandler
    {
        DENY_COPY(ComProxySecondaryEventHandler );

    public:
        ComProxySecondaryEventHandler(Common::ComPointer<IFabricSecondaryEventHandler> const & comImpl);
        virtual ~ComProxySecondaryEventHandler();

        //
        // ISecondaryEventHandler methods
        //
        Common::ErrorCode OnCopyComplete(IStoreEnumeratorPtr const &);

        Common::ErrorCode OnReplicationOperation(IStoreNotificationEnumeratorPtr const &);
            
    private:
        Common::ComPointer<IFabricSecondaryEventHandler> const comImpl_;
    };
}
