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
    /// Returns Restart partition result object.
    /// </summary>
    /// <remarks>
    /// This class returns the Selected partition for which restart partition action was called.
    /// </remarks>    
    [Serializable]
    public class RestartPartitionResult
    {
        /// <summary>
        /// Restart partition result constructor.
        /// </summary>
        /// <param name="selectedPartition">selected partition</param>
        internal RestartPartitionResult(SelectedPartition selectedPartition)
        {
            ReleaseAssert.AssertIfNull(selectedPartition, "Selected partition cannot be null");
            this.SelectedPartition = selectedPartition;
        }

        /// <summary>
        /// Gets selected partition.
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