// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{    
    internal sealed class EnableEndpointV2Utility
    {        
        public static bool GetValue(NativeConfigStore configStore)
        {
            ThrowIf.Null(configStore, "configStore");

            string value = configStore.ReadUnencryptedString("Common", "EnableEndpointV2");
            bool flag = string.IsNullOrEmpty(value) ? false : bool.Parse(value);            
            return flag;
        }
    }
}