// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Describes the RunAsPolicy associated with a CodePackage specified in application manifest. </para>
    /// </summary>
    public sealed class RunAsPolicyDescription
    {
        RunAsPolicyDescription()
        {
        }

        /// <summary>
        /// <para>Gets or sets the UserName specified in RunAsPolicy associated with a CodePackage.</para>
        /// </summary>
        /// <value>
        /// <para>The UserName specified in RunAsPolicy associated with a CodePackage.</para>
        /// </value>
        public string UserName
        {
            get;
            private set;
        }

        internal static unsafe RunAsPolicyDescription CreateFromNative(NativeTypes.FABRIC_RUNAS_POLICY_DESCRIPTION nativeDescription)
        {
            return new RunAsPolicyDescription()
            {
                UserName = NativeTypes.FromNativeString(nativeDescription.UserName)
            };
        }
    }
}