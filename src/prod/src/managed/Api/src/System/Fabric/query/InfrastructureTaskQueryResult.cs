// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Collections.ObjectModel;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Interop;

    [SuppressMessage("StyleCop.CSharp.OrderingRules", "SA1201:ElementsMustAppearInTheCorrectOrder", Justification = "Preserve order of public members from V1.")]
    [SuppressMessage("StyleCop.CSharp.OrderingRules", "SA1204:StaticElementsMustAppearBeforeInstanceElements", Justification = "Current grouping improves readability.")]
    [SuppressMessage("StyleCop.CSharp.OrderingRules", "SA1202:ElementsMustBeOrderedByAccess", Justification = "Current grouping improves readability.")]
    internal sealed class InfrastructureTaskQueryResult
    {
        #region Fields & Properties

        public Collection<InfrastructureTaskResultItem> Items { get; private set; }

        #endregion

        private InfrastructureTaskQueryResult()
        {
        }

        public static InfrastructureTaskQueryResult CreateFromNative(NativeInfrastructureService.IFabricInfrastructureTaskQueryResult native)
        {
            var managed = new InfrastructureTaskQueryResult();
            managed.Items = InfrastructureTaskResultItem.FromNativeQueryList(native.get_Result());
            return managed;
        }
    }
}