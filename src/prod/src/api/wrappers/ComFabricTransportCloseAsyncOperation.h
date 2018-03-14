// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {7250081A-B694-4DF5-BA77-F03A59B38003}
    static const GUID CLSID_ComFabricTransportCloseAsyncOperation =
    { 0x7250081a, 0xb694, 0x4df5,{ 0xba, 0x77, 0xf0, 0x3a, 0x59, 0xb3, 0x80, 0x3 } };

    class ComFabricTransportCloseAsyncOperation : public  Common::ComAsyncOperationContext
    {
        DENY_COPY(ComFabricTransportCloseAsyncOperation);

        COM_INTERFACE_AND_DELEGATE_LIST(
            ComFabricTransportCloseAsyncOperation,
            CLSID_ComFabricTransportCloseAsyncOperation,
            ComFabricTransportCloseAsyncOperation,
            ComAsyncOperationContext)

    public:
        ComFabricTransportCloseAsyncOperation(__in IServiceCommunicationClientPtr & impl)
            : ComAsyncOperationContext()
            , impl_(impl)
            , timeout_()
        {
        }

        HRESULT Initialize(
            __in DWORD timeoutInMilliSeconds,
            __in IFabricAsyncOperationCallback * callback,
            Common::ComponentRootSPtr const & rootSPtr);

        static HRESULT End(__in IFabricAsyncOperationContext * context);

    protected:
        virtual void OnStart(Common::AsyncOperationSPtr const & proxySPtr);


    private:

        void FinishRequest(Common::AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously);


        HRESULT ComAsyncOperationContextEnd();

        IServiceCommunicationClientPtr impl_;
        Common::TimeSpan timeout_;
    };
}

