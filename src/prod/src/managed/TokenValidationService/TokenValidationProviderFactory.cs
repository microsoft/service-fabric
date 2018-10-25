// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.TokenValidationService
{
    using System;
    using System.Fabric.Common;
    using System.Globalization;

    internal sealed class TokenValidationProviderFactory
    {
        private const string TraceType = "TokenValidationProviderFactory";
        private const string ProvidersConfigName = "Providers";
        private const string DSTSProviderName = "DSTS";
        private const string AADProviderName = "AAD";

        public static ITokenValidationProvider Create(string configSection)
        {
            var nativeConfig = NativeConfigStore.FabricGetConfigStore(null);
            var providersValue = nativeConfig.ReadUnencryptedString(configSection, ProvidersConfigName);
            var providers = providersValue.Split(new char[]{ ',' }, StringSplitOptions.RemoveEmptyEntries);

            if (providers.Length > 1)
            {
                // TVS API currently has no way of specifying which provider should be
                // performing validation of the supplied token
                throw new FabricException(
                    "Multiple TVS validation providers not supported",
                    FabricErrorCode.InvalidConfiguration);
            }

            // DSTS is the default provider for backwards compatibility
            if (providers.Length < 1 || string.Equals(providers[0], DSTSProviderName, StringComparison.OrdinalIgnoreCase))
            {
                TokenValidationServiceFactory.TraceSource.WriteInfo(
                    TraceType,
                    "Creating {0} provider",
                    DSTSProviderName);

                return new DSTSValidationProvider(configSection);
            }
            else if (string.Equals(providers[0], AADProviderName, StringComparison.OrdinalIgnoreCase))
            {
                TokenValidationServiceFactory.TraceSource.WriteInfo(
                    TraceType,
                    "Creating {0} provider",
                    AADProviderName);

                return new AADValidationProvider(configSection);
            }
            else
            {
                throw new FabricException(string.Format(CultureInfo.InvariantCulture, 
                    "Invalid TVS provider '{0}'. Valid providers are '{1}' and '{2}'", 
                        providers[0],
                        DSTSProviderName,
                        AADProviderName),
                    FabricErrorCode.InvalidConfiguration);
            }
        }
    }
}