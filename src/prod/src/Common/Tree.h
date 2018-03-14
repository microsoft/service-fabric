// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Common/AssertWF.h"

namespace Common
{
    class TreeNodeIndex
    {
    public:
        TreeNodeIndex()
        {
        }

        TreeNodeIndex(std::vector<size_t> && index)
            : index_(std::move(index))
        {
        }

        __declspec(property(get=get_Length)) size_t Length;
        size_t get_Length() const { return index_.size(); }

        bool operator < (TreeNodeIndex const& other) const
        {
            return index_ < other.index_;
        }

        bool operator == (TreeNodeIndex const& other) const
        {
            return index_ == other.index_;
        }

        bool IsPrefixOf(TreeNodeIndex const& other) const
        {
            bool ret = true;
            if (this != &other)
            {
                if (Length > other.Length)
                {
                    ret = false;
                }
                else
                {
                    for (size_t i = 0; i < Length; ++i)
                    {
                        if (GetSegment(i) != other.GetSegment(i))
                        {
                            ret = false;
                            break;
                        }
                    }
                }
            }

            return ret;
        }

        void Append(size_t value)
        {
            index_.push_back(value);
        }

        size_t GetSegment(size_t segmentIndex) const
        {
            ASSERT_IF(segmentIndex > index_.size(), "Invalid segment index");
            return index_[segmentIndex];
        }

        TreeNodeIndex GetIndexPrefix(size_t len) const
        {
            std::vector<size_t> ret;

            ASSERT_IF(len > Length, "Prefix length should be <= index size");

            for (size_t i = 0; i < len; i++)
            {
                ret.push_back(GetSegment(i));
            }

            return TreeNodeIndex(move(ret));
        }

        void WriteTo(TextWriter& writer, FormatOptions const&) const
        {
            writer.Write("{0}", index_);
        }

    private:
        std::vector<size_t> index_;
    };


    template <class T>
    class Tree
    {
        DENY_COPY_ASSIGNMENT(Tree);

    public:
        class Node
        {
            DENY_COPY_ASSIGNMENT(Node);

        public:
            // create the tree structure according to a reference tree
            template <class TRef>
            static Node Create(
                typename Tree<TRef>::Node const& refNode,
                std::function<T(TRef const& ref)> creator)
            {
                // create children nodes first
                std::vector<Node> children;
                std::vector<typename Tree<TRef>::Node> const& refChildren = refNode.Children;
                for (size_t i = 0; i < refChildren.size(); i++)
                {
                    children.push_back(Create<TRef>(refChildren[i], creator));
                }

                return Node(creator(refNode.Data), std::move(children));
            }

            template <class TRef>
            static Node CreateEmpty(
                typename Tree<TRef>::Node const& refNode,
                std::function<T()> creator)
            {
                // create children nodes first
                std::vector<Node> children;
                std::vector<typename Tree<TRef>::Node> const& refChildren = refNode.Children;
                for (size_t i = 0; i < refChildren.size(); i++)
                {
                    children.push_back(CreateEmpty<TRef>(refChildren[i], creator));
                }

                return Node(creator(), std::move(children));
            }

            explicit Node(T && data)
                : data_(std::move(data))
            {
            }

            Node(T && data, std::vector<Node> && children)
                : data_(std::move(data)), children_(std::move(children))
            {
            }

            Node(Node const& other)
                : data_(other.data_), children_(other.children_)
            {
            }

            Node(Node && other)
                : data_(std::move(other.data_)), children_(std::move(other.children_))
            {
            }

            Node & operator = (Node && other)
            {
                if (this != &other)
                {
                    data_ = std::move(other.data_);
                    children_ = std::move(other.children_);
                }

                return *this;
            }

            void ForEachChild(std::function<void(Node const &)> processor) const
            {
                for_each(children_.begin(), children_.end(), processor);
            }

            void ForEachChild(std::function<void(Node &)> processor)
            {
                for_each(children_.begin(), children_.end(), processor);
            }

            void ForEachNodeInPostOrder(std::function<void(Node const &)> processor) const
            {
                for_each(children_.begin(), children_.end(), [&processor](Node const & node){
                    node.ForEachNodeInPostOrder(processor);
                });

                processor(*this);
            }

            void ForEachNodeInPostOrder(std::function<void(Node &)> processor)
            {
                for_each(children_.begin(), children_.end(), [&processor](Node & node){
                    node.ForEachNodeInPostOrder(processor);
                });

                processor(*this);
            }

            void ForEachNodeInPreOrder(std::function<void(Node const &)> processor) const
            {
                processor(*this);
                for_each(children_.begin(), children_.end(), [&processor](Node const & node){
                    node.ForEachNodeInPreOrder(processor);
                });
            }

            void ForEachNodeInPreOrder(std::function<void(Node &)> processor)
            {
                processor(*this);
                for_each(children_.begin(), children_.end(), [&processor](Node & node){
                    node.ForEachNodeInPreOrder(processor);
                });
            }

            __declspec(property(get=get_Data)) T const& Data;
            T const& get_Data() const { return data_; }

            __declspec(property(get=get_DataRef)) T & DataRef;
            T & get_DataRef() { return data_; }

            __declspec(property(get=get_Children)) std::vector<Node> const& Children;
            std::vector<Node> const& get_Children() const { return children_; }

            __declspec(property(get=get_ChildrenRef)) std::vector<Node> & ChildrenRef;
            std::vector<Node> & get_ChildrenRef() { return children_; }

        private:
            T data_;
            std::vector<Node> children_;
        };

        Tree()
            : root_(nullptr)
        {
        }

        explicit Tree(std::unique_ptr<Node> && root)
            : root_(std::move(root))
        {
        }

        Tree(Tree const& other)
            : root_(other.root_ == nullptr ? nullptr : make_unique<Node>(*(other.root_)))
        {
        }

        Tree(Tree && other)
            : root_(std::move(other.root_))
        {
        }

        Tree & operator = (Tree && other)
        {
            if (this != &other)
            {
                root_ = std::move(other.root_);
            }

            return *this;
        }

        __declspec(property(get=get_IsEmpty)) bool IsEmpty;
        bool get_IsEmpty() const { return root_ == nullptr; }


        __declspec(property(get=get_Root)) Node const& Root;
        Node const& get_Root() const { return *root_; }

        __declspec(property(get=get_RootRef)) Node & RootRef;
        Node & get_RootRef() { return *root_; }

        __declspec(property(get = get_LeavesCount)) size_t LeavesCount;
        size_t get_LeavesCount() const
        {
            if (root_ == nullptr) return 0;

            size_t numberOfLeaves = 0;
            ForEachNodeInPreOrder([&](Node const & node)
            {
                if (node.Children.size() == 0)
                {
                    numberOfLeaves++;
                }
            });

            return numberOfLeaves;
        }

        void SetRoot(std::unique_ptr<Node> && root)
        {
            root_ = move(root);
        }

        Node const& GetNodeByIndex(TreeNodeIndex const& nodeIndex) const
        {
            ASSERT_IF(root_ == nullptr, "GetNodeByIndex const: Tree is empty");
            Node const *currentNode = &(*root_);
            for (size_t i = 0; i < nodeIndex.Length; i++)
            {
                size_t index = nodeIndex.GetSegment(i);
                std::vector<Node> const& children = currentNode->Children;
                ASSERT_IF(index >= children.size(), "Tree node index not valid");
                currentNode = &(children[index]);
            }

            return *currentNode;
        }

        Node & GetNodeByIndex(TreeNodeIndex const& nodeIndex)
        {
            ASSERT_IF(root_ == nullptr, "GetNodeByIndex: Tree is empty");
            Node *currentNode = &(*root_);
            for (size_t i = 0; i < nodeIndex.Length; i++)
            {
                size_t index = nodeIndex.GetSegment(i);
                std::vector<Node> & children = currentNode->ChildrenRef;
                ASSERT_IF(index >= children.size(), "Tree node index not valid");
                currentNode = &(children[index]);
            }

            return *currentNode;
        }

        void ForEachNodeInPath(TreeNodeIndex const& nodeIndex, std::function<void(Node const &)> processor) const
        {
            ASSERT_IF(root_ == nullptr, "ForEachNodeInPath const: Tree is empty");
            Node const* currentNode = &(*root_);
            processor(*currentNode);
            for (size_t i = 0; i < nodeIndex.Length; i++)
            {
                size_t index = nodeIndex.GetSegment(i);
                std::vector<Node> & children = currentNode->Children;
                ASSERT_IF(index >= children.size(), "Tree node index not valid");
                currentNode = &(children[index]);
                processor(*currentNode);
            }
        }

        void ForEachNodeInPath(TreeNodeIndex const& nodeIndex, std::function<void(Node &)> processor)
        {
            ASSERT_IF(root_ == nullptr, "ForEachNodeInPath: Tree is empty");
            Node *currentNode = &(*root_);
            processor(*currentNode);
            for (size_t i = 0; i < nodeIndex.Length; i++)
            {
                size_t index = nodeIndex.GetSegment(i);
                std::vector<Node> & children = currentNode->ChildrenRef;
                ASSERT_IF(index >= children.size(), "Tree node index not valid");
                currentNode = &(children[index]);
                processor(*currentNode);
            }
        }

        void ForEachNodeInPreOrder(std::function<void(Node const &)> processor) const
        {
            ASSERT_IF(root_ == nullptr, "ForEachNodeInPreOrder: Tree is empty");

            Node const * currentNode = &(*root_);
            currentNode->ForEachNodeInPreOrder([&processor](Node const & refData)
            {
                processor(refData);
            });
        }

    private:

        std::unique_ptr<Node> root_;
    };
}
