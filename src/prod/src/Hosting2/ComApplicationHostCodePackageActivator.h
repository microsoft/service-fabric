// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    // {25B34E6A-A95C-4F77-A2C3-220AB2612952}
    static const GUID CLSID_ComApplicationHostCodePackageActivator =
        { 0x25b34e6a, 0xa95c, 0x4f77, { 0xa2, 0xc3, 0x22, 0xa, 0xb2, 0x61, 0x29, 0x52 } };

    class ComApplicationHostCodePackageActivator :
        public ITentativeApplicationHostCodePackageActivator,
        private Common::ComUnknownBase
    {
        DENY_COPY(ComApplicationHostCodePackageActivator);

        BEGIN_COM_INTERFACE_LIST(ComApplicationHostCodePackageActivator)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricCodePackageActivator)
            COM_INTERFACE_ITEM(IID_IFabricCodePackageActivator, IFabricCodePackageActivator)
            COM_INTERFACE_ITEM(CLSID_ComApplicationHostCodePackageActivator, ComApplicationHostCodePackageActivator)
        END_COM_INTERFACE_LIST()

    public:
        ComApplicationHostCodePackageActivator(ApplicationHostCodePackageActivatorSPtr const & activatorImpl);
        virtual ~ComApplicationHostCodePackageActivator();

        ULONG STDMETHODCALLTYPE TryAddRef() override;

        HRESULT STDMETHODCALLTYPE BeginActivateCodePackage(
            /* [in] */ FABRIC_STRING_LIST *codePackageNames,
            /* [in] */ FABRIC_STRING_MAP *environment,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) override;

        HRESULT STDMETHODCALLTYPE EndActivateCodePackage(
            /* [in] */ IFabricAsyncOperationContext *context) override;

        HRESULT STDMETHODCALLTYPE BeginDeactivateCodePackage(
            /* [in] */ FABRIC_STRING_LIST *codePackageNames,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) override;

        HRESULT STDMETHODCALLTYPE EndDeactivateCodePackage(
            /* [in] */ IFabricAsyncOperationContext *context) override;

        HRESULT STDMETHODCALLTYPE AbortCodePackage(
            /* [in] */ FABRIC_STRING_LIST *codePackageNames) override;

        HRESULT STDMETHODCALLTYPE RegisterCodePackageEventHandler(
            /* [in] */ IFabricCodePackageEventHandler *eventHandler,
            /* [retval][out] */ ULONGLONG *callbackHandle) override;

        HRESULT STDMETHODCALLTYPE UnregisterCodePackageEventHandler(
            /* [in] */ ULONGLONG callbackHandle) override;

    private:

        Common::TimeSpan GetTimeSpan(DWORD timeMilliSec);

    private:
        class ActivateCodePackageComAsyncOperationContext;
        class DeactivateCodePackageComAsyncOperationContext;
        class RegisterCodePackageEventHandlerComAsyncOperationContext;

    private:
        std::wstring objectId_;
        ApplicationHostCodePackageActivatorSPtr const activatorImpl_;
    };
}
