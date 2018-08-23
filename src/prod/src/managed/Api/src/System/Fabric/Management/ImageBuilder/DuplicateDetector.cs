// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageBuilder
{
    using System.Collections.Generic;
    using System.Fabric.Strings;

    internal class DuplicateDetector
    {
        private static readonly string TraceType = "DuplicateDetector";

        private HashSet<string> hashSet;
        private string elementName;
        private string propertyName;
        private string fileName;

        public DuplicateDetector(string elementName, string propertyName)
            : this(elementName, propertyName, null, true /*ignoreCase*/)
        {
        }

        public DuplicateDetector(string elementName, string propertyName, string fileName)
            : this(elementName, propertyName, fileName, true /*ignoreCase*/)
        {
        }

        public DuplicateDetector(string elementName, string propertyName, string fileName, bool ignoreCase)
        {
            this.elementName = elementName;
            this.propertyName = propertyName;
            this.fileName = fileName;

            if (ignoreCase)
            {
                this.hashSet = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            }
            else
            {
                this.hashSet = new HashSet<string>(StringComparer.Ordinal);
            }
        }

        public void Add(string name)
        {
            if (this.hashSet.Contains(name))
            {
                if(string.IsNullOrEmpty(this.fileName))
                {
                    ImageBuilderUtility.TraceAndThrowValidationError(
                        TraceType,                        
                        StringResources.ImageBuilderError_DuplicateElementFound,
                        this.elementName,
                        this.propertyName,
                        name);
                }
                else
                {
                    ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                        TraceType,
                        this.fileName,
                        StringResources.ImageBuilderError_DuplicateElementFound,
                        this.elementName,
                        this.propertyName,
                        name);
                }
            }

            this.hashSet.Add(name);
        }
    }
}