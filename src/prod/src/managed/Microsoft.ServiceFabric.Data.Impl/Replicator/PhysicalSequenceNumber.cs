// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    internal struct PhysicalSequenceNumber
    {
        internal static readonly PhysicalSequenceNumber InvalidPsn = new PhysicalSequenceNumber(-1);
        private long psn;

        internal PhysicalSequenceNumber(long psn)
        {
            this.psn = psn;
        }

        internal long PSN
        {
            get { return this.psn; }
        }

        public static bool operator <(PhysicalSequenceNumber left, PhysicalSequenceNumber right)
        {
            return left.psn < right.psn;
        }

        public static bool operator >=(PhysicalSequenceNumber left, PhysicalSequenceNumber right)
        {
            return left.psn >= right.psn;
        }

        public static bool operator >(PhysicalSequenceNumber left, PhysicalSequenceNumber right)
        {
            return left.psn > right.psn;
        }

        public static bool operator <=(PhysicalSequenceNumber left, PhysicalSequenceNumber right)
        {
            return left.psn <= right.psn;
        }

        public static bool operator ==(PhysicalSequenceNumber left, PhysicalSequenceNumber right)
        {
            return left.psn == right.psn;
        }

        public static bool operator !=(PhysicalSequenceNumber left, PhysicalSequenceNumber right)
        {
            return left.psn != right.psn;
        }

        public static PhysicalSequenceNumber operator +(PhysicalSequenceNumber left, PhysicalSequenceNumber right)
        {
            return new PhysicalSequenceNumber(left.psn + right.psn);
        }

        public static PhysicalSequenceNumber operator -(PhysicalSequenceNumber left, PhysicalSequenceNumber right)
        {
            return new PhysicalSequenceNumber(left.psn - right.psn);
        }

        public static PhysicalSequenceNumber operator +(PhysicalSequenceNumber left, long right)
        {
            return new PhysicalSequenceNumber(left.psn + right);
        }

        public static PhysicalSequenceNumber operator -(PhysicalSequenceNumber left, long right)
        {
            return new PhysicalSequenceNumber(left.psn - right);
        }

        public static PhysicalSequenceNumber operator ++(PhysicalSequenceNumber operand)
        {
            operand.psn++;
            return operand;
        }

        public static PhysicalSequenceNumber operator --(PhysicalSequenceNumber operand)
        {
            operand.psn--;
            return operand;
        }

        public override bool Equals(object obj)
        {
            if ((obj is PhysicalSequenceNumber) == false)
            {
                return false;
            }

            var arg = (PhysicalSequenceNumber) obj;
            return this.psn == arg.psn;
        }

        public override int GetHashCode()
        {
            return this.psn.GetHashCode();
        }

        public override string ToString()
        {
            return this.psn.ToString();
        }
    }
}