// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.WindowsFabricValidator
{
    using System.Fabric.Management.ServiceModel;
    using System.Security;

    internal static class SettingsOverridesTypeSectionParameterExtension
    {
        public static SecureString GetSecureValue(this SettingsOverridesTypeSectionParameter parameter, string storeName)
        {
            if (parameter.IsEncrypted)
            {
                return FabricValidatorUtility.DecryptValue(storeName, parameter.Value);
            }
            else
            {
                if(string.IsNullOrEmpty(parameter.Value))
                {
                    return null;
                }

                return FabricValidatorUtility.CharArrayToSecureString(parameter.Value.ToCharArray());
            }
        }
    }
}