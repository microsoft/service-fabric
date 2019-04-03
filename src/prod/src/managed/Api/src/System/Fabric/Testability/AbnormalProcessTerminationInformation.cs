// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability
{
    public class AbnormalProcessTerminationInformation : IEquatable<AbnormalProcessTerminationInformation>
    {
        public AbnormalProcessTerminationInformation()
        {
        }

        public AbnormalProcessTerminationInformation(string processName, string nodeID, DateTime failureTime, long exitCode)
        {
            this.ProcessName = processName;
            this.NodeID = nodeID;
            this.FailureTime = failureTime;
            this.ExitCode = exitCode;
        }

        public string ProcessName { get; internal set; }

        public string NodeID { get; internal set; }

        public DateTime FailureTime { get; internal set; }

        public long ExitCode { get; internal set; }

        public override bool Equals(object right)
        {
            if (ReferenceEquals(right, null))
            {
                return false;
            }

            if (ReferenceEquals(this, right))
            {
                return true;
            }

            if (this.GetType() != right.GetType())
            {
                return false;
            }

            return this.Equals(right as AbnormalProcessTerminationInformation);
        }

        public bool Equals(AbnormalProcessTerminationInformation other)
        {
            return string.Equals(this.ProcessName, other.ProcessName)
                   && string.Equals(this.NodeID, other.NodeID)
                   && DateTime.Equals(this.FailureTime, other.FailureTime)
                   && this.ExitCode == other.ExitCode;
        }

        public override int GetHashCode()
        {
            return (string.IsNullOrEmpty(this.ProcessName) ? 0 : this.ProcessName.GetHashCode())
                  ^ this.NodeID.GetHashCode()
                  ^ this.FailureTime.GetHashCode()
                  ^ this.ExitCode.GetHashCode();
        }

        public override string ToString()
        {
            return string.Format(
                "ProcessName : {0}, NodeID : {1}, FailureTime : {2}, ExitCode : {3} ",
                this.ProcessName,
                this.NodeID,
                this.FailureTime,
                this.ExitCode);
        }
    }
}

#pragma warning restore 1591