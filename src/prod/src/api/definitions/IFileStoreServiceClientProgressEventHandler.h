// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IFileStoreServiceClientProgressEventHandler
    {
    public:
        virtual ~IFileStoreServiceClientProgressEventHandler() {};

        virtual void IncrementReplicatedFiles(size_t) = 0;
        virtual void IncrementTotalFiles(size_t value) = 0;

        virtual void IncrementTransferCompletedItems(size_t) = 0;
        virtual void IncrementTotalTransferItems(size_t value) = 0;

        virtual void IncrementCompletedItems(size_t) = 0;
        virtual void IncrementTotalItems(size_t) = 0;
    };
}

