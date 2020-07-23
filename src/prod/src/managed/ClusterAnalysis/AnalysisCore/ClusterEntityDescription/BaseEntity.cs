// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.ClusterEntityDescription
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Runtime.Serialization;
    using ClusterAnalysis.Common;

    /// <summary>
    /// </summary>
    [DataContract]
    [KnownType("GetKnownTypes")]
    public abstract class BaseEntity : IEquatable<BaseEntity>, IUniquelyIdentifiable
    {
        private static List<Type> knownTypes;

        /// <inheritdoc />
        public abstract bool Equals(BaseEntity other);

        /// <inheritdoc />
        public override bool Equals(object obj)
        {
            return this.Equals(obj as BaseEntity);
        }

        /// <inheritdoc />
        public override int GetHashCode()
        {
            return base.GetHashCode();
        }

        /// <inheritdoc />
        public abstract int GetUniqueIdentity();

        internal static IList<Type> GetKnownTypes()
        {
            if (knownTypes != null)
            {
                return knownTypes;
            }

            knownTypes = new List<Type> { typeof(BaseEntity) };
            var assemblies = AppDomain.CurrentDomain.GetAssemblies();
            foreach (var assembly in assemblies)
            {
                try
                {
                    var types = assembly.GetTypes();
                    knownTypes.AddRange(types.Where(type => type.IsSubclassOf(typeof(BaseEntity))));
                }
                catch (Exception)
                {
                    // ignored
                }
            }

            return knownTypes.ToArray();
        }
    }
}