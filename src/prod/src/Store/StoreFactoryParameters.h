// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    struct StoreFactoryParameters
    {
        StoreFactoryParameters()
            : Type(StoreType::Invalid)
            , NodeInstance()
            , NodeName()
            , ConnectionString()
            , DirectoryPath()
            , FileName()
            , AssertOnFatalError(false)
            , KtlLogger()
            , AllowMigration(false)
        {
        }

        StoreType::Enum Type;

        Federation::NodeInstance NodeInstance;
        std::wstring NodeName;

        // Used by SQL store
        std::wstring ConnectionString;

        // Used by Local Store
        std::wstring DirectoryPath;
        std::wstring FileName;
        bool AssertOnFatalError;

        // Used by TStore
        KtlLogger::KtlLoggerNodeSPtr KtlLogger;
        bool AllowMigration;
    };
}
