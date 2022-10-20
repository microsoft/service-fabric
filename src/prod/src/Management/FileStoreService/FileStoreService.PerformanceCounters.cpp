// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Management::FileStoreService;
using namespace Common;
using namespace ServiceModel;

INITIALIZE_COUNTER_SET(FileStoreServiceCounters)

void Management::FileStoreService::FileStoreServiceCounters::OnCreateUploadSessionRequest()
{
    this->RateOfCreateUploadSessionRequests.Increment();
    this->NumberOfCreateUploadSessionRequests.Increment();
}

void Management::FileStoreService::FileStoreServiceCounters::OnCommitUploadSessionRequest()
{
    this->RateOfCommitUploadSessionRequests.Increment();
    this->NumberOfCommitUploadSessionRequests.Increment();
}

void Management::FileStoreService::FileStoreServiceCounters::OnDeleteUploadSessionRequest()
{
    this->RateOfDeleteUploadSessionRequests.Increment();
    this->NumberOfDeleteUploadSessionRequests.Increment();
}

void Management::FileStoreService::FileStoreServiceCounters::OnSuccessfulDeleteUploadSessionRequest()
{
    this->NumberOfSuccessfulDeleteUploadSessionRequests.Increment();
    this->RateOfSuccessfulDeleteUploadSessionRequests.Increment();
}

void Management::FileStoreService::FileStoreServiceCounters::OnSuccessfulCreateUploadSessionRequest()
{
    this->NumberOfSuccessfulCreateUploadSessionRequests.Increment();
    this->RateOfSuccessfulCreateUploadSessionRequests.Increment();
}

void Management::FileStoreService::FileStoreServiceCounters::OnSuccessfulCommitUploadSessionRequest(int64 elapsedTicks)
{
    this->NumberOfSuccessfulCommitUploadSessionRequests.Increment();
    this->RateOfSuccessfulCommitUploadSessionRequests.Increment();
    this->AverageCommitUploadSessionProcessTime.UpdateWithNormalizedTime(elapsedTicks);
}

void Management::FileStoreService::FileStoreServiceCounters::OnDeleteRequest()
{
    this->RateOfDeleteRequests.Increment();
}

void Management::FileStoreService::FileStoreServiceCounters::OnInternalListRequest()
{
    this->RateOfInternalListRequests.Increment();
}

void Management::FileStoreService::FileStoreServiceCounters::OnPendingCommitUploadSessionReply()
{
    this->RateOfPendingCommitUploadOperations.Increment();
}

void Management::FileStoreService::FileStoreServiceCounters::OnCopyActionRequest()
{
    this->RateOfCopyActionRequests.Increment();
}

void Management::FileStoreService::FileStoreServiceCounters::OnUploadActionRequest()
{
    this->RateOfUploadActionRequests.Increment();
}

void Management::FileStoreService::FileStoreServiceCounters::OnNonTransientFailedCommitUploadSessionRequest()
{
    this->NumberOfNonTransientFailedCommitUploadSessionRequests.Increment();
}

void Management::FileStoreService::FileStoreServiceCounters::OnSuccessfulUploadChunkContentRequest()
{
    this->RateOfSuccessfulUploadChunkContentRequests.Increment();
}
