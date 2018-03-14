// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    namespace TestHelper
    {
        class EseStoreTestHelper
        {
        public:
            static void CreateDirectories(int);
            static void DeleteDirectories(int);

            static std::wstring GetEseDirectory(int);

        private:
            static Common::GlobalWString EseDirectoryPrefix;
        };
    }
}
