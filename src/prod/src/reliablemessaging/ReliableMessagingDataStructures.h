// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "stdafx.h"

namespace ReliableMessaging
{
	/* 
		   There are two use cases for the KNodeWrapper template

		   1. As a KNode wrapper that can be used in constructing NOFAIL KNodeList and similar KTL data structures 
		      of ItemType objects when they do not contain the required KListEntry field for allocation-free linking

		   2. As a KNode wrapper that can be used in constructing NOFAIL KNodeList and similar KTL data structures 
		      of ItemType objects that do contain the required KListEntry field for allocation-free linking when that
			  field is already in use in a different data structure, e.g., a KHashTable

		*/

	template <class ItemType>
	class KNodeWrapper : public KObject<KNodeWrapper<ItemType>>, public KShared<KNodeWrapper<ItemType>>
	{
		K_FORCE_SHARED(KNodeWrapper<ItemType>)

	public:

		static const ULONG LinkOffset;

		KNodeWrapper(ItemType content)
		{
			content_ = content;
		}

		ItemType GetContent()
		{
			return content_;
		}

	private:

		ItemType content_;
		KListEntry qlink_;
	};


	/* 
		   The ResourcePool template class is used for managing pools of reusable ItemType instances.
		   
		   A set of ItemType instances is pre-allocated when the ResourcePool is constructed.  
		   The size of the initial pool is a constructor parameter.

		   The ItemType must derive from KShared<ItemType> and must support a static Create factory 
		   method with the signature
		   
			static Common::ErrorCode ItemType::Create(__in KAllocator& allocator, __out KSharedPtr<ItemType>& result)

		   which is used to populate the initial pool of ItemType instances as well as later instances 
		   if the initial pool runs out (currently not implementing the latter use case).
		   
		   Items are initialized outside of the pool functionality, however ItemType instances must
		   implement a parameterless Reuse method that is called to wipe ItemType instances when they 
		   are returned to the pool.

		   Pooled ItemType instances are held in a KNodeListShared structure.  As such they must contain 
		   a LIST_ENTRY element and must supply its FIELD_OFFSET to the constructor of the ResourcePool.

		*/




	template <class ItemType>
	class ResourcePool : public KObject<ResourcePool<ItemType>>
	{
	public:

		ResourcePool(KAllocator &allocator, ULONG const LinkOffset) : 
															pool_(LinkOffset),
															allocator_(allocator)
		{}

		// Initialize allocates real resources to the pool -- typically as part of a constructor or Open process
		// Failure to Initialize is meant to be a cause for constructor/Open failure, hence Initialize will clear 
		// the pool if the initial volume of resources cannot be allocated, before returning with failure
		Common::ErrorCode Initialize(LONG initialPoolSize, LONG increment)
		{
			initialPoolSize_ = initialPoolSize;
			increment_ = increment;

			for (LONG i=0; i<initialPoolSize_; i++)
			{
				KSharedPtr<ItemType> newItem = nullptr;
				Common::ErrorCode createResult = ItemType::Create(allocator_,newItem);

				if (!createResult.IsSuccess())
				{
					pool_.Reset();
					return Common::ErrorCodeValue::OutOfMemory;
				}

				pool_.AppendTail(newItem);
			}

			return Common::ErrorCodeValue::Success;
		}

		// Grow is typically called when the pool runs out of resources, depending on throttling policy
		// Grow is best effort -- it will return OutOfMemory if it cannot allocate thye full increment asked for 
		// but will not clear to pool of resources that have been successfully allocated
		Common::ErrorCode Grow(ULONG increment)
		{
			for (ULONG i=0; i<increment; i++)
			{
				KSharedPtr<ItemType> newItem = nullptr;
				Common::ErrorCode createResult = ItemType::Create(allocator_,newItem);

				if (!createResult.IsSuccess())
				{
					return Common::ErrorCodeValue::OutOfMemory;
				}

				K_LOCK_BLOCK(poolLock_)
				{
					pool_.AppendTail(newItem);
				}
			}

			return Common::ErrorCodeValue::Success;
		}

		// Shrink is typically called when the pool exceeds a policy maximum of resources
		// Grow is best effort -- it will remove and release at most the increment asked for 
		// but will stop without failure if the pool runs out before the increment is satisfied
		void Shrink(ULONG increment)
		{
			for (ULONG i=0; i<increment; i++)
			{
				KSharedPtr<ItemType> newItem = GetItem();

				if (newItem !- nullptr)
				{
					ItemType *itemToRelease = newItem.Detach();
					itemToRelease->Release();
				}
				else
				{
					// OK to shrink till we hit empty
					break;
				}
			}
		}

		KSharedPtr<ItemType> GetItem()
		{
			KSharedPtr<ItemType> itemToReturn;

			K_LOCK_BLOCK(poolLock_)
			{
				itemToReturn = pool_.RemoveHead();
			}

			return itemToReturn;
		}

		KSharedPtr<ItemType> GetItemGrowIfNeeded()
		{
			KSharedPtr<ItemType> item = GetItem();
			Common::ErrorCode growResult = Common::ErrorCodeValue::Success;

			while (item == nullptr && growResult.IsSuccess())
			{
				growResult = Grow(increment_);
				item = GetItem();
			}

			return item;
		}

		void ReturnItem(__in KSharedPtr<ItemType>& item)
		{
			item->Reuse();

			K_LOCK_BLOCK(poolLock_)
			{
				pool_.AppendTail(item);
			}
		}

	private:
		KNodeListShared<ItemType> pool_;
		LONG initialPoolSize_;
		LONG increment_;
		KSpinLock poolLock_;
		KAllocator &allocator_;
	};


	/* 
		
		The HashTableMap template class is a wrapper for KNodeHashTable that implements the patterns we need.

		It assumes that ItemType derives from KShared<ItemType> and is designed for use with KNodeHashTable.

		Specifically, HashTableMap
		   
		- manages AddRef and Release on successful insertion and deletion
		- always uses FALSE for ForceUpdate
		- uses simple bool success/failre reporting
		- will eventually transparently manage Resizing 
		   
	*/

	template <class KeyType, class ItemType>
	class HashTableMap : public KObject<HashTableMap<KeyType, ItemType>>
	{
	public:
    
		typedef KDelegate<ULONG(const KeyType& Key)> HashFunctionType;

		HashTableMap(
			__in ULONG size,
			__in HashFunctionType hashFunction,
			__in ULONG keyOffset,
			__in ULONG linkOffset,
			__in KAllocator& allocator
			)
			:   table_(size,hashFunction,keyOffset,linkOffset,allocator)
		{}

		bool Find(
				__in  KeyType& keyVal,
				__out ItemType*& item)
		{
			NTSTATUS result = table_.Get(keyVal,item);
			return (NT_SUCCESS(result));
		}

		bool Remove(
				__in ItemType *item)
		{
			NTSTATUS result = table_.Remove(item);

			if (NT_SUCCESS(result))
			{
				item->Release();
				return true;
			}
			else
			{
				return false;
			}
		}

		bool Insert(
				__in ItemType *newItem)
		{
			NTSTATUS result = table_.Put(newItem,FALSE);

			// the only failure status possible is collision
			if (NT_SUCCESS(result))
			{
				newItem->AddRef();
				return true;
			}
			else
			{
				ASSERT_IFNOT(result==STATUS_OBJECT_NAME_COLLISION, "Unexpected status returned from KNodeHashTable Put");
				return false;
			}
		}

	private:

		KNodeHashTable<KeyType, ItemType> table_;
	};

}
