// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test
{
    using System;
    using System.Fabric.Security;
    using System.Globalization;
    using System.Text;

    internal static class AccessControlUtility
    {
        #region Acl ToString

        public static string ToString(Acl acl)
        {
            var sb = new StringBuilder();
            sb.AppendFormat(CultureInfo.InvariantCulture, "Acl[AceCount={0}, AceEntries=", acl.AceItems.Count);
            sb.AppendLine();

            for (int i = 0; i < acl.AceItems.Count; i++)
            {
                sb.AppendFormat(CultureInfo.InvariantCulture, "#{0} {1}", i, ToString(acl.AceItems[i]));
                sb.AppendLine();
            }

            sb.AppendLine("]");
            return sb.ToString();
        }

        #endregion

        #region Ace ToString

        public static string ToString(Ace ace)
        {
            return string.Format(
                CultureInfo.InvariantCulture,
                "Ace[AceType={0}, Principal={1}, AccessMask={2}", ace.AceType, ToString(ace.Principal), ace.AccessMask);
        }

        #endregion

        #region ResourceIdentifier.ToString()

        public static string ToString(ResourceIdentifier resourceIdentifier)
        {
            switch (resourceIdentifier.Kind)
            {
                case ResourceIdentifierKind.Application:
                    return ToString((ApplicationResourceIdentifier)resourceIdentifier);
                case ResourceIdentifierKind.ApplicationType:
                    return ToString((ApplicationTypeResourceIdentifier)resourceIdentifier);
                case ResourceIdentifierKind.Name:
                    return ToString((NameResourceIdentifier)resourceIdentifier);
                case ResourceIdentifierKind.PathInFabricImageStore:
                    return ToString((FabricImageStorePathResourceIdentifier)resourceIdentifier);
                case ResourceIdentifierKind.Service:
                    return ToString((ServiceResourceIdentifier)resourceIdentifier);
                default:
                    return string.Format(CultureInfo.InvariantCulture, "Unknown ResourceIdentifier {0}", resourceIdentifier.Kind);
            }
        }

        public static string ToString(ApplicationTypeResourceIdentifier resourceIdentifier)
        {
            return string.Format(CultureInfo.InvariantCulture, "ApplicationTypeResourceIdentifier[ApplicationTypeName={0}]", resourceIdentifier.ApplicationTypeName);
        }

        public static string ToString(ApplicationResourceIdentifier resourceIdentifier)
        {
            return string.Format(CultureInfo.InvariantCulture, "ApplicationResourceIdentifier[ApplicationName={0}]", resourceIdentifier.ApplicationName);
        }

        public static string ToString(ServiceResourceIdentifier resourceIdentifier)
        {
            return string.Format(CultureInfo.InvariantCulture, "ServiceResourceIdentifier[ServiceName={0}]", resourceIdentifier.ServiceName);
        }

        public static string ToString(FabricImageStorePathResourceIdentifier resourceIdentifier)
        {
            return string.Format(CultureInfo.InvariantCulture, "FabricImageStorePathResourceIdentifier[Path={0}]", resourceIdentifier.Path);
        }

        public static string ToString(NameResourceIdentifier resourceIdentifier)
        {
            return string.Format(CultureInfo.InvariantCulture, "NameResourceIdentifier[Name={0}]", resourceIdentifier.Name);
        }

        #endregion

        #region PrincipalIdentifier.ToString()

        public static string ToString(PrincipalIdentifier principalIdentifier)
        {
            switch (principalIdentifier.Kind)
            {
                case PrincipalIdentifierKind.Claim:
                    return ToString((ClaimPrincipalIdentifier)principalIdentifier);
                case PrincipalIdentifierKind.Role:
                    return ToString((RolePrincipalIdentifier)principalIdentifier);
                case PrincipalIdentifierKind.Windows:
                    return ToString((WindowsPrincipalIdentifier)principalIdentifier);
                case PrincipalIdentifierKind.X509:
                    return ToString((X509PrincipalIdentifier)principalIdentifier);
                default:
                    return string.Format(CultureInfo.InvariantCulture, "Unknown PrincipalIdentifier {0}", principalIdentifier.Kind);
            }
        }

        public static string ToString(ClaimPrincipalIdentifier principalIdentifier)
        {
            return string.Format(CultureInfo.InvariantCulture, "ClaimPrincipalIdentifier[Claim={0}]", principalIdentifier.Claim);
        }

        public static string ToString(RolePrincipalIdentifier principalIdentifier)
        {
            return string.Format(CultureInfo.InvariantCulture, "RolePrincipalIdentifier[RoleName={0}]", principalIdentifier.RoleName);
        }

        public static string ToString(WindowsPrincipalIdentifier principalIdentifier)
        {
            return string.Format(CultureInfo.InvariantCulture, "WindowsPrincipalIdentifier[AccountName={0}]", principalIdentifier.AccountName);
        }

        public static string ToString(X509PrincipalIdentifier principalIdentifier)
        {
            return string.Format(CultureInfo.InvariantCulture, "X509PrincipalIdentifier[CommonName={0}]", principalIdentifier.CommonName);
        }

        #endregion
    }
}