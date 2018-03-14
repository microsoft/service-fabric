// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace FabricTest
{
	// {F082BCAB-F285-4C66-8AD1-1C3357C333CF}
	static const GUID CLSID_ComTestPostBackupHandlerContext =
	{ 0xf082bcab, 0xf285, 0x4c66, { 0x8a, 0xd1, 0x1c, 0x33, 0x57, 0xc3, 0x33, 0xcf } };

	class ComTestPostBackupHandler::ComTestPostBackupHandlerContext : public Common::ComAsyncOperationContext
	{
		DENY_COPY(ComTestPostBackupHandlerContext);

		COM_INTERFACE_AND_DELEGATE_LIST(
			ComTestPostBackupHandlerContext,
			CLSID_ComTestPostBackupHandlerContext,
			ComTestPostBackupHandlerContext,
			ComAsyncOperationContext)

	public:
		ComTestPostBackupHandlerContext(
			__in FABRIC_STORE_BACKUP_INFO * info,
			__in IFabricAsyncOperationCallback * parentCallback,
			__in bool postBackupReturnValue,
			__out IFabricAsyncOperationContext ** context)
			: info_(*info)	
			, parentCallback_(parentCallback)
			, postBackupReturnValue_(postBackupReturnValue)
		{
			HRESULT hr = this->QueryInterface(
				CLSID_ComTestPostBackupHandlerContext,
				reinterpret_cast<void**>(context));
			CODING_ERROR_ASSERT(SUCCEEDED(hr));
		}

		virtual ~ComTestPostBackupHandlerContext() { }

		HRESULT STDMETHODCALLTYPE End(__out BOOLEAN * status)
		{
			HRESULT hr = Common::ComAsyncOperationContext::End();

			*status = SUCCEEDED(hr)
				? postBackupReturnValue_ ? TRUE : FALSE
				: FALSE;
			
			return hr;
		}

	protected:
		virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
		{
			// do some lengthy operation here such as copying backup folder to Azure blob etc.

			TryComplete(proxySPtr, Common::ErrorCode::Success());

			if (!parentCallback_)
			{
				return;
			}

			Common::ComPointer<IFabricAsyncOperationContext> context;

			HRESULT hr = this->QueryInterface(
				CLSID_ComTestPostBackupHandlerContext,
				reinterpret_cast<void**>(context.InitializationAddress()));
			CODING_ERROR_ASSERT(SUCCEEDED(hr));

			parentCallback_->Invoke(context.GetRawPointer());
		}

	private:
		FABRIC_STORE_BACKUP_INFO & info_;		
		IFabricAsyncOperationCallback * parentCallback_;
		bool postBackupReturnValue_;
	};

	HRESULT ComTestPostBackupHandler::BeginPostBackup(
		__in FABRIC_STORE_BACKUP_INFO * info,
		__in IFabricAsyncOperationCallback * callback,
		__out IFabricAsyncOperationContext ** context)
	{		
		Common::ComAsyncOperationContextCPtr childContext;
		childContext.SetNoAddRef(
			new ComTestPostBackupHandlerContext(info, callback, postBackupReturnValue_, context));

		ComponentRootSPtr rootCPtr;

		HRESULT hr = childContext->Initialize(rootCPtr, nullptr);

		if (FAILED(hr)) { return hr; }

		Common::ComAsyncOperationContextCPtr ptr(childContext, IID_IFabricAsyncOperationContext);
		hr = childContext->Start(ptr);

		return hr;
	}

	HRESULT ComTestPostBackupHandler::EndPostBackup(IFabricAsyncOperationContext * context, __out BOOLEAN * status)
	{
		Common::ComPointer<ComTestPostBackupHandlerContext> pContext(context, CLSID_ComTestPostBackupHandlerContext);

		HRESULT hr = pContext->End(status);
		return hr;		
	}
}
