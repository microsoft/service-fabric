// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Hosting2
{
    class DummyStatelessServiceFactory : public IFabricStatelessServiceFactory, public Common::ComUnknownBase
    {
        COM_INTERFACE_LIST1(
            StatelessServiceFactory,
            IID_IFabricStatelessServiceFactory,
            IFabricStatelessServiceFactory)

        public:
            DummyStatelessServiceFactory(std::wstring supportedServiceType);
            virtual ~DummyStatelessServiceFactory();

            virtual HRESULT STDMETHODCALLTYPE CreateInstance( 
                /* [in] */ LPCWSTR serviceType,
                /* [in] */ FABRIC_URI serviceName,
                /* [in] */ ULONG initializationDataLength,
                /* [size_is][in] */ const byte *initializationData,
                /* [in] */ FABRIC_PARTITION_ID partitionId,
                /* [in] */ FABRIC_INSTANCE_ID instanceId,
                /* [retval][out] */ IFabricStatelessServiceInstance **serviceInstance);

            std::wstring const & get_SupportedServiceType() { return supportedServiceType_; }

        private:
            std::wstring supportedServiceType_;
    };
}
