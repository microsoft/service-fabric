// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Fabric.Strings;

    internal class ItemList<TItem> : Collection<TItem>
    {
        public ItemList()
            : this(false, false)
        {
        }

        public ItemList(IList<TItem> list)
            : this(false, false, list)
        {
        }

        public ItemList(bool mayContainNullValues, bool mayContainDuplicates)
            : base()
        {
            this.MayContainNullValues = mayContainNullValues;
            this.MayContainDuplicates = mayContainDuplicates;
        }

        public ItemList(bool mayContainNullValues, bool mayContainDuplicates, IList<TItem> list)
            : base(list)
        {
            this.MayContainNullValues = mayContainNullValues;
            this.MayContainDuplicates = mayContainDuplicates;
        }

        private bool MayContainNullValues
        {
            get;
            set;
        }

        private bool MayContainDuplicates
        {
            get;
            set;
        }

        public IList<TItem> AsReadOnly()
        {
            return new ReadOnlyCollection<TItem>(this);
        }

        protected override void InsertItem(int index, TItem item)
        {
            ValidateInsertOrAdd(index, item);
            base.InsertItem(index, item);
        }

        protected override void SetItem(int index, TItem item)
        {
            ValidateInsertOrAdd(index, item);
            base.InsertItem(index, item);
        }

        private void ValidateInsertOrAdd(int index, TItem item)
        {
            if (!this.MayContainNullValues)
            {
                Requires.Argument("item", item).NotNull();
            }

            if (!this.MayContainDuplicates)
            {
                int currentIndex = this.IndexOf(item);

                // The item either should not be in the list, or is overwritten with the same value
                if (currentIndex != -1 && currentIndex != index)
                {
                    throw new ArgumentException(
                        StringResources.Error_ItemAlreadyInList,
                        "item");
                }
            }
        }
    }
}