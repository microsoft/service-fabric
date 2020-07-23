// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Testability.Service
{
    using System.Fabric.Common;
    using System.Fabric.Testability2;

    /// <summary>
    /// Returns Invoke data loss result object.
    /// </summary>
    /// <remarks>
    /// This class returns the SelectedPartition information for which invoke data loss action was called.
    /// </remarks>    
    [Serializable]
    public class InvokeDataLossResult
    {
        /// <summary>
        /// Invoke data loss result object constructor.
        /// </summary>
        /// <param name="selectedPartition">Selected partition</param>
        internal InvokeDataLossResult(SelectedPartition selectedPartition)
        {
            ReleaseAssert.AssertIf(selectedPartition == null, "Selected partition cannot be null");
            this.SelectedPartition = selectedPartition;
        }

        /// <summary>
        /// Gets the selected partition.
        /// </summary>
        public SelectedPartition SelectedPartition { get; private set; }
    }
}