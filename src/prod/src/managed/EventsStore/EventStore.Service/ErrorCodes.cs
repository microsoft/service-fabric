// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventStore.Service
{
    internal static class ErrorCodes
    {
        public const int GenericError = -1;

        public const int InvalidArgs = -2;

        public const int EntityTypeNotSupported = -3;

        public const int AzureConnectionInformationMissing = -4;

        public const int AzureAccountKeyInvalid = -5;

        public const int AzureAccountKeyMissing = -6;

        public const int AzureAccountNameMissing = -7;

        public const int AzureAccountKeyDecryptionFailed = -8;

        public const int AzureAccountStringParsingFailed = -9;

        public const int ParamMissingOutputFile = -10;

        public const int ParamMissingGetQueryKey = -11;

        public const int ParamInvalidOutputDir = -12;

        public const int GenericArgumentException = -13;
    }
}
