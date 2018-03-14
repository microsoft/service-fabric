// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    class RuntimeFolders final
        : public KObject<RuntimeFolders>
        , public KShared<RuntimeFolders>
        , public IRuntimeFolders
    {
        K_FORCE_SHARED(RuntimeFolders)
        K_SHARED_INTERFACE_IMP(IRuntimeFolders)

    public:

        static IRuntimeFolders::SPtr Create(
            __in IFabricCodePackageActivationContext & codePackage,
            __in KAllocator & allocator);

        static IRuntimeFolders::SPtr Create(
            __in IFabricTransactionalReplicatorRuntimeConfigurations & runtimeConfigurations,
            __in KAllocator & allocator);

        LPCWSTR get_WorkDirectory() const override;

    private:

        RuntimeFolders(__in LPCWSTR workDirectory);

        KString::SPtr workDirectory_;
    };
}
