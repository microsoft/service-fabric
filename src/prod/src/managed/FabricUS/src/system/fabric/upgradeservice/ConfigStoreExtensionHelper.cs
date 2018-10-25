// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.UpgradeService
{
    using System.Fabric.Common;

    static class ConfigStoreExtensionHelper
    {
        private static readonly TraceType TraceType = new TraceType("ConfigStoreExtensionHelper");

        public static T Read<T>(this IConfigStore configStore, string sectionName, string parameterName, T defaultValue)
        {
            string parameterValue = configStore.ReadUnencryptedString(sectionName, parameterName);
            if (string.IsNullOrEmpty(parameterValue))
            {
                return defaultValue;
            }

            return (T)Convert.ChangeType(parameterValue, typeof(T));
        }

        public static TimeSpan ReadTimeSpan(this IConfigStore configStore, string sectionName, string parameterName, TimeSpan defaultValue)
        {
            string parameterValue = configStore.ReadUnencryptedString(sectionName, parameterName);
            if (string.IsNullOrEmpty(parameterValue))
            {
                return defaultValue;
            }

            int parameterValueInSeconds = 0;
            if (!int.TryParse(parameterValue, out parameterValueInSeconds))
            {
                Trace.WriteWarning(TraceType, "Value {0} from SectionName:{1} and ParameterName:{2} is not valid.", parameterValue, sectionName, parameterName);
                ReleaseAssert.Fail("Value {0} from SectionName:{1} and ParameterName:{2} is not valid.", parameterValue, sectionName, parameterName);
            }

            return TimeSpan.FromSeconds(parameterValueInSeconds);
        }
    }
}