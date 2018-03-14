// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    class ComResolvedServicePartitionResult
        : public IFabricResolvedServicePartitionResult,
          public IInternalFabricResolvedServicePartition,
          private Common::ComUnknownBase
    {
        DENY_COPY_ASSIGNMENT(ComResolvedServicePartitionResult);

        BEGIN_COM_INTERFACE_LIST(ComResolvedServicePartitionResult)
            COM_INTERFACE_ITEM(IID_IUnknown,IFabricResolvedServicePartitionResult)
            COM_INTERFACE_ITEM(IID_IFabricResolvedServicePartitionResult,IFabricResolvedServicePartitionResult)
            COM_INTERFACE_ITEM(IID_IInternalFabricResolvedServicePartition,IInternalFabricResolvedServicePartition)
        END_COM_INTERFACE_LIST()
        
    public:
        ComResolvedServicePartitionResult(
            IResolvedServicePartitionResultPtr const & impl);

        ComResolvedServicePartitionResult();

        IResolvedServicePartitionResultPtr const& get_Impl() const { return impl_; }

        const FABRIC_RESOLVED_SERVICE_PARTITION * STDMETHODCALLTYPE get_Partition();

        HRESULT STDMETHODCALLTYPE GetEndpoint(
            __out /* [out, retval] */ const FABRIC_RESOLVED_SERVICE_ENDPOINT ** endpoint);

        HRESULT STDMETHODCALLTYPE CompareVersion(
            __in /* [in] */ IFabricResolvedServicePartitionResult * other,
            __out /* [out, retval] */ LONG * compareResult);

        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_FMVersion( 
            /* [out] */ LONGLONG *value);
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Generation( 
            /* [out] */ LONGLONG *value);

        __declspec(property(get=get_NativePartition)) IResolvedServicePartitionResultPtr const & NativePartition;
        IResolvedServicePartitionResultPtr const & get_NativePartition() const { return impl_; }

    private:

        IResolvedServicePartitionResultPtr impl_;
        Common::ScopedHeap heap_;
        Common::ReferencePointer<FABRIC_RESOLVED_SERVICE_PARTITION> partition_;
        Common::ReferencePointer<FABRIC_RESOLVED_SERVICE_ENDPOINT> mainEndpoint_;
        Common::RwLock lock_;
    };
}
