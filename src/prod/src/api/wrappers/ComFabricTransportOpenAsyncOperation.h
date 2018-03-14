// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    static const GUID CLSID_ComFabricTransportOpenAsyncOperation =
    { 0xd80af29b, 0xfb0, 0x4c68, { 0x9e, 0xfb, 0xb5, 0x41, 0x58, 0x24, 0x44, 0xf6 } };

    class ComFabricTransportOpenAsyncOperation : public  Common::ComAsyncOperationContext
    {
        DENY_COPY(ComFabricTransportOpenAsyncOperation);

        COM_INTERFACE_AND_DELEGATE_LIST(
            ComFabricTransportOpenAsyncOperation,
            CLSID_ComFabricTransportOpenAsyncOperation,
            ComFabricTransportOpenAsyncOperation,
            ComAsyncOperationContext)

    public:
        ComFabricTransportOpenAsyncOperation(__in IServiceCommunicationClientPtr & impl)
            : ComAsyncOperationContext()
            , impl_(impl)
            , timeout_()
        {
        }

        HRESULT Initialize(
            __in DWORD timeoutInMilliSeconds,
            __in IFabricAsyncOperationCallback * callback,
            Common::ComponentRootSPtr const & rootSPtr);

        static HRESULT End(IFabricAsyncOperationContext * context);

    protected:
        virtual void OnStart(Common::AsyncOperationSPtr const & proxySPtr);


    private:

        void FinishRequest(Common::AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously);


        HRESULT ComAsyncOperationContextEnd();

        IServiceCommunicationClientPtr impl_;
        Common::TimeSpan timeout_;
    };
}

