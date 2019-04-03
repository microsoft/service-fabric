// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include "Common/Common.h"
#include "ServiceGroup.Runtime.h"
#include "ServiceGroup.InternalPublic.h"


#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace ServiceGroupTests
{
    using namespace std;
    using namespace Common;
    using namespace ServiceGroup;
    using namespace ServiceModel;

    class DummyStatefulService : public IFabricStatefulServiceReplica,
                                 private ComUnknownBase
    {
        DENY_COPY(DummyStatefulService)

        BEGIN_COM_INTERFACE_LIST(DummyStatefulService)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricStatefulServiceReplica)
            COM_INTERFACE_ITEM(IID_IFabricStatefulServiceReplica, IFabricStatefulServiceReplica)
        END_COM_INTERFACE_LIST()
        
    public:
        DummyStatefulService()
        {
        }

        ~DummyStatefulService()
        {
        }

        HRESULT STDMETHODCALLTYPE BeginOpen( 
            __in FABRIC_REPLICA_OPEN_MODE openMode,
            __in IFabricStatefulServicePartition *partition,
            __in IFabricAsyncOperationCallback *callback,
            __out IFabricAsyncOperationContext **context) 
        {
             UNREFERENCED_PARAMETER(openMode);
             UNREFERENCED_PARAMETER(partition);
             UNREFERENCED_PARAMETER(callback);
             UNREFERENCED_PARAMETER(context);
             return E_NOTIMPL;
        }

        HRESULT STDMETHODCALLTYPE EndOpen( 
            __in IFabricAsyncOperationContext *context,
            __out IFabricReplicator **replicator
            ) 
        {
             UNREFERENCED_PARAMETER(context);
             UNREFERENCED_PARAMETER(replicator);
             return E_NOTIMPL;
        }

        HRESULT STDMETHODCALLTYPE BeginChangeRole( 
            __in FABRIC_REPLICA_ROLE newRole,
            __in IFabricAsyncOperationCallback *callback,
            __out IFabricAsyncOperationContext **context) 
        {
             UNREFERENCED_PARAMETER(newRole);
             UNREFERENCED_PARAMETER(callback);
             UNREFERENCED_PARAMETER(context);
             return E_NOTIMPL;
        }
        
        HRESULT STDMETHODCALLTYPE EndChangeRole( 
            __in IFabricAsyncOperationContext *context,
            __out IFabricStringResult **serviceAddress)
        {
            UNREFERENCED_PARAMETER(context);
            UNREFERENCED_PARAMETER(serviceAddress);
            return E_NOTIMPL;
        }

        HRESULT STDMETHODCALLTYPE BeginClose( 
            __in IFabricAsyncOperationCallback *callback,
            __out IFabricAsyncOperationContext **context) 
        {
             UNREFERENCED_PARAMETER(callback);
             UNREFERENCED_PARAMETER(context);
             return E_NOTIMPL;
        }

        HRESULT STDMETHODCALLTYPE EndClose( 
            __in IFabricAsyncOperationContext *context) 
        {
             UNREFERENCED_PARAMETER(context);
             return E_NOTIMPL;
        }

        void STDMETHODCALLTYPE Abort( void) 
        {
        }
    };

    class DummyStatelessService : public IFabricStatelessServiceInstance,
                                  private ComUnknownBase
    {
        DENY_COPY(DummyStatelessService)

        BEGIN_COM_INTERFACE_LIST(DummyStatelessService)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricStatelessServiceInstance)
            COM_INTERFACE_ITEM(IID_IFabricStatelessServiceInstance, IFabricStatelessServiceInstance)
        END_COM_INTERFACE_LIST()
        
    public:

        DummyStatelessService()
        {
        }

        ~DummyStatelessService()
        {
        }

        HRESULT STDMETHODCALLTYPE BeginOpen( 
            __in IFabricStatelessServicePartition *partition,
            __in IFabricAsyncOperationCallback *callback,
            __out IFabricAsyncOperationContext **context) 
        {
             UNREFERENCED_PARAMETER(partition);
             UNREFERENCED_PARAMETER(callback);
             UNREFERENCED_PARAMETER(context);
             return E_NOTIMPL;
        }

        HRESULT STDMETHODCALLTYPE EndOpen( 
            __in IFabricAsyncOperationContext *context,
            __out IFabricStringResult **serviceAddress) 
        {
             UNREFERENCED_PARAMETER(context);
             UNREFERENCED_PARAMETER(serviceAddress);
             return E_NOTIMPL;
        }

        HRESULT STDMETHODCALLTYPE BeginClose( 
            __in IFabricAsyncOperationCallback *callback,
            __out IFabricAsyncOperationContext **context) 
        {
             UNREFERENCED_PARAMETER(callback);
             UNREFERENCED_PARAMETER(context);
             return E_NOTIMPL;
        }

        HRESULT STDMETHODCALLTYPE EndClose( 
            __in IFabricAsyncOperationContext *context) 
        {
             UNREFERENCED_PARAMETER(context);
             return E_NOTIMPL;
        }

        void STDMETHODCALLTYPE Abort( void) 
        {
        }
    };

    class DummyFactory : public IFabricStatelessServiceFactory,
                         public IFabricStatefulServiceFactory,
                         private ComUnknownBase
    {
        DENY_COPY(DummyFactory)

        BEGIN_COM_INTERFACE_LIST(DummyFactory)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricStatelessServiceFactory)
            COM_INTERFACE_ITEM(IID_IFabricStatelessServiceFactory, IFabricStatelessServiceFactory)
            COM_INTERFACE_ITEM(IID_IFabricStatefulServiceFactory, IFabricStatefulServiceFactory)
        END_COM_INTERFACE_LIST()
        
    public:
        DummyFactory()
        {
        }

        ~DummyFactory()
        {
        }

        //
        // IFabricStatelessServiceFactory members
        //
         HRESULT STDMETHODCALLTYPE CreateInstance( 
            __in LPCWSTR serviceType,
            __in FABRIC_URI serviceName,
            __in ULONG initializationDataLength,
            __in const byte *initializationData,
            __in FABRIC_PARTITION_ID partitionId,
            __in FABRIC_INSTANCE_ID instanceId,
            __out IFabricStatelessServiceInstance **serviceInstance)
         {
             UNREFERENCED_PARAMETER(serviceName);
             UNREFERENCED_PARAMETER(initializationDataLength);
             UNREFERENCED_PARAMETER(initializationData);
             UNREFERENCED_PARAMETER(partitionId);
             UNREFERENCED_PARAMETER(instanceId);

             if (wstring(serviceType) == wstring(L"E_FAIL"))
             {
                 return E_FAIL;
             }
             
             ComPointer<IFabricStatelessServiceInstance> instance = make_com<DummyStatelessService, IFabricStatelessServiceInstance>();
             *serviceInstance = instance.DetachNoRelease();

             return S_OK;
         }

         //
         // IFabricStatefulServiceFactory members
         //
         HRESULT STDMETHODCALLTYPE CreateReplica( 
            __in LPCWSTR serviceType,
            __in FABRIC_URI serviceName,
            __in ULONG initializationDataLength,
            __in const byte *initializationData,
            __in FABRIC_PARTITION_ID partitionId,
            __in FABRIC_REPLICA_ID replicaId,
            __out IFabricStatefulServiceReplica **serviceReplica)
         {
             UNREFERENCED_PARAMETER(serviceName);
             UNREFERENCED_PARAMETER(initializationDataLength);
             UNREFERENCED_PARAMETER(initializationData);
             UNREFERENCED_PARAMETER(partitionId);
             UNREFERENCED_PARAMETER(replicaId);

             if (wstring(serviceType) == wstring(L"E_FAIL"))
             {
                 return E_FAIL;
             }
             
             ComPointer<IFabricStatefulServiceReplica> replica = make_com<DummyStatefulService, IFabricStatefulServiceReplica>();
             *serviceReplica = replica.DetachNoRelease();

             return S_OK;
         }
    };

    class ServiceGroupFactoryTest
    {
    };

    BOOST_FIXTURE_TEST_SUITE(ServiceGroupFactoryTestSuite,ServiceGroupFactoryTest)

    BOOST_AUTO_TEST_CASE(CreateStatelessFactory)
    {
        ComPointer<IFabricStatelessServiceFactory> innerStatelessFactory = make_com<DummyFactory, IFabricStatelessServiceFactory>();

        ComPointer<IFabricServiceGroupFactoryBuilder> builder;

        //
        // CServiceGroupFactoryBuilder does not friend make_com(T&)
        //
        CServiceGroupFactoryBuilder* builderRaw = new CServiceGroupFactoryBuilder(nullptr);
        HRESULT hr = builderRaw->QueryInterface(IID_IFabricServiceGroupFactoryBuilder, builder.VoidInitializationAddress());
        builderRaw->Release();
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        hr = builder->AddStatelessServiceFactory(L"Type1", innerStatelessFactory.GetRawPointer());
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        hr = builder->AddStatelessServiceFactory(L"Type2", innerStatelessFactory.GetRawPointer());
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        ComPointer<IFabricServiceGroupFactory> groupFactory;
        hr = builder->ToServiceGroupFactory(groupFactory.InitializationAddress());
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        ComPointer<IFabricStatelessServiceFactory> outerStatelessFactory;
        hr = groupFactory->QueryInterface(IID_InternalStatelessServiceGroupFactory, outerStatelessFactory.VoidInitializationAddress());
        VERIFY_IS_TRUE(SUCCEEDED(hr));
    }

    BOOST_AUTO_TEST_CASE(CreateStatefulFactory)
    {
        ComPointer<IFabricStatefulServiceFactory> innerStatefulFactory = make_com<DummyFactory, IFabricStatefulServiceFactory>();

        ComPointer<IFabricServiceGroupFactoryBuilder> builder;

        //
        // CServiceGroupFactoryBuilder does not friend make_com(T&)
        //
        CServiceGroupFactoryBuilder* builderRaw = new CServiceGroupFactoryBuilder(nullptr);
        HRESULT hr = builderRaw->QueryInterface(IID_IFabricServiceGroupFactoryBuilder, builder.VoidInitializationAddress());
        builderRaw->Release();
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        hr = builder->AddStatefulServiceFactory(L"Type1", innerStatefulFactory.GetRawPointer());
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        hr = builder->AddStatefulServiceFactory(L"Type2", innerStatefulFactory.GetRawPointer());
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        ComPointer<IFabricServiceGroupFactory> groupFactory;
        hr = builder->ToServiceGroupFactory(groupFactory.InitializationAddress());
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        ComPointer<IFabricStatefulServiceFactory> outerStatefulFactory;
        hr = groupFactory->QueryInterface(IID_InternalIStatefulServiceGroupFactory, outerStatefulFactory.VoidInitializationAddress());
        VERIFY_IS_TRUE(SUCCEEDED(hr));
    }

    BOOST_AUTO_TEST_CASE(CreateMixedStatefulFactory)
    {
        ComPointer<IFabricStatefulServiceFactory> innerStatefulFactory = make_com<DummyFactory, IFabricStatefulServiceFactory>();

        ComPointer<IFabricStatelessServiceFactory> innerStatelessFactory = make_com<DummyFactory, IFabricStatelessServiceFactory>();

        ComPointer<IFabricServiceGroupFactoryBuilder> builder;

        //
        // CServiceGroupFactoryBuilder does not friend make_com(T&)
        //
        CServiceGroupFactoryBuilder* builderRaw = new CServiceGroupFactoryBuilder(nullptr);
        HRESULT hr = builderRaw->QueryInterface(IID_IFabricServiceGroupFactoryBuilder, builder.VoidInitializationAddress());
        builderRaw->Release();
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        hr = builder->AddStatefulServiceFactory(L"Type1", innerStatefulFactory.GetRawPointer());
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        hr = builder->AddStatelessServiceFactory(L"Type2", innerStatelessFactory.GetRawPointer());
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        ComPointer<IFabricServiceGroupFactory> groupFactory;
        hr = builder->ToServiceGroupFactory(groupFactory.InitializationAddress());
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        ComPointer<IFabricStatefulServiceFactory> outerStatefulFactory;
        hr = groupFactory->QueryInterface(IID_InternalIStatefulServiceGroupFactory, outerStatefulFactory.VoidInitializationAddress());
        VERIFY_IS_TRUE(SUCCEEDED(hr));
    }

    BOOST_AUTO_TEST_CASE(CreateStatelessFactoryDuplicateName)
    {
        ComPointer<IFabricStatelessServiceFactory> innerStatelessFactory = make_com<DummyFactory, IFabricStatelessServiceFactory>();

        ComPointer<IFabricServiceGroupFactoryBuilder> builder;

        //
        // CServiceGroupFactoryBuilder does not friend make_com(T&)
        //
        CServiceGroupFactoryBuilder* builderRaw = new CServiceGroupFactoryBuilder(nullptr);
        HRESULT hr = builderRaw->QueryInterface(IID_IFabricServiceGroupFactoryBuilder, builder.VoidInitializationAddress());
        builderRaw->Release();
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        hr = builder->AddStatelessServiceFactory(L"Type1", innerStatelessFactory.GetRawPointer());
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        hr = builder->AddStatelessServiceFactory(L"Type1", innerStatelessFactory.GetRawPointer());
        VERIFY_IS_FALSE(SUCCEEDED(hr));
    }

    BOOST_AUTO_TEST_CASE(CreateStatefulFactoryDuplicateName)
    {
        ComPointer<IFabricStatefulServiceFactory> innerStatefulFactory = make_com<DummyFactory, IFabricStatefulServiceFactory>();

        ComPointer<IFabricServiceGroupFactoryBuilder> builder;

        //
        // CServiceGroupFactoryBuilder does not friend make_com(T&)
        //
        CServiceGroupFactoryBuilder* builderRaw = new CServiceGroupFactoryBuilder(nullptr);
        HRESULT hr = builderRaw->QueryInterface(IID_IFabricServiceGroupFactoryBuilder, builder.VoidInitializationAddress());
        builderRaw->Release();
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        hr = builder->AddStatefulServiceFactory(L"Type1", innerStatefulFactory.GetRawPointer());
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        hr = builder->AddStatefulServiceFactory(L"Type1", innerStatefulFactory.GetRawPointer());
        VERIFY_IS_FALSE(SUCCEEDED(hr));
    }

    BOOST_AUTO_TEST_CASE(CreateMixedStatefulFactoryDuplicateName)
    {
        ComPointer<IFabricStatefulServiceFactory> innerStatefulFactory = make_com<DummyFactory, IFabricStatefulServiceFactory>();

        ComPointer<IFabricStatelessServiceFactory> innerStatelessFactory = make_com<DummyFactory, IFabricStatelessServiceFactory>();

        ComPointer<IFabricServiceGroupFactoryBuilder> builder;

        //
        // CServiceGroupFactoryBuilder does not friend make_com(T&)
        //
        CServiceGroupFactoryBuilder* builderRaw = new CServiceGroupFactoryBuilder(nullptr);
        HRESULT hr = builderRaw->QueryInterface(IID_IFabricServiceGroupFactoryBuilder, builder.VoidInitializationAddress());
        builderRaw->Release();
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        hr = builder->AddStatefulServiceFactory(L"Type1", innerStatefulFactory.GetRawPointer());
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        hr = builder->AddStatelessServiceFactory(L"Type1", innerStatelessFactory.GetRawPointer());
        VERIFY_IS_FALSE(SUCCEEDED(hr));
    }

    BOOST_AUTO_TEST_CASE(CreateWithInvalidNames)
    {
        ComPointer<IFabricStatefulServiceFactory> innerStatefulFactory = make_com<DummyFactory, IFabricStatefulServiceFactory>();

        ComPointer<IFabricStatelessServiceFactory> innerStatelessFactory = make_com<DummyFactory, IFabricStatelessServiceFactory>();

        ComPointer<IFabricServiceGroupFactoryBuilder> builder;

        //
        // CServiceGroupFactoryBuilder does not friend make_com(T&)
        //
        CServiceGroupFactoryBuilder* builderRaw = new CServiceGroupFactoryBuilder(nullptr);
        HRESULT hr = builderRaw->QueryInterface(IID_IFabricServiceGroupFactoryBuilder, builder.VoidInitializationAddress());
        builderRaw->Release();
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        hr = builder->AddStatefulServiceFactory(L"", innerStatefulFactory.GetRawPointer());
        VERIFY_IS_FALSE(SUCCEEDED(hr));

        hr = builder->AddStatelessServiceFactory(L"", innerStatelessFactory.GetRawPointer());
        VERIFY_IS_FALSE(SUCCEEDED(hr));

        hr = builder->AddStatefulServiceFactory(nullptr, innerStatefulFactory.GetRawPointer());
        VERIFY_IS_FALSE(SUCCEEDED(hr));

        hr = builder->AddStatelessServiceFactory(nullptr, innerStatelessFactory.GetRawPointer());
        VERIFY_IS_FALSE(SUCCEEDED(hr));
    }

    BOOST_AUTO_TEST_CASE(CreateWithInvalidFactories)
    {
        ComPointer<IFabricServiceGroupFactoryBuilder> builder;

        //
        // CServiceGroupFactoryBuilder does not friend make_com(T&)
        //
        CServiceGroupFactoryBuilder* builderRaw = new CServiceGroupFactoryBuilder(nullptr);
        HRESULT hr = builderRaw->QueryInterface(IID_IFabricServiceGroupFactoryBuilder, builder.VoidInitializationAddress());
        builderRaw->Release();
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        hr = builder->AddStatefulServiceFactory(L"Test1", nullptr);
        VERIFY_IS_FALSE(SUCCEEDED(hr));

        hr = builder->AddStatelessServiceFactory(L"Test2", nullptr);
        VERIFY_IS_FALSE(SUCCEEDED(hr));
    }

    BOOST_AUTO_TEST_CASE(CreateEmpty)
    {
        ComPointer<IFabricServiceGroupFactoryBuilder> builder;

        //
        // CServiceGroupFactoryBuilder does not friend make_com(T&)
        //
        CServiceGroupFactoryBuilder* builderRaw = new CServiceGroupFactoryBuilder(nullptr);
        HRESULT hr = builderRaw->QueryInterface(IID_IFabricServiceGroupFactoryBuilder, builder.VoidInitializationAddress());
        builderRaw->Release();
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        ComPointer<IFabricServiceGroupFactory> groupFactory;
        hr = builder->ToServiceGroupFactory(groupFactory.InitializationAddress());
        VERIFY_IS_FALSE(SUCCEEDED(hr));
    }

    BOOST_AUTO_TEST_CASE(CreateStatelessFactoryThenRemove)
    {
        ComPointer<IFabricStatelessServiceFactory> innerStatelessFactory = make_com<DummyFactory, IFabricStatelessServiceFactory>();

        ComPointer<IFabricServiceGroupFactoryBuilder> builder;

        //
        // CServiceGroupFactoryBuilder does not friend make_com(T&)
        //
        CServiceGroupFactoryBuilder* builderRaw = new CServiceGroupFactoryBuilder(nullptr);
        HRESULT hr = builderRaw->QueryInterface(IID_IFabricServiceGroupFactoryBuilder, builder.VoidInitializationAddress());
        builderRaw->Release();
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        hr = builder->AddStatelessServiceFactory(L"Type1", innerStatelessFactory.GetRawPointer());
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        hr = builder->AddStatelessServiceFactory(L"Type2", innerStatelessFactory.GetRawPointer());
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        hr = builder->RemoveServiceFactory(L"Type1");
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        hr = builder->RemoveServiceFactory(L"Type2");
        VERIFY_IS_TRUE(SUCCEEDED(hr));
    }

    BOOST_AUTO_TEST_CASE(CreateStatefulFactoryThenRemove)
    {
        ComPointer<IFabricStatefulServiceFactory> innerStatefulFactory = make_com<DummyFactory, IFabricStatefulServiceFactory>();

        ComPointer<IFabricServiceGroupFactoryBuilder> builder;

        //
        // CServiceGroupFactoryBuilder does not friend make_com(T&)
        //
        CServiceGroupFactoryBuilder* builderRaw = new CServiceGroupFactoryBuilder(nullptr);
        HRESULT hr = builderRaw->QueryInterface(IID_IFabricServiceGroupFactoryBuilder, builder.VoidInitializationAddress());
        builderRaw->Release();
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        hr = builder->AddStatefulServiceFactory(L"Type1", innerStatefulFactory.GetRawPointer());
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        hr = builder->AddStatefulServiceFactory(L"Type2", innerStatefulFactory.GetRawPointer());
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        hr = builder->RemoveServiceFactory(L"Type1");
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        hr = builder->RemoveServiceFactory(L"Type2");
        VERIFY_IS_TRUE(SUCCEEDED(hr));
    }

    BOOST_AUTO_TEST_CASE(CreateMixedStatefulFactoryThenRemove)
    {
        ComPointer<IFabricStatefulServiceFactory> innerStatefulFactory = make_com<DummyFactory, IFabricStatefulServiceFactory>();

        ComPointer<IFabricStatelessServiceFactory> innerStatelessFactory = make_com<DummyFactory, IFabricStatelessServiceFactory>();

        ComPointer<IFabricServiceGroupFactoryBuilder> builder;

        //
        // CServiceGroupFactoryBuilder does not friend make_com(T&)
        //
        CServiceGroupFactoryBuilder* builderRaw = new CServiceGroupFactoryBuilder(nullptr);
        HRESULT hr = builderRaw->QueryInterface(IID_IFabricServiceGroupFactoryBuilder, builder.VoidInitializationAddress());
        builderRaw->Release();
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        hr = builder->AddStatefulServiceFactory(L"Type1", innerStatefulFactory.GetRawPointer());
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        hr = builder->AddStatelessServiceFactory(L"Type2", innerStatelessFactory.GetRawPointer());
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        hr = builder->RemoveServiceFactory(L"Type1");
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        hr = builder->RemoveServiceFactory(L"Type2");
        VERIFY_IS_TRUE(SUCCEEDED(hr));
    }

    BOOST_AUTO_TEST_CASE(CreateMixedStatefulFactoryThenRemoveTwice)
    {
        ComPointer<IFabricStatefulServiceFactory> innerStatefulFactory = make_com<DummyFactory, IFabricStatefulServiceFactory>();

        ComPointer<IFabricStatelessServiceFactory> innerStatelessFactory = make_com<DummyFactory, IFabricStatelessServiceFactory>();

        ComPointer<IFabricServiceGroupFactoryBuilder> builder;

        //
        // CServiceGroupFactoryBuilder does not friend make_com(T&)
        //
        CServiceGroupFactoryBuilder* builderRaw = new CServiceGroupFactoryBuilder(nullptr);
        HRESULT hr = builderRaw->QueryInterface(IID_IFabricServiceGroupFactoryBuilder, builder.VoidInitializationAddress());
        builderRaw->Release();
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        hr = builder->AddStatefulServiceFactory(L"Type1", innerStatefulFactory.GetRawPointer());
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        hr = builder->AddStatelessServiceFactory(L"Type2", innerStatelessFactory.GetRawPointer());
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        hr = builder->RemoveServiceFactory(L"Type1");
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        hr = builder->RemoveServiceFactory(L"Type2");
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        hr = builder->RemoveServiceFactory(L"Type1");
        VERIFY_IS_FALSE(SUCCEEDED(hr));

        hr = builder->RemoveServiceFactory(L"Type2");
        VERIFY_IS_FALSE(SUCCEEDED(hr));
    }

    BOOST_AUTO_TEST_CASE(CreateMixedStatefulFactoryThenRemoveNonExisting)
    {
        ComPointer<IFabricStatefulServiceFactory> innerStatefulFactory = make_com<DummyFactory, IFabricStatefulServiceFactory>();

        ComPointer<IFabricStatelessServiceFactory> innerStatelessFactory = make_com<DummyFactory, IFabricStatelessServiceFactory>();

        ComPointer<IFabricServiceGroupFactoryBuilder> builder;

        //
        // CServiceGroupFactoryBuilder does not friend make_com(T&)
        //
        CServiceGroupFactoryBuilder* builderRaw = new CServiceGroupFactoryBuilder(nullptr);
        HRESULT hr = builderRaw->QueryInterface(IID_IFabricServiceGroupFactoryBuilder, builder.VoidInitializationAddress());
        builderRaw->Release();
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        hr = builder->AddStatefulServiceFactory(L"Type1", innerStatefulFactory.GetRawPointer());
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        hr = builder->AddStatelessServiceFactory(L"Type2", innerStatelessFactory.GetRawPointer());
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        hr = builder->RemoveServiceFactory(L"Type3");
        VERIFY_IS_FALSE(SUCCEEDED(hr));

        hr = builder->RemoveServiceFactory(L"Type4");
        VERIFY_IS_FALSE(SUCCEEDED(hr));

         hr = builder->RemoveServiceFactory(L"Type1");
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        hr = builder->RemoveServiceFactory(L"Type2");
        VERIFY_IS_TRUE(SUCCEEDED(hr));
    }

    BOOST_AUTO_TEST_CASE(CreateStatelessFactoryThenCreateInstance)
    {
        Common::Guid partitionId = Common::Guid::NewGuid();

        ComPointer<IFabricStatelessServiceFactory> innerStatelessFactory = make_com<DummyFactory, IFabricStatelessServiceFactory>();

        ComPointer<IFabricServiceGroupFactoryBuilder> builder;

        //
        // CServiceGroupFactoryBuilder does not friend make_com(T&)
        //
        CServiceGroupFactoryBuilder* builderRaw = new CServiceGroupFactoryBuilder(nullptr);
        HRESULT hr = builderRaw->QueryInterface(IID_IFabricServiceGroupFactoryBuilder, builder.VoidInitializationAddress());
        builderRaw->Release();
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        hr = builder->AddStatelessServiceFactory(L"Type1", innerStatelessFactory.GetRawPointer());
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        hr = builder->AddStatelessServiceFactory(L"Type2", innerStatelessFactory.GetRawPointer());
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        ComPointer<IFabricServiceGroupFactory> groupFactory;
        hr = builder->ToServiceGroupFactory(groupFactory.InitializationAddress());
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        ComPointer<IFabricStatelessServiceFactory> outerStatelessFactory;
        hr = groupFactory->QueryInterface(IID_InternalStatelessServiceGroupFactory, outerStatelessFactory.VoidInitializationAddress());
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        ComPointer<IFabricStatelessServiceInstance> instance;

        CServiceGroupMemberDescription member1;
        member1.ServiceDescriptionType = FABRIC_SERVICE_DESCRIPTION_KIND_STATELESS;
        member1.ServiceName = L"fabric:/app/group#m1";
        member1.ServiceType = L"Type1";
        member1.Identifier = Common::Guid::NewGuid().AsGUID();

        CServiceGroupMemberDescription member2;
        member2.ServiceDescriptionType = FABRIC_SERVICE_DESCRIPTION_KIND_STATELESS;
        member2.ServiceName = L"fabric:/app/group#m2";
        member2.ServiceType = L"Type2";
        member2.Identifier = Common::Guid::NewGuid().AsGUID();

        CServiceGroupDescription description;
        description.ServiceGroupMemberData.push_back(member1);
        description.ServiceGroupMemberData.push_back(member2);

        vector<byte> initData;
        VERIFY_IS_TRUE(FabricSerializer::Serialize(&description, initData).IsSuccess());

        hr = outerStatelessFactory->CreateInstance(
            L"GroupType",
            L"fabric:/app/group",
            (ULONG)initData.size(),
            initData.data(),
            partitionId.AsGUID(),
            0,
            instance.InitializationAddress());

        VERIFY_IS_TRUE(SUCCEEDED(hr));
    }

    BOOST_AUTO_TEST_CASE(CreateStatefulFactoryThenCreateReplica)
    {
        Common::Guid partitionId = Common::Guid::NewGuid();

        ComPointer<IFabricStatefulServiceFactory> innerStatefulFactory = make_com<DummyFactory, IFabricStatefulServiceFactory>();

        ComPointer<IFabricServiceGroupFactoryBuilder> builder;

        //
        // CServiceGroupFactoryBuilder does not friend make_com(T&)
        //
        CServiceGroupFactoryBuilder* builderRaw = new CServiceGroupFactoryBuilder(nullptr);
        HRESULT hr = builderRaw->QueryInterface(IID_IFabricServiceGroupFactoryBuilder, builder.VoidInitializationAddress());
        builderRaw->Release();
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        hr = builder->AddStatefulServiceFactory(L"Type1", innerStatefulFactory.GetRawPointer());
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        hr = builder->AddStatefulServiceFactory(L"Type2", innerStatefulFactory.GetRawPointer());
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        ComPointer<IFabricServiceGroupFactory> groupFactory;
        hr = builder->ToServiceGroupFactory(groupFactory.InitializationAddress());
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        ComPointer<IFabricStatefulServiceFactory> outerStatefulFactory;
        hr = groupFactory->QueryInterface(IID_InternalIStatefulServiceGroupFactory, outerStatefulFactory.VoidInitializationAddress());
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        ComPointer<IFabricStatefulServiceReplica> replica;

        CServiceGroupMemberDescription member1;
        member1.ServiceDescriptionType = FABRIC_SERVICE_DESCRIPTION_KIND_STATEFUL;
        member1.ServiceName = L"fabric:/app/group#m1";
        member1.ServiceType = L"Type1";
        member1.Identifier = Common::Guid::NewGuid().AsGUID();

        CServiceGroupMemberDescription member2;
        member2.ServiceDescriptionType = FABRIC_SERVICE_DESCRIPTION_KIND_STATEFUL;
        member2.ServiceName = L"fabric:/app/group#m2";
        member2.ServiceType = L"Type2";
        member2.Identifier = Common::Guid::NewGuid().AsGUID();

        CServiceGroupDescription description;
        description.ServiceGroupMemberData.push_back(member1);
        description.ServiceGroupMemberData.push_back(member2);

        vector<byte> initData;
        VERIFY_IS_TRUE(FabricSerializer::Serialize(&description, initData).IsSuccess());

        hr = outerStatefulFactory->CreateReplica(
            L"GroupType",
            L"fabric:/app/group",
            (ULONG)initData.size(),
            initData.data(),
            partitionId.AsGUID(),
            0,
            replica.InitializationAddress());

        VERIFY_IS_TRUE(SUCCEEDED(hr));
    }

    BOOST_AUTO_TEST_CASE(CreateStatelessFactoryThenCreateInstanceAndFail)
    {
        Common::Guid partitionId = Common::Guid::NewGuid();

        ComPointer<IFabricStatelessServiceFactory> innerStatelessFactory = make_com<DummyFactory, IFabricStatelessServiceFactory>();

        ComPointer<IFabricServiceGroupFactoryBuilder> builder;

        //
        // CServiceGroupFactoryBuilder does not friend make_com(T&)
        //
        CServiceGroupFactoryBuilder* builderRaw = new CServiceGroupFactoryBuilder(nullptr);
        HRESULT hr = builderRaw->QueryInterface(IID_IFabricServiceGroupFactoryBuilder, builder.VoidInitializationAddress());
        builderRaw->Release();
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        hr = builder->AddStatelessServiceFactory(L"E_FAIL", innerStatelessFactory.GetRawPointer());
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        hr = builder->AddStatelessServiceFactory(L"Type2", innerStatelessFactory.GetRawPointer());
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        ComPointer<IFabricServiceGroupFactory> groupFactory;
        hr = builder->ToServiceGroupFactory(groupFactory.InitializationAddress());
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        ComPointer<IFabricStatelessServiceFactory> outerStatelessFactory;
        hr = groupFactory->QueryInterface(IID_InternalStatelessServiceGroupFactory, outerStatelessFactory.VoidInitializationAddress());
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        ComPointer<IFabricStatelessServiceInstance> instance;

        CServiceGroupMemberDescription member1;
        member1.ServiceDescriptionType = FABRIC_SERVICE_DESCRIPTION_KIND_STATELESS;
        member1.ServiceName = L"fabric:/app/group#m1";
        member1.ServiceType = L"E_FAIL";
        member1.Identifier = Common::Guid::NewGuid().AsGUID();

        CServiceGroupMemberDescription member2;
        member2.ServiceDescriptionType = FABRIC_SERVICE_DESCRIPTION_KIND_STATELESS;
        member2.ServiceName = L"fabric:/app/group#m2";
        member2.ServiceType = L"Type2";
        member2.Identifier = Common::Guid::NewGuid().AsGUID();

        CServiceGroupDescription description;
        description.ServiceGroupMemberData.push_back(member1);
        description.ServiceGroupMemberData.push_back(member2);

        vector<byte> initData;
        VERIFY_IS_TRUE(FabricSerializer::Serialize(&description, initData).IsSuccess());

        hr = outerStatelessFactory->CreateInstance(
            L"GroupType",
            L"fabric:/app/group",
            (ULONG)initData.size(),
            initData.data(),
            partitionId.AsGUID(),
            0,
            instance.InitializationAddress());

        VERIFY_IS_FALSE(SUCCEEDED(hr));
    }

    BOOST_AUTO_TEST_CASE(CreateStatefulFactoryThenCreateReplicaAndFail)
    {
        Common::Guid partitionId = Common::Guid::NewGuid();

        ComPointer<IFabricStatefulServiceFactory> innerStatefulFactory = make_com<DummyFactory, IFabricStatefulServiceFactory>();

        ComPointer<IFabricServiceGroupFactoryBuilder> builder;

        //
        // CServiceGroupFactoryBuilder does not friend make_com(T&)
        //
        CServiceGroupFactoryBuilder* builderRaw = new CServiceGroupFactoryBuilder(nullptr);
        HRESULT hr = builderRaw->QueryInterface(IID_IFabricServiceGroupFactoryBuilder, builder.VoidInitializationAddress());
        builderRaw->Release();
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        hr = builder->AddStatefulServiceFactory(L"E_FAIL", innerStatefulFactory.GetRawPointer());
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        hr = builder->AddStatefulServiceFactory(L"Type2", innerStatefulFactory.GetRawPointer());
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        ComPointer<IFabricServiceGroupFactory> groupFactory;
        hr = builder->ToServiceGroupFactory(groupFactory.InitializationAddress());
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        ComPointer<IFabricStatefulServiceFactory> outerStatefulFactory;
        hr = groupFactory->QueryInterface(IID_InternalIStatefulServiceGroupFactory, outerStatefulFactory.VoidInitializationAddress());
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        ComPointer<IFabricStatefulServiceReplica> replica;

        CServiceGroupMemberDescription member1;
        member1.ServiceDescriptionType = FABRIC_SERVICE_DESCRIPTION_KIND_STATEFUL;
        member1.ServiceName = L"fabric:/app/group#m1";
        member1.ServiceType = L"E_FAIL";
        member1.Identifier = Common::Guid::NewGuid().AsGUID();

        CServiceGroupMemberDescription member2;
        member2.ServiceDescriptionType = FABRIC_SERVICE_DESCRIPTION_KIND_STATEFUL;
        member2.ServiceName = L"fabric:/app/group#m2";
        member2.ServiceType = L"Type2";
        member2.Identifier = Common::Guid::NewGuid().AsGUID();

        CServiceGroupDescription description;
        description.ServiceGroupMemberData.push_back(member1);
        description.ServiceGroupMemberData.push_back(member2);

        vector<byte> initData;
        VERIFY_IS_TRUE(FabricSerializer::Serialize(&description, initData).IsSuccess());

        hr = outerStatefulFactory->CreateReplica(
            L"GroupType",
            L"fabric:/app/group",
            (ULONG)initData.size(),
            initData.data(),
            partitionId.AsGUID(),
            0,
            replica.InitializationAddress());

        VERIFY_IS_FALSE(SUCCEEDED(hr));
    }

    BOOST_AUTO_TEST_CASE(CreateStatelessFactoryThenCreateInstanceNoFactory)
    {
        Common::Guid partitionId = Common::Guid::NewGuid();

        ComPointer<IFabricStatelessServiceFactory> innerStatelessFactory = make_com<DummyFactory, IFabricStatelessServiceFactory>();

        ComPointer<IFabricServiceGroupFactoryBuilder> builder;

        //
        // CServiceGroupFactoryBuilder does not friend make_com(T&)
        //
        CServiceGroupFactoryBuilder* builderRaw = new CServiceGroupFactoryBuilder(nullptr);
        HRESULT hr = builderRaw->QueryInterface(IID_IFabricServiceGroupFactoryBuilder, builder.VoidInitializationAddress());
        builderRaw->Release();
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        hr = builder->AddStatelessServiceFactory(L"Type1", innerStatelessFactory.GetRawPointer());
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        hr = builder->AddStatelessServiceFactory(L"Type2", innerStatelessFactory.GetRawPointer());
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        ComPointer<IFabricServiceGroupFactory> groupFactory;
        hr = builder->ToServiceGroupFactory(groupFactory.InitializationAddress());
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        ComPointer<IFabricStatelessServiceFactory> outerStatelessFactory;
        hr = groupFactory->QueryInterface(IID_InternalStatelessServiceGroupFactory, outerStatelessFactory.VoidInitializationAddress());
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        ComPointer<IFabricStatelessServiceInstance> instance;

        CServiceGroupMemberDescription member1;
        member1.ServiceDescriptionType = FABRIC_SERVICE_DESCRIPTION_KIND_STATELESS;
        member1.ServiceName = L"fabric:/app/group#m1";
        member1.ServiceType = L"TypeX";
        member1.Identifier = Common::Guid::NewGuid().AsGUID();

        CServiceGroupMemberDescription member2;
        member2.ServiceDescriptionType = FABRIC_SERVICE_DESCRIPTION_KIND_STATELESS;
        member2.ServiceName = L"fabric:/app/group#m2";
        member2.ServiceType = L"Type2";
        member2.Identifier = Common::Guid::NewGuid().AsGUID();

        CServiceGroupDescription description;
        description.ServiceGroupMemberData.push_back(member1);
        description.ServiceGroupMemberData.push_back(member2);

        vector<byte> initData;
        VERIFY_IS_TRUE(FabricSerializer::Serialize(&description, initData).IsSuccess());

        hr = outerStatelessFactory->CreateInstance(
            L"GroupType",
            L"fabric:/app/group",
            (ULONG)initData.size(),
            initData.data(),
            partitionId.AsGUID(),
            0,
            instance.InitializationAddress());

        VERIFY_IS_FALSE(SUCCEEDED(hr));
    }

    BOOST_AUTO_TEST_CASE(CreateStatefulFactoryThenCreateReplicaNoFactory)
    {
        Common::Guid partitionId = Common::Guid::NewGuid();

        ComPointer<IFabricStatefulServiceFactory> innerStatefulFactory = make_com<DummyFactory, IFabricStatefulServiceFactory>();

        ComPointer<IFabricServiceGroupFactoryBuilder> builder;

        //
        // CServiceGroupFactoryBuilder does not friend make_com(T&)
        //
        CServiceGroupFactoryBuilder* builderRaw = new CServiceGroupFactoryBuilder(nullptr);
        HRESULT hr = builderRaw->QueryInterface(IID_IFabricServiceGroupFactoryBuilder, builder.VoidInitializationAddress());
        builderRaw->Release();
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        hr = builder->AddStatefulServiceFactory(L"Type1", innerStatefulFactory.GetRawPointer());
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        hr = builder->AddStatefulServiceFactory(L"Type2", innerStatefulFactory.GetRawPointer());
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        ComPointer<IFabricServiceGroupFactory> groupFactory;
        hr = builder->ToServiceGroupFactory(groupFactory.InitializationAddress());
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        ComPointer<IFabricStatefulServiceFactory> outerStatefulFactory;
        hr = groupFactory->QueryInterface(IID_InternalIStatefulServiceGroupFactory, outerStatefulFactory.VoidInitializationAddress());
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        ComPointer<IFabricStatefulServiceReplica> replica;

        CServiceGroupMemberDescription member1;
        member1.ServiceDescriptionType = FABRIC_SERVICE_DESCRIPTION_KIND_STATEFUL;
        member1.ServiceName = L"fabric:/app/group#m1";
        member1.ServiceType = L"Type1";
        member1.Identifier = Common::Guid::NewGuid().AsGUID();

        CServiceGroupMemberDescription member2;
        member2.ServiceDescriptionType = FABRIC_SERVICE_DESCRIPTION_KIND_STATEFUL;
        member2.ServiceName = L"fabric:/app/group#m2";
        member2.ServiceType = L"TypeX";
        member2.Identifier = Common::Guid::NewGuid().AsGUID();

        CServiceGroupDescription description;
        description.ServiceGroupMemberData.push_back(member1);
        description.ServiceGroupMemberData.push_back(member2);

        vector<byte> initData;
        VERIFY_IS_TRUE(FabricSerializer::Serialize(&description, initData).IsSuccess());

        hr = outerStatefulFactory->CreateReplica(
            L"GroupType",
            L"fabric:/app/group",
            (ULONG)initData.size(),
            initData.data(),
            partitionId.AsGUID(),
            0,
            replica.InitializationAddress());

        VERIFY_IS_FALSE(SUCCEEDED(hr));
    }

    BOOST_AUTO_TEST_SUITE_END()
}
