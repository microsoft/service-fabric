// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "service.h"

namespace NativeServiceGroupMember
{

	class ServiceImplementation : 
		public ISimpleAdder,
		public IFabricStatelessServiceInstance,
		private ComUnknownBase
	{
		DENY_COPY(ServiceImplementation)

		BEGIN_COM_INTERFACE_LIST(ServiceImplementation)
			COM_INTERFACE_ITEM(IID_IUnknown, ISimpleAdder)
			COM_INTERFACE_ITEM(IID_ISimpleAdder, ISimpleAdder)
			COM_INTERFACE_ITEM(IID_IFabricStatelessServiceInstance, IFabricStatelessServiceInstance)
		END_COM_INTERFACE_LIST()

	public:

		ServiceImplementation(wstring const & partnerUri);
		~ServiceImplementation();

		// IFabricStatelessServiceInstance
		STDMETHODIMP BeginOpen( 
            __in IFabricStatelessServicePartition *partition,
            __in IFabricAsyncOperationCallback *callback,
            __out IFabricAsyncOperationContext **context);
        
        STDMETHODIMP EndOpen( 
            __in IFabricAsyncOperationContext *context,
            __out IFabricStringResult **serviceAddress);
        
        STDMETHODIMP BeginClose( 
            __in IFabricAsyncOperationCallback *callback,
            __out IFabricAsyncOperationContext **context);
        
        STDMETHODIMP EndClose( 
            __in IFabricAsyncOperationContext *context);

        STDMETHODIMP_(void) Abort();

		// ISimpleAdder
		STDMETHODIMP Add( 
            __in LONG first,
            __in LONG second,
            __out LONG *result);

	private:

		wstring partnerUri_;
		ComPointer<IFabricServiceGroupPartition> serviceGroupPartition_;

		template <class ComImplementation, class T0>
		friend Common::ComPointer<ComImplementation> Common::make_com(T0 && a0);
	};
}
