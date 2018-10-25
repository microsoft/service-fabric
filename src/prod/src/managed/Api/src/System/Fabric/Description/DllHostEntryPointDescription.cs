// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.Runtime.InteropServices;
    using System.Text;

    /// <summary>
    /// <para>Reserved for future use.</para>
    /// </summary>
    public sealed class DllHostEntryPointDescription : EntryPointDescription
    {
        internal DllHostEntryPointDescription()
            : base(CodePackageEntryPointKind.DllHost)
        {
            this.IsolationPolicy = DllHostIsolationPolicy.DedicatedProcess;
            this.HostedDlls = new List<DllHostHostedDllDescription>();
        }

        /// <summary>
        /// <para>Reserved for future use.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Fabric.Description.DllHostIsolationPolicy" />.</para>
        /// </value>
        public DllHostIsolationPolicy IsolationPolicy { get; internal set; }

        /// <summary>
        /// <para>Reserved for future use.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Collections.Generic.ICollection{T}" />.</para>
        /// </value>
        public ICollection<DllHostHostedDllDescription> HostedDlls { get; internal set; }

        /// <summary>
        /// <para> Reserved for future use.</para>
        /// </summary>
        /// <returns>
        /// <para>Returns <see cref="System.String" />.</para>
        /// </returns>
        public override string ToString()
        {
            StringBuilder builder = new StringBuilder();

            builder.AppendLine("DllHost(");
            builder.AppendLine(string.Format(CultureInfo.InvariantCulture, "IsolationPolicy={0},", this.IsolationPolicy));
            builder.AppendLine("HostedDlls=(");
            foreach (var item in this.HostedDlls)
            {
                builder.AppendLine(item.ToString());
            }

            builder.Append("))");

            return builder.ToString();
        }

        internal static new unsafe DllHostEntryPointDescription CreateFromNative(IntPtr nativeDescriptionPtr)
        {
            NativeTypes.FABRIC_DLLHOST_ENTRY_POINT_DESCRIPTION* nativeDescription =
                (NativeTypes.FABRIC_DLLHOST_ENTRY_POINT_DESCRIPTION*)nativeDescriptionPtr;

            DllHostIsolationPolicy isolationPolicy = DllHostIsolationPolicy.Invalid;
            switch (nativeDescription->IsolationPolicyType)
            {
                case NativeTypes.FABRIC_DLLHOST_ISOLATION_POLICY.FABRIC_DLLHOST_ISOLATION_POLICY_SHARED_DOMAIN:
                    isolationPolicy = DllHostIsolationPolicy.SharedDomain;
                    break;
                case NativeTypes.FABRIC_DLLHOST_ISOLATION_POLICY.FABRIC_DLLHOST_ISOLATION_POLICY_DEDICATED_DOMAIN:
                    isolationPolicy = DllHostIsolationPolicy.DedicatedDomain;
                    break;
                case NativeTypes.FABRIC_DLLHOST_ISOLATION_POLICY.FABRIC_DLLHOST_ISOLATION_POLICY_DEDICATED_PROCESS:
                    isolationPolicy = DllHostIsolationPolicy.DedicatedProcess;
                    break;
                default:
                    AppTrace.TraceSource.WriteError("DllHostEntryPointDescription.CreateFromNative", "Unknown isolation policy type : {0}", nativeDescription->IsolationPolicyType);
                    ReleaseAssert.Failfast(string.Format(CultureInfo.CurrentCulture, StringResources.Error_UnknownIsolationPolicyType_Formatted, nativeDescription->IsolationPolicyType));
                    break;
            }

            List<DllHostHostedDllDescription> hostedDlls = new List<DllHostHostedDllDescription>();
            if (nativeDescription->HostedDlls != IntPtr.Zero)
            {
                NativeTypes.FABRIC_DLLHOST_HOSTED_DLL_DESCRIPTION_LIST* nativeHostedDllDescriptionList =
                    (NativeTypes.FABRIC_DLLHOST_HOSTED_DLL_DESCRIPTION_LIST*)nativeDescription->HostedDlls;

                for (int i = 0; i < nativeHostedDllDescriptionList->Count; ++i)
                {
                    hostedDlls.Add(DllHostHostedDllDescription.CreateFromNative(
                        nativeHostedDllDescriptionList->Items +
                        (i * Marshal.SizeOf(typeof(NativeTypes.FABRIC_DLLHOST_HOSTED_DLL_DESCRIPTION)))));
                }
            }

            return new DllHostEntryPointDescription()
            {
                IsolationPolicy = isolationPolicy,
                HostedDlls = hostedDlls
            };
        }
    }
}