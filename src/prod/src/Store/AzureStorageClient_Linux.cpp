//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Store;

class AzureStorageClient::Impl
{
};

AzureStorageClient::AzureStorageClient(PartitionedReplicaId const &, std::wstring &&) : impl_() { }

shared_ptr<AzureStorageClient> AzureStorageClient::Create(PartitionedReplicaId const & id, std::wstring && rawConnection)
{
    return shared_ptr<AzureStorageClient>(new AzureStorageClient(id, move(rawConnection)));
}

ErrorCode AzureStorageClient::Upload(
    std::wstring const &, 
    std::wstring const &,
    std::wstring const &)
{
    return ErrorCodeValue::NotImplemented;
}

ErrorCode AzureStorageClient::Download(
    std::wstring const &, 
    std::wstring const &,
    std::wstring const &)
{
    return ErrorCodeValue::NotImplemented;
}
