// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
	Common::StringLiteral const TraceComponent("ComTestPostBackupHandler");

	// {2B2E938E-F07D-49A2-BEAC-4E466A34BCB7}
	static const GUID CLSID_ComTestPostBackupHandler =
	{ 0x2b2e938e, 0xf07d, 0x49a2, { 0xbe, 0xac, 0x4e, 0x46, 0x6a, 0x34, 0xbc, 0xb7 } };

	/// <summary>
	/// A COM class that customers using the key-value store will implement in order to enable
	/// incremental backups
	/// </summary>
	class ComTestPostBackupHandler
		: public IFabricStorePostBackupHandler
		, private Common::ComUnknownBase
	{
		DENY_COPY(ComTestPostBackupHandler)
				
		BEGIN_COM_INTERFACE_LIST(ComTestPostBackupHandler)		
		COM_INTERFACE_ITEM(IID_IFabricStorePostBackupHandler, IFabricStorePostBackupHandler)
		END_COM_INTERFACE_LIST()

	private:
		class ComTestPostBackupHandlerContext;

	public:
		ComTestPostBackupHandler(bool postBackupReturnValue = true) 
			: postBackupReturnValue_(postBackupReturnValue)
		{ 
		}
		
		virtual ~ComTestPostBackupHandler() { }

		virtual HRESULT BeginPostBackup(
			__in FABRIC_STORE_BACKUP_INFO * info,
			__in IFabricAsyncOperationCallback * callback,
			__out IFabricAsyncOperationContext ** context);

		virtual HRESULT EndPostBackup(__in IFabricAsyncOperationContext * context, __out BOOLEAN * status);

	private:

		/// <summary>
		/// The return value to be used as a simulation from the post backup handler. This return value
		/// determines if incremental backup is allowed the next time.
		/// </summary>
		bool postBackupReturnValue_;
	};
}
