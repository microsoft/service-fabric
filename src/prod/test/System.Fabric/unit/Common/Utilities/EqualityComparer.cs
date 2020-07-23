// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Description;
    using System.Linq;

    public class EqualityComparer<T> : IEqualityComparer<T>
    {
        private readonly Func<T, T, bool> compareFunc;

        public EqualityComparer(Func<T,T,bool> compareFunc)
        {
            this.compareFunc = compareFunc;
        }

        public bool Equals(T x, T y)
        {
            if ((x == null) && (y == null))
            {
                return true;
            }
            else if ((x == null) || (y == null))
            {
                return false;
            }
            else
            {
                return compareFunc(x, y);
            }
        }

        public int GetHashCode(T obj)
        {
            return obj.GetHashCode();
        }
    }

    public class ApplicationParameterEqualityComparer : EqualityComparer<ApplicationParameter>
    {
        public ApplicationParameterEqualityComparer()
            : base(ApplicationParameterEqualityComparer.CompareFunc)
        {
        }
        
        private static bool CompareFunc(ApplicationParameter x, ApplicationParameter y)
        {
            return ((x.Name == y.Name) && (x.Value == y.Value));   
        }
    }

    public class ApplicationParameterListEqualityComparer : EqualityComparer<ApplicationParameterList>
    {
        public ApplicationParameterListEqualityComparer()
            : base(ApplicationParameterListEqualityComparer.CompareFunc)
        {
        }

        private static bool CompareFunc(ApplicationParameterList x, ApplicationParameterList y)
        {
            if (x.Count != y.Count)
            {
                return false;
            }
            else
            {
                return x.OrderBy(p => p.Name).SequenceEqual(y.OrderBy(p => p.Name), new ApplicationParameterEqualityComparer());
            }
        }
    }
}