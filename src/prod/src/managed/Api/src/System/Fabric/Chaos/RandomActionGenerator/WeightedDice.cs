// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Chaos.RandomActionGenerator
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Linq;

    /// <summary>
    /// This class will be helpful in generating random weighted outcomes.
    /// Basically it will be used to choose one of the several options with their associated weight/probability ratio.
    /// </summary>
    class WeightedDice<T>
    {
        // Maps each face to it's weight.
        private readonly Dictionary<T, double> faceToWeight;

        // Keys are in strictly increasing order. Where keys are incremented by weight of it's corresponding value--face's weight.
        // This is the most important data structure this class has to figure out which face/to show on top
        private readonly SortedList<double, T> rangeTree;
        private double maxRange = 0;
        private Random random;

        public WeightedDice(Random random)
        {
            this.random = random;
            this.faceToWeight = new Dictionary<T, double>();
            this.rangeTree = new SortedList<double, T>();
        }

        /// <summary>
        /// Returns a randomly chosen face. Probability of any face is proportional to it's weight.
        /// </summary>
        /// <returns>Returns a randomly chosen face.</returns>
        public T NextRoll()
        {
            double arbiter = this.random.NextDouble();
            double searchPoint = arbiter * this.maxRange; // Normalize the random number.
            T face = this.rangeTree.FirstOrDefault(predicate: pair => pair.Key > searchPoint).Value;
            return face;
        }

        /// <summary>
        /// Adds a new face to the dice.
        /// </summary>
        /// <param name="faceId">The id of the face.</param>
        /// <param name="weight">The weight of the dice.</param>
        /// <returns></returns>
        public void AddNewFace(T faceId, double weight)
        {
            ReleaseAssert.AssertIf(this.faceToWeight.ContainsKey(faceId), "Face {0} already exists.", faceId);
            this.faceToWeight[faceId] = weight;
            this.UpdateRangeTree();
        }

        /// <summary>
        /// Update the range tree.
        /// </summary>
        private void UpdateRangeTree()
        {
            this.rangeTree.Clear();
            this.maxRange = 0;

            foreach (var categroryWeightPair in this.faceToWeight)
            {
                if (categroryWeightPair.Value > 0)
                {
                    this.maxRange += categroryWeightPair.Value;
                    this.rangeTree.Add(this.maxRange, categroryWeightPair.Key);
                }
            }
        }
    }
}