// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

namespace Store
{
    ErrorCode StoreFactory::CreateLocalStore(StoreFactoryParameters const & parameters, ILocalStoreSPtr & store)
    {
        switch (parameters.Type)
        {
        case StoreType::Local:
        {
#ifdef PLATFORM_UNIX
            UNREFERENCED_PARAMETER(parameters);
            UNREFERENCED_PARAMETER(store);

            CODING_ASSERT("ESE not supported in Linux for RA");
#else
            auto error = Directory::Create2(parameters.DirectoryPath);
            if (!error.IsSuccess()) { return error; }

            EseLocalStoreSettings settings;
            settings.StoreName = parameters.FileName;
            settings.AssertOnFatalError = parameters.AssertOnFatalError;

            store = make_shared<EseLocalStore>(
                parameters.DirectoryPath,
                settings,
                EseLocalStore::LOCAL_STORE_FLAG_NONE);
            return error;
#endif
        }
        case StoreType::TStore:
        {
            store = make_shared<TSLocalStore>(
                make_unique<TSReplicatedStoreSettings>(parameters.DirectoryPath),
                parameters.KtlLogger);

            return ErrorCodeValue::Success;
        }

        default:
            Assert::CodingError("Unknown type {0}", static_cast<int>(parameters.Type));
        }
    }
}
