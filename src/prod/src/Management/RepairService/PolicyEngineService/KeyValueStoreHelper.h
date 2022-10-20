// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace RepairPolicyEngine
{
    class KeyValueStoreHelper
    {
	public:
        static bool LoadFromPersistentStore(NodeRepairInfo& nodeRepairInfo, Common::ComPointer<IFabricKeyValueStoreReplica2> keyValueStorePtr);

        static HRESULT WriteToPersistentStore(NodeRepairInfo& nodeRepairInfo, Common::ComPointer<IFabricKeyValueStoreReplica2> keyValueStorePtr, const RepairPolicyEngineConfiguration& repairConfiguration);

        static bool GetReplicatorAddress(Common::ComPointer<IFabricCodePackageActivationContext> activationContextCPtr, _Out_ std::wstring& replicatorAddress);

    private:
        static HRESULT KeyValueStoreHelper::WriteToPersistentStoreInternal(NodeRepairInfo& nodeRepairInfo, 
                                                 Common::ComPointer<IFabricKeyValueStoreReplica2>  keyValueStorePtr, 
                                                 const RepairPolicyEngineConfiguration& repairConfiguration);

        static const std::wstring ReplicatorEndpointName;
	};
}
