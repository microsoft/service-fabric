// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include "EseStoreTestHelper.h"

using namespace Common;
using namespace std;

namespace Naming
{
    namespace TestHelper
    {
        Common::GlobalWString EseStoreTestHelper::EseDirectoryPrefix = Common::make_global<std::wstring>(L"EseStoreTestFiles_");

        void EseStoreTestHelper::CreateDirectories(int count)
        {
            for (int ix = 0; ix < count; ++ix)
            {
                Directory::Create(GetEseDirectory(ix));
            }
        }

        void EseStoreTestHelper::DeleteDirectories(int count)
        {
            bool succeeded = false;
            int attempts = 0;
            while (!succeeded && attempts < 10)
            {
                succeeded = true;
                ++attempts;
                for (int ix = 0; ix < count; ++ix)
                {
                    wstring directory = GetEseDirectory(ix);

                    if (Directory::Exists(directory))
                    {
                        Trace.WriteInfo(Constants::TestSource, "Deleting directory '{0}'", directory);

                        auto error = Directory::Delete(directory, true);
                        if(!error.IsSuccess())
                        {
                            Trace.WriteWarning(Constants::TestSource, "Couldn't delete directory '{0}' due to {1}", directory, error);
                            succeeded = false;
                            Sleep(2000);
                            break;
                        }
                    }
                }
            }
        }

        wstring EseStoreTestHelper::GetEseDirectory(int index)
        {
            wstring directory;
            StringWriter writer(directory);
            writer.Write(EseDirectoryPrefix);
            writer.Write(index);
            //writer.Write(L"\\");

            wstring rootPath;
            Environment::GetEnvironmentVariable(
                wstring(L"TEMP"),
                rootPath,
                Environment::GetExecutablePath());

            return Path::Combine(rootPath, directory);
        }
    }
}
