// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System.Fabric;

    internal struct LogicalSequenceNumber
    {
        internal static readonly Epoch InvalidEpoch = new Epoch(-1, -1);

        internal static readonly Epoch ZeroEpoch = new Epoch(0, 0);

        internal static readonly LogicalSequenceNumber InvalidLsn = new LogicalSequenceNumber(-1);

        internal static readonly LogicalSequenceNumber ZeroLsn = new LogicalSequenceNumber(0);

        internal static readonly LogicalSequenceNumber OneLsn = new LogicalSequenceNumber(1);

        internal static readonly LogicalSequenceNumber MaxLsn = new LogicalSequenceNumber(long.MaxValue);

        private long lsn;

        internal LogicalSequenceNumber(long lsn)
        {
            this.lsn = lsn;
        }

        internal long LSN
        {
            get { return this.lsn; }
        }

        public static bool operator <(LogicalSequenceNumber left, LogicalSequenceNumber right)
        {
            return left.lsn < right.lsn;
        }

        public static bool operator >=(LogicalSequenceNumber left, LogicalSequenceNumber right)
        {
            return left.lsn >= right.lsn;
        }

        public static bool operator >(LogicalSequenceNumber left, LogicalSequenceNumber right)
        {
            return left.lsn > right.lsn;
        }

        public static bool operator <=(LogicalSequenceNumber left, LogicalSequenceNumber right)
        {
            return left.lsn <= right.lsn;
        }

        public static bool operator ==(LogicalSequenceNumber left, LogicalSequenceNumber right)
        {
            return left.lsn == right.lsn;
        }

        public static bool operator !=(LogicalSequenceNumber left, LogicalSequenceNumber right)
        {
            return left.lsn != right.lsn;
        }

        public static LogicalSequenceNumber operator +(LogicalSequenceNumber left, LogicalSequenceNumber right)
        {
            return new LogicalSequenceNumber(left.lsn + right.lsn);
        }

        public static LogicalSequenceNumber operator -(LogicalSequenceNumber left, LogicalSequenceNumber right)
        {
            return new LogicalSequenceNumber(left.lsn - right.lsn);
        }

        public static LogicalSequenceNumber operator +(LogicalSequenceNumber left, long right)
        {
            return new LogicalSequenceNumber(left.lsn + right);
        }

        public static LogicalSequenceNumber operator -(LogicalSequenceNumber left, long right)
        {
            return new LogicalSequenceNumber(left.lsn - right);
        }

        public static LogicalSequenceNumber operator ++(LogicalSequenceNumber operand)
        {
            operand.lsn++;
            return operand;
        }

        public static LogicalSequenceNumber operator --(LogicalSequenceNumber operand)
        {
            operand.lsn--;
            return operand;
        }

        public override bool Equals(object obj)
        {
            if ((obj is LogicalSequenceNumber) == false)
            {
                return false;
            }

            var arg = (LogicalSequenceNumber) obj;
            return this.lsn == arg.lsn;
        }

        public override int GetHashCode()
        {
            return this.lsn.GetHashCode();
        }

        public override string ToString()
        {
            return this.lsn.ToString();
        }
    }
}