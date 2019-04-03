// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace LBSimulator
{
    class FM;

    class DataParser
    {
        DENY_COPY(DataParser);

    public:
        static std::wstring const SectionNodeTypes;
        static std::wstring const SectionNode;
        static std::wstring const SectionService;
        static std::wstring const SectionServiceInsert;
        static std::wstring const SectionFailoverUnit;
        static std::wstring const SectionPlacement;
        static std::wstring const VectorStartDelimiter;
        static std::wstring const VectorDelimiter;

        static std::wstring const nodeTypeLabelArray[];
        static std::vector<std::wstring> const nodeTypeLabels;
        static std::wstring const nodeLabelArray[];
        static std::vector<std::wstring> const nodeLabels;
        static std::wstring const serviceLabelArray[];
        static std::vector<std::wstring> const serviceLabels;
        static std::wstring const failoverUnitLabelArray[];
        static std::vector<std::wstring> const failoverUnitLabels;
        static std::wstring const placementLabelArray[];
        static std::vector<std::wstring> const placementLabels;

        DataParser(FM & fm, Reliability::LoadBalancingComponent::PLBConfig & plbConfig);

        void Parse(std::wstring const& fileName);
        void CreateNode(std::map<std::wstring, std::wstring> & nodeLabelledVec, size_t index = -1);
        void CreateService(std::map<std::wstring, std::wstring> & nodeLabelledVec, size_t serviceIndex, bool generateFTAndReplicas = true);

    private:
        void ParseNodeTypesSection(std::wstring const & fileTextW, size_t & pos);
        void ParseNodeSection(std::wstring const & fileTextW, size_t & pos);
        void ParseServiceSection(std::wstring const & fileTextW, size_t & pos, bool insertNew);
        void ParseFailoverUnitSection(std::wstring const & fileTextW, size_t & pos);
        void ParsePlacementSection(std::wstring const & fileTextW, size_t & pos);
        bool ValidateTransition(std::wstring state);
        void ParseSections(std::wstring const & lineBuf, std::wstring const & fileTextW, size_t & pos);

        FM & fm_;
        Reliability::LoadBalancingComponent::PLBConfig & config_;
        std::set<std::pair<std::wstring, std::wstring>> stateMachine_;
        std::map<int, Common::Guid> failoverUnitIndexMap_;
        std::wstring currentState_;
    };
}
