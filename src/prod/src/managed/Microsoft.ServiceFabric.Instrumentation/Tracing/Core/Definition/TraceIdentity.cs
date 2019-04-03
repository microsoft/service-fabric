// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition
{
    using System;
    using System.Globalization;

    /// <summary>
    /// Identifier for a Trace
    /// </summary>
    /// <remarks>
    /// Today we use {Id, TaskName} as the unique identifier for a trace. We can potentially
    /// add Operation Code to make it more robust, however not needed right now.
    /// </remarks>
    public class TraceIdentity
    {
        public TraceIdentity(int id, TaskName name)
        {
            if (id < 0)
            {
                throw new ArgumentOutOfRangeException("id");
            }

            this.TraceId = id;
            this.TaskName = name;
        }

        /// <summary>
        /// Id of the Trace
        /// </summary>
        public int TraceId { get; }

        /// <summary>
        /// Task who is writing the Trace
        /// </summary>
        public TaskName TaskName { get; }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "ID: {0}, TaskName: {1}", this.TraceId, this.TaskName);
        }

        public override bool Equals(object obj)
        {
            var other = obj as TraceIdentity;
            if (other == null)
            {
                return false;
            }

            return this.TraceId == other.TraceId && this.TaskName == other.TaskName;
        }

        public override int GetHashCode()
        {
            return this.TraceId << 8 | (ushort)this.TaskName;
        }
    }
}