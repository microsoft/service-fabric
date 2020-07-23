// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    using System.Fabric.Strings;

    internal static class ValidatorExtensions
    {
        public static void ThrowValidationExceptionIfNullOrEmpty(this string parameter, string parameterName)
        {
            if (string.IsNullOrWhiteSpace(parameter))
            {
                throw new ValidationException(
                    ClusterManagementErrorCode.BestPracticesAnalyzerModelInvalid,
                    StringResources.Error_BPARequiredParameter, 
                    parameterName);
            }
        }

        public static void ThrowValidationExceptionIfNull<T>(this T parameter, string parameterName)
        {
            if (parameter == null)
            {
                throw new ValidationException(
                        ClusterManagementErrorCode.BestPracticesAnalyzerModelInvalid,
                        StringResources.Error_BPARequiredParameter,
                        parameterName);
            }
        }
    }
}