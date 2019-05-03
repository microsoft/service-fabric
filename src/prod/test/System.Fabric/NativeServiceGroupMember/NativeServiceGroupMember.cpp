// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include "ServiceFactory.h"

using namespace NativeServiceGroupMember;

HRESULT FabricMain(IFabricRuntime * runtime, IFabricCodePackageActivationContext * activationContext)
{
    if (runtime == NULL || activationContext == NULL)
    {
        return E_POINTER;
    }

    try
    {
        ComPointer<IFabricStatelessServiceFactory> statelessFactory = make_com<ServiceFactory, IFabricStatelessServiceFactory>();
        ComPointer<IFabricStatefulServiceFactory> statefulFactory = make_com<ServiceFactory, IFabricStatefulServiceFactory>();

        FABRIC_SERVICE_GROUP_TYPE_DESCRIPTION_LIST const * list = activationContext->get_ServiceGroupTypes();
        for(ULONG i = 0; i < list->Count; i++)
        {
            if(list->Items[i].UseImplicitFactory)
            {
                FABRIC_SERVICE_GROUP_TYPE_MEMBER_DESCRIPTION_LIST const * members = list->Items[i].Members;
                for(ULONG j = 0; j < members->Count; j++)
                {
                    if(wstring(members->Items[j].ServiceTypeName) == L"StatelessAdderNative")
                    {
                        HRESULT hr = runtime->RegisterStatelessServiceFactory(L"StatelessAdderNative", statelessFactory.GetRawPointer());
                        if (FAILED(hr))
                        {
                            return hr;
                        }
                    }
                    else if(wstring(members->Items[j].ServiceTypeName) == L"StatefulAdderNative")
                    {
                        HRESULT hr = runtime->RegisterStatefulServiceFactory(L"StatefulAdderNative", statefulFactory.GetRawPointer());
                        if (FAILED(hr))
                        {
                            return hr;
                        }
                    }                    
                }
            }
        }

        return S_OK;
    }
    catch (bad_alloc const &) { return E_OUTOFMEMORY; }
    catch (...) { return E_FAIL; }
}


//
// For verification of dll export used by unmanaged DLL hosting.
FnFabricMain dllhostExport = FabricMain;
