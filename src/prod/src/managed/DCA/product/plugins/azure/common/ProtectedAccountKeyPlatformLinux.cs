// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA
{
    using System;
    using System.IO;
    using Newtonsoft.Json;
    using Newtonsoft.Json.Linq;

    internal class ProtectedAccountKeyPlatformLinux : IProtectedAccountKeyPlatform
    {
        internal const string jsonAccountKeyName = "DcaAccountKeys";

        public byte[] DecryptProtectedAccountKey(string protectedAccountKey)
        {
            byte[] bytes = new byte[protectedAccountKey.Length * sizeof(char)];
            Buffer.BlockCopy(protectedAccountKey.ToCharArray(), 0, bytes, 0, bytes.Length);
            return bytes;
        }

        public bool TryGetProtectedAccountKey(string protectedAccountKeyName, out string protectedAccountKey)
        {
            protectedAccountKey = null;

            try
            {
                if (string.IsNullOrEmpty(protectedAccountKeyName))
                {
                    return false;
                }

                using (StreamReader file = File.OpenText(AzureUploaderCommon.AzureFileStore.azureDCAInfoFileName))
                {
                    using (JsonTextReader reader = new JsonTextReader(file))
                    {
                        JToken jsonToken = JToken.ReadFrom(reader);
                        protectedAccountKey = jsonToken[jsonAccountKeyName][protectedAccountKeyName].ToString();
                    }
                }

                return !string.IsNullOrEmpty(protectedAccountKey);
            }
            catch (Exception e)
            {
                if (e is IOException || e is ArgumentException || e is NullReferenceException)
                {
                    return false;
                }

                throw;
            }
        }
    }
}