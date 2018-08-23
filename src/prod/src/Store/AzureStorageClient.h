//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

#pragma once

namespace Store
{
    class AzureStorageClient
    {
    public:

        static std::shared_ptr<AzureStorageClient> Create(PartitionedReplicaId const &, std::wstring && rawConnection);

        Common::ErrorCode Upload(
            std::wstring const & srcPath, 
            std::wstring const & destPath,
            std::wstring const & containerName);

        Common::ErrorCode Download(
            std::wstring const & srcPath, 
            std::wstring const & destPath,
            std::wstring const & containerName);

    private:

        class Impl;

        AzureStorageClient(PartitionedReplicaId const &, std::wstring && rawConnection);

        std::shared_ptr<Impl> impl_;
    };

    typedef std::shared_ptr<AzureStorageClient> AzureStorageClientSPtr;
}
