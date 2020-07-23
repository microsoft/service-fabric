// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.Common
{
    public interface IUniquelyIdentifiable
    {
        /// <summary>
        /// Gets the unique identifier which uniquely identifies this particular analysis container.
        /// </summary>
        /// <returns></returns>
        /// <remarks>
        /// The returned identifier should be able to uniquely identify the particular container across
        /// all objects of same type created across lifetime of the service. Think of this abstraction 
        /// as similar to GetHashCode however with GetHashCode, since base class provides a default 
        /// implementation, its easy for child classes to not provide a good implementation. At least
        /// an abstract method forces them to write their own implementation.
        /// </remarks>
        int GetUniqueIdentity();
    }
}