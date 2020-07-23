// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Autopilot
{
    internal class StringConstants
    {
        public const string ApplicationConfigurationFileName = "Config.ini";

        public const string EnvironmentSyncConfigurationSectionName = "ServiceFabricEnvironmentSync";

        public const string CertificateManagerConfigurationSectionName = "ServiceFabricEnvironmentSync.Certificate";

        public const string CertificateListConfigurationSectionNameTemplate = "ServiceFabricEnvironmentSync.Certificate.LocalMachine.{0}";

        public const string EncryptedPrivateKeyFileNameTemplate = "{0}.pfx.encr";

        public const string EncryptedPasswordFileNameTemplate = "{0}.pfx.password.encr";

        public const string PublicKeyFileNameTemplate = "{0}.cer";

        public const string CustomLogIdName = "ServiceFabricEnvironmentSync";
    }
}