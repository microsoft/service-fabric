// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    using System;
    using System.Collections.Generic;

    internal class RandomMatrixSearch
    {
        private int[,] data;
        private int[,] existing;
        private int target;
        private int row;
        private int col;
        private int[,] result;
        private int[] rowCount;
        private int[] colCount;
        private Random rand;
        private Bucket rowBucket;
        private Bucket colBucket;
        private List<int> cx;
        private List<int> cy;

        private RandomMatrixSearch(int[,] data, int[,] existing, int target)
        {
            this.data = data;
            this.target = target;
            this.row = data.GetUpperBound(0) + 1;
            this.col = data.GetUpperBound(1) + 1;

            this.result = new int[this.row, this.col];
            this.rowCount = new int[this.row];
            this.colCount = new int[this.col];

            this.rand = new Random();
            this.cx = new List<int>();
            this.cy = new List<int>();

            this.existing = existing ?? new int[this.row, this.col];
        }

        public static int[,] Run(int[,] data, int[,] existing, int target, bool allowSuboptimal)
        {
            var search = new RandomMatrixSearch(data, existing, target);
            return search.Start(allowSuboptimal);
        }

        public int[,] Start(bool allowSuboptimal)
        {
            if (!this.Init(allowSuboptimal))
            {
                return null;
            }

            const double Decay = 0.9;
            const double C = 45000;
            const int Iteration = 150;

            var temp = 1.0;
            bool changed;

            do
            {
                changed = false;

                var f1 = this.rowBucket.Eval() + this.colBucket.Eval();
                for (var i = 0; i < Iteration; i++)
                {
                    if (f1 == 0)
                    {
                        return this.result;
                    }

                    var source = this.FindSource();
                    var dest = this.FindDestination(source);
                    if (source >= 0 && dest >= 0)
                    {
                        this.Update(source, dest);

                        var f2 = this.rowBucket.Eval() + this.colBucket.Eval();
                        if (f2 < f1 || Math.Exp(Math.Min(f1 - f2, -0.5) / (C * temp)) >= this.rand.NextDouble())
                        {
                            f1 = f2;
                            changed = true;
                        }
                        else
                        {
                            this.Update(dest, source);
                        }
                    }
                }

                temp *= Decay;
            }
            while (changed);

            if (allowSuboptimal)
            {
                return this.result;
            }

            return null;
        }

        private int FindSource()
        {
            var r = this.rand.Next(this.cx.Count);
            // ReSharper disable once ForCanBeConvertedToForeach - This is not used because there is no itemset, iteration is over cx.Count.
            for (var i = 0; i < this.cx.Count; i++)
            {
                int x = this.cx[r];
                int y = this.cy[r];
                if (this.result[x, y] > this.existing[x, y])
                {
                    return r;
                }

                r++;
                if (r == this.cx.Count)
                {
                    r = 0;
                }
            }

            return -1;
        }

        private int FindDestination(int source)
        {
            var r = this.rand.Next(this.cx.Count);
            // ReSharper disable once ForCanBeConvertedToForeach - This is not used because there is no itemset, iteration is over cx.Count.
            for (var i = 0; i < this.cx.Count; i++)
            {
                int x = this.cx[r];
                int y = this.cy[r];

                if (r != source && this.result[x, y] < this.data[x, y])
                {
                    return r;
                }

                r++;
                if (r == this.cx.Count)
                {
                    r = 0;
                }
            }

            return -1;
        }

        private void Update(int source, int dest)
        {
            var i1 = this.cx[source];
            var j1 = this.cy[source];
            var i2 = this.cx[dest];
            var j2 = this.cy[dest];

            this.result[i1, j1]--;
            this.result[i2, j2]++;

            if (i1 != i2)
            {
                this.rowBucket.Update(this.rowCount[i1], this.rowCount[i2]);
                this.rowCount[i1]--;
                this.rowCount[i2]++;
            }

            if (j1 != j2)
            {
                this.colBucket.Update(this.colCount[j1], this.colCount[j2]);
                this.colCount[j1]--;
                this.colCount[j2]++;
            }
        }

        private bool InitFixedValues()
        {
            int rowTarget = this.target / this.row;
            for (var i = 0; i < this.row; i++)
            {
                int count = 0;
                for (var j = 0; j < this.col; j++)
                {
                    count += this.data[i, j];
                }

                if (count < rowTarget)
                {
                    return false;
                }

                if (count == rowTarget)
                {
                    for (var j = 0; j < this.col; j++)
                    {
                        this.result[i, j] = this.data[i, j];
                    }
                }
            }

            var colTarget = this.target / this.col;
            for (var j = 0; j < this.col; j++)
            {
                int count = 0;
                for (var i = 0; i < this.row; i++)
                {
                    count += this.data[i, j];
                }

                if (count < colTarget)
                {
                    return false;
                }

                if (count == colTarget)
                {
                    for (var i = 0; i < this.row; i++)
                    {
                        this.result[i, j] = this.data[i, j];
                    }
                }
            }

            return true;
        }

        private bool Init(bool allowSuboptimal)
        {
            if (!allowSuboptimal)
            {
                if (!this.InitFixedValues())
                {
                    return false;
                }
            }

            int count = 0;
            int remain = 0;
            for (int i = 0; i < this.row; i++)
            {
                for (int j = 0; j < this.col; j++)
                {
                    if (this.result[i, j] == 0)
                    {
                        this.result[i, j] = this.existing[i, j];
                    }

                    count += this.result[i, j];

                    int n = this.data[i, j] - this.result[i, j];
                    if (n > 0)
                    {
                        this.cx.Add(i);
                        this.cy.Add(j);

                        remain += n;
                    }
                }
            }

            if (count + remain < this.target)
            {
                return false;
            }

            for (var i = count; i < this.target; i++)
            {
                var j = this.FindDestination(-1);
                this.result[this.cx[j], this.cy[j]]++;
            }

            for (var i = 0; i < this.row; i++)
            {
                for (var j = 0; j < this.col; j++)
                {
                    this.rowCount[i] += this.result[i, j];
                    this.colCount[j] += this.result[i, j];
                }
            }

            this.rowBucket = new Bucket(this.rowCount, this.target);
            this.colBucket = new Bucket(this.colCount, this.target);

            if (this.cx.Count <= 1 || count == this.target)
            {
                if (this.rowBucket.Eval() + this.colBucket.Eval() == 0)
                {
                    return true;
                }

                if (!allowSuboptimal)
                {
                    return false;
                }
            }

            return true;
        }

        private class Bucket
        {
            private int[] count;
            private int low;
            private int high;

            public Bucket(int[] data, int total)
            {
                this.count = new int[total + 1];

                this.low = this.high = data[0];
                foreach (int item in data)
                {
                    this.count[item]++;
                    this.high = Math.Max(this.high, item);
                    this.low = Math.Min(this.low, item);
                }
            }

            public void Update(int source, int dest)
            {
                this.count[source]--;
                this.count[source - 1]++;
                this.count[dest]--;
                this.count[dest + 1]++;

                this.low = Math.Min(this.low, source - 1);
                if (this.count[source] == 0 && source == this.high)
                {
                    this.high--;
                }

                this.high = Math.Max(this.high, dest + 1);
                if (this.count[dest] == 0 && dest == this.low)
                {
                    this.low++;
                }
            }

            public int Eval()
            {
                var i = this.low;
                var j = this.high;
                var r = 0;
                var c1 = this.count[i];
                var c2 = this.count[j];

                while (j - i > 1)
                {
                    var c = Math.Min(c1, c2);
                    r += (j - i - 1) * c;
                    if (c == c1)
                    {
                        c2 -= c1;
                        c1 = this.count[++i];
                    }
                    else
                    {
                        c1 -= c2;
                        c2 = this.count[--j];
                    }
                }

                return r;
            }
        }
    }
}