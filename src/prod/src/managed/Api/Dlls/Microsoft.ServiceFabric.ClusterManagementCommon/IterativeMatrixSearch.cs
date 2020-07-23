// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    using System;
    using System.Collections.Generic;
    using System.Text;

    internal class IterativeMatrixSearch
    {
        private readonly int row;
        private readonly int col;
        private readonly int rowTarget;
        private readonly int rowLowerAllowed;
        private readonly int rowUpperAllowed;
        private readonly int colTarget;
        private readonly int colLowerAllowed;
        private readonly int colUpperAllowed;
        private readonly Node[,] nodes;
        private readonly Range[] rowRanges;
        private readonly Range[] colRanges;
        private readonly bool[] rowRefresh;
        private readonly bool[] colRefresh;
        private int rowLowerCount;
        private int rowUpperCount;
        private int colLowerCount;
        private int colUpperCount;
        private Node current;

        public IterativeMatrixSearch(int[,] data, int target)
        {
            this.row = data.GetUpperBound(0) + 1;
            this.col = data.GetUpperBound(1) + 1;

            this.rowTarget = target / this.row;
            this.rowLowerAllowed = ((this.rowTarget + 1) * this.row) - target;
            this.rowUpperAllowed = this.row - this.rowLowerAllowed;
            this.rowLowerCount = this.rowUpperCount = 0;

            this.colTarget = target / this.col;
            this.colLowerAllowed = ((this.colTarget + 1) * this.col) - target;
            this.colUpperAllowed = this.col - this.colLowerAllowed;
            this.colLowerCount = this.colUpperCount = 0;

            this.nodes = new Node[this.row, this.col];
            this.rowRanges = new Range[this.row];
            this.colRanges = new Range[this.col];

            this.rowRefresh = new bool[this.row];
            this.colRefresh = new bool[this.col];

            for (var i = 0; i < this.row; i++)
            {
                var t = 0;
                for (var j = 0; j < this.col; j++)
                {
                    this.nodes[i, j] = new Node(data[i, j], i, j);
                    t += data[i, j];
                }

                this.rowRanges[i].Low = 0;
                this.rowRanges[i].High = t;
                this.rowRefresh[i] = true;

                if (t == this.rowTarget)
                {
                    this.rowLowerCount++;
                }
            }

            for (var j = 0; j < this.col; j++)
            {
                var t = 0;
                for (var i = 0; i < this.row; i++)
                {
                    t += data[i, j];
                }

                this.colRanges[j].Low = 0;
                this.colRanges[j].High = t;
                this.colRefresh[j] = true;

                if (t == this.colTarget)
                {
                    this.colLowerCount++;
                }
            }

            this.current = new Node(0, 0, -1);
        }

        public static int[,] Run(int[,] data, int target)
        {
            var row = data.GetUpperBound(0) + 1;
            var col = data.GetUpperBound(1) + 1;

            var search = new IterativeMatrixSearch(data, target);
            var resultNodes = search.Start();
            if (resultNodes == null)
            {
                return null;
            }

            var result = new int[row, col];
            for (var i = 0; i < row; i++)
            {
                for (var j = 0; j < col; j++)
                {
                    result[i, j] = resultNodes[i, j].Value.Low;
                }
            }

            return result;
        }

        public override string ToString()
        {
            var result = new StringBuilder();

            for (var i = 0; i < this.row; i++)
            {
                for (var j = 0; j < this.col; j++)
                {
                    result.AppendFormat("{0}\t", this.nodes[i, j].Value);
                }

                result.AppendFormat("({0})\n", this.rowRanges[i]);
            }

            for (var j = 0; j < this.col; j++)
            {
                result.AppendFormat("({0})\t", this.colRanges[j]);
            }

            result.AppendFormat("\nrow {0}/{1} col {2}/{3}", this.rowLowerCount, this.rowUpperCount, this.colLowerCount, this.colUpperCount);

            return result.ToString();
        }

        private void AdjustRange(Node node, Range value)
        {
            var oldRowLowerCount = this.rowLowerCount;
            var oldRowUpperCount = this.rowUpperCount;
            var oldColLowerCount = this.colLowerCount;
            var oldColUpperCount = this.colUpperCount;

            var d1 = value.Low - node.Value.Low;
            var d2 = value.High - node.Value.High;

            if (this.rowRanges[node.Row].High == this.rowTarget)
            {
                this.rowLowerCount--;
            }
            else if (this.rowRanges[node.Row].Low == this.rowTarget + 1)
            {
                this.rowUpperCount--;
            }

            this.rowRanges[node.Row].Low += d1;
            this.rowRanges[node.Row].High += d2;

            if (this.rowRanges[node.Row].High == this.rowTarget)
            {
                this.rowLowerCount++;
            }
            else if (this.rowRanges[node.Row].Low == this.rowTarget + 1)
            {
                this.rowUpperCount++;
            }

            if (this.colRanges[node.Col].High == this.colTarget)
            {
                this.colLowerCount--;
            }
            else if (this.colRanges[node.Col].Low == this.colTarget + 1)
            {
                this.colUpperCount--;
            }

            this.colRanges[node.Col].Low += d1;
            this.colRanges[node.Col].High += d2;

            if (this.colRanges[node.Col].High == this.colTarget)
            {
                this.colLowerCount++;
            }
            else if (this.colRanges[node.Col].Low == this.colTarget + 1)
            {
                this.colUpperCount++;
            }

            if ((oldRowLowerCount >= this.rowLowerAllowed) != (this.rowLowerCount >= this.rowLowerAllowed) ||
                (oldRowUpperCount >= this.rowUpperAllowed) != (this.rowUpperCount >= this.rowUpperAllowed))
            {
                for (var i = 0; i < this.row; i++)
                {
                    if (i != node.Row)
                    {
                        this.rowRefresh[i] = true;
                    }
                }
            }

            if ((oldColLowerCount >= this.colLowerAllowed) != (this.colLowerCount >= this.colLowerAllowed) ||
                (oldColUpperCount >= this.colUpperAllowed) != (this.colUpperCount >= this.colUpperAllowed))
            {
                for (var j = 0; j < this.col; j++)
                {
                    if (j != node.Col)
                    {
                        this.colRefresh[j] = true;
                    }
                }
            }
        }

        private bool ChangeValue(Node node, Range value)
        {
            this.AdjustRange(node, value);

            if (this.current.Links.Contains(node))
            {
                node.Value = value;
            }
            else
            {
                node = new Node(value, node);
                this.nodes[node.Row, node.Col] = node;
                this.current.Links.Add(node);
            }

            return this.rowLowerCount <= this.rowLowerAllowed && this.rowUpperCount <= this.rowUpperAllowed && this.colLowerCount <= this.colLowerAllowed && this.colUpperCount <= this.colUpperAllowed;
        }

        private Range GetRowTarget(int i)
        {
            if (this.rowRanges[i].Low == this.rowTarget + 1)
            {
                return new Range(this.rowTarget + 1, this.rowTarget + 1);
            }

            if (this.rowRanges[i].High == this.rowTarget)
            {
                return new Range(this.rowTarget, this.rowTarget);
            }

            if (this.rowLowerCount >= this.rowLowerAllowed)
            {
                return new Range(this.rowTarget + 1, this.rowTarget + 1);
            }

            if (this.rowUpperCount >= this.rowUpperAllowed)
            {
                return new Range(this.rowTarget, this.rowTarget);
            }

            return new Range(this.rowTarget, this.rowTarget + 1);
        }

        private Range GetColTarget(int j)
        {
            if (this.colRanges[j].Low == this.colTarget + 1)
            {
                return new Range(this.colTarget + 1, this.colTarget + 1);
            }

            if (this.colRanges[j].High == this.colTarget)
            {
                return new Range(this.colTarget, this.colTarget);
            }

            if (this.colLowerCount >= this.colLowerAllowed)
            {
                return new Range(this.colTarget + 1, this.colTarget + 1);
            }

            if (this.colUpperCount >= this.colUpperAllowed)
            {
                return new Range(this.colTarget, this.colTarget);
            }

            return new Range(this.colTarget, this.colTarget + 1);
        }

        private bool CheckRow(int i)
        {
            var targetRange = this.GetRowTarget(i);
            if (this.rowRanges[i].Disjoint(targetRange))
            {
                return false;
            }

            for (var j = 0; j < this.col; j++)
            {
                var node = this.nodes[i, j];
                if (!node.Value.IsFixed)
                {
                    var other = this.rowRanges[i].Subtract(node.Value);
                    var range = new Range(targetRange.Low - other.High, targetRange.High - other.Low).Overlap(node.Value);
                    if (range.Low != node.Value.Low || range.High != node.Value.High)
                    {
                        if (!this.ChangeValue(node, range))
                        {
                            return false;
                        }

                        this.colRefresh[j] = true;
                    }
                }
            }

            return true;
        }

        private bool CheckCol(int j)
        {
            var targetRange = this.GetColTarget(j);
            if (this.colRanges[j].Disjoint(targetRange))
            {
                return false;
            }

            for (var i = 0; i < this.row; i++)
            {
                var node = this.nodes[i, j];
                if (!node.Value.IsFixed)
                {
                    var other = this.colRanges[j].Subtract(node.Value);
                    var range = new Range(targetRange.Low - other.High, targetRange.High - other.Low).Overlap(node.Value);
                    if (range.Low != node.Value.Low || range.High != node.Value.High)
                    {
                        if (!this.ChangeValue(node, range))
                        {
                            return false;
                        }

                        this.rowRefresh[i] = true;
                    }
                }
            }

            return true;
        }

        private bool Refresh()
        {
            var found = true;

            while (found)
            {
                found = false;

                for (var i = 0; i < this.row; i++)
                {
                    if (this.rowRefresh[i])
                    {
                        this.rowRefresh[i] = false;
                        found = true;

                        if (!this.CheckRow(i))
                        {
                            return false;
                        }
                    }
                }

                for (var j = 0; j < this.col; j++)
                {
                    if (this.colRefresh[j])
                    {
                        this.colRefresh[j] = false;
                        found = true;

                        if (!this.CheckCol(j))
                        {
                            return false;
                        }
                    }
                }
            }

            return true;
        }

        private Node GetNextNode()
        {
            var i = this.current.Row;
            var j = this.current.Col;

            do
            {
                j++;
                if (j == this.col)
                {
                    j = 0;
                    if (++i >= this.row)
                    {
                        return null;
                    }
                }
            }
            while (this.nodes[i, j].Value.IsFixed);

            return this.nodes[i, j];
        }

        private Node[,] Start()
        {
            var path = new Stack<Node>();

            while (true)
            {
                if (this.Refresh())
                {
                    var node = this.GetNextNode();
                    if (node == null)
                    {
                        return this.nodes;
                    }

                    this.ChangeValue(node, new Range(node.Value.Low, node.Value.Low));
                    this.rowRefresh[node.Row] = this.colRefresh[node.Col] = true;

                    path.Push(this.current);
                    this.current = this.nodes[node.Row, node.Col];
                }
                else
                {
                    if (path.Count == 0)
                    {
                        return null;
                    }

                    foreach (var link in this.current.Links)
                    {
                        this.AdjustRange(link, link.Prev.Value);
                        this.nodes[link.Row, link.Col] = link.Prev;
                    }

                    this.current.Links.Clear();

                    var range = new Range(this.current.Value.Low + 1, this.current.Prev.Value.High);
                    this.AdjustRange(this.current, range);
                    this.current.Value = range;

                    this.rowRefresh[this.current.Row] = this.colRefresh[this.current.Col] = true;

                    this.current = path.Pop();
                }
            }
        }

        private struct Range
        {
            public int Low;
            public int High;

            public Range(int low, int high)
            {
                this.Low = low;
                this.High = high;
            }

            public bool IsFixed
            {
                get
                {
                    return this.Low == this.High;
                }
            }

            public bool Disjoint(Range other)
            {
                return this.Low > other.High || this.High < other.Low;
            }

            public Range Subtract(Range other)
            {
                return new Range(this.Low - other.Low, this.High - other.High);
            }

            public Range Overlap(Range other)
            {
                return new Range(Math.Max(this.Low, other.Low), Math.Min(this.High, other.High));
            }

            public override string ToString()
            {
                return string.Format("{0}-{1}", this.Low, this.High);
            }
        }

        private class Node
        {
            public readonly Node Prev;
            public readonly List<Node> Links;
            public readonly int Row;
            public readonly int Col;

            public Node(int value, int row, int col)
            {
                this.Value = new Range(0, value);
                this.Row = row;
                this.Col = col;
                this.Prev = null;
                this.Links = new List<Node>();
            }

            public Node(Range value, Node prev)
            {
                this.Value = value;
                this.Row = prev.Row;
                this.Col = prev.Col;
                this.Prev = prev;
                this.Links = new List<Node>();
            }

            public Range Value
            {
                get;
                set;
            }
        }
    }
}