// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Management
{
    namespace CentralSecretService
    {
        class ServiceModelConstants
        {
        public:
            static const int SecretNameMaxLength = 256;
            static const int SecretVersionMaxLength = 256;
            static const int SecretValueMaxSize = 4 * 1024 * 1024; // 4MB

            static std::wstring const SecretResourceNameSeparator;
            static std::wstring const SecretResourceNameFormat;
        };
    }
}