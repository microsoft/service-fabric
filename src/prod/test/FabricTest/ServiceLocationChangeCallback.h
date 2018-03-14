// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class ServiceLocationChangeCallback
        : public IFabricServicePartitionResolutionChangeHandler
        , private Common::ComUnknownBase
    {
        DENY_COPY(ServiceLocationChangeCallback);

        BEGIN_COM_INTERFACE_LIST(ServiceLocationChangeCallback)
            COM_INTERFACE_ITEM(IID_IUnknown,IFabricServicePartitionResolutionChangeHandler)
            COM_INTERFACE_ITEM(IID_IFabricServicePartitionResolutionChangeHandler,IFabricServicePartitionResolutionChangeHandler)
        END_COM_INTERFACE_LIST()
    public:
    
        ServiceLocationChangeCallback(
            Common::ComPointer<IFabricServiceManagementClient> const & client,
            std::wstring const & uri);

        ServiceLocationChangeCallback(
            Common::ComPointer<IFabricServiceManagementClient> const & client,
            std::wstring const & uri,
            __int64 key);
    
        ServiceLocationChangeCallback(
            Common::ComPointer<IFabricServiceManagementClient> const & client,
            std::wstring const & uri,
            std::wstring const & key);

        HRESULT Register();

        HRESULT Unregister();
    
        bool WaitForCallback(
            Common::TimeSpan const timeout,
            HRESULT expectedError);

        void STDMETHODCALLTYPE OnChange(
            IFabricServiceManagementClient * source,
            LONGLONG handlerId,
            IFabricResolvedServicePartitionResult * partition,
            HRESULT error);

        static Common::ComPointer<ServiceLocationChangeCallback> Create(
            Common::ComPointer<IFabricServiceManagementClient> const & client,
            std::wstring const & uri,
            FABRIC_PARTITION_KEY_TYPE keyType,
            void const * key);

    private:
        void AddChange(
            LONGLONG handlerId,
            IFabricResolvedServicePartitionResult * partition, 
            HRESULT hr);

        void UpdateHandlerIdCallerHoldsLock(LONGLONG handlerId);

        bool TryMatchChange(HRESULT expectedError);

        Common::ComPointer<IFabricServiceManagementClient> client_;
        std::wstring uri_;
        FABRIC_PARTITION_KEY_TYPE keyType_;
        __int64 intKey_;
        std::wstring stringKey_;
        LONGLONG callbackHandler_;
        
        // The callback may be fired multiple times between the wait for callback calls;
        // Remember all values for validation.
        typedef std::pair<Common::ComPointer<IFabricResolvedServicePartitionResult>, HRESULT> ServicePartitionResolutionChange;
        vector<ServicePartitionResolutionChange> changes_;
        
        // Event used to signal waiters that callback was raised
        Common::AutoResetEvent callbackCalledEvent_;
        
        // Lock to protect access to the vector of changes
        Common::ExclusiveLock lock_;
    };

    typedef Common::ComPointer<ServiceLocationChangeCallback> ServiceLocationChangeCallbackCPtr;
}
