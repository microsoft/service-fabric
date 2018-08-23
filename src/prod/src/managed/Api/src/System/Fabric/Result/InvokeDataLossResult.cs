// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Result
{
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Text;

    /// <summary>
    /// Returns Invoke data loss result object.
    /// </summary>
    /// <remarks>
    /// This class returns the SelectedPartition information for which invoke data loss action was called.
    /// </remarks>
    [Serializable]
    public sealed class InvokeDataLossResult
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
        /// <value>The partition selected by the test command.</value>
        public SelectedPartition SelectedPartition { get; private set; }

        /// <summary>
        /// Returns the string representation of the contained information.
        /// </summary>
        /// <returns>A string containing information about the SelectedPartition 
        /// and (conditionally) about the Exception </returns>
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder();

            sb.AppendFormat("SelectedPartition: {0}", this.SelectedPartition);

            return sb.ToString();
        }
    }
}