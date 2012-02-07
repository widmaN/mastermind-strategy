#include <iostream>
#include <iomanip>
#include <cassert>

#include <stdlib.h>

#include "StrategyTree.h"
#include "Algorithm.hpp"

using namespace Mastermind;

///////////////////////////////////////////////////////////////////////////
// StrategyTreeNode implementation
//

StrategyTreeNode* StrategyTreeNode::Create(StrategyTreeMemoryManager *mm)
{
	assert(mm != NULL);

	//StrategyTreeNode *node = (StrategyTreeNode*)malloc(sizeof(StrategyTreeNode));
	StrategyTreeNode *node = mm->Alloc();
	node->m_depth = 0;
	node->m_totaldepth = 0;
	node->m_hits = 0;
	node->m_childcount = 0;
	return node;
}

void StrategyTreeNode::Destroy(StrategyTreeMemoryManager *mm, StrategyTreeNode *node)
{
	for (int i = 0; i < node->m_childcount; i++) {
		int j = node->m_childindex[i];
		StrategyTreeNode* child = node->m_children[j];
		if (child != Done()) {
			Destroy(mm, child);
		}
	}
	//node->m_depth = 0;
	//node->m_totaldepth = 0;
	//node->m_hits = 0;
	//node->m_childcount = 0;
	//free(node);
	mm->Free(node);
}

void StrategyTreeNode::AddChild(Feedback fb, StrategyTreeNode *child)
{
	int j = fb.value();
#ifndef NDEBUG
	for (int i = 0; i < m_childcount; i++) {
		assert(m_childindex[i] != j);
	}
#endif
	m_children[j] = child;
	m_childindex[m_childcount++] = j;

	if (child == Done()) {
		if (m_depth < 1)
			m_depth = 1;
		m_hits++;
		m_totaldepth += 1;
	} else {
		if (m_depth < (child->m_depth + 1)) {
			m_depth = child->m_depth + 1;
		} 
		m_hits += child->m_hits;
		m_totaldepth += (child->m_totaldepth + child->m_hits);
	}
}

int StrategyTreeNode::FillDepthInfo(int depth, int freq[], int max_depth) const
{
	int total = 0;
	for (int i = 0; i < m_childcount; i++) {
		int j = m_childindex[i]; 
		const StrategyTreeNode *child = m_children[j];
		if (child == Done()) { // this is the leaf
			if (depth > max_depth) {
				freq[max_depth]++;
			} else {
				freq[depth]++;
			}
			total += depth;
		} else {
			total += child->FillDepthInfo(depth + 1, freq, max_depth);
		}
	}
	return total;
}

int StrategyTreeNode::GetDepthInfo(int freq[], int max_depth) const
{
	memset(freq, 0, sizeof(int)*(max_depth+1));
	return FillDepthInfo(1, freq, max_depth);
}

void StrategyTreeNode::WriteToFile(std::ostream &os, FileFormat format, int indent) const
{
	assert(indent >= 0 && indent < 200);

	for (int i = 0; i < m_childcount; i++) 
	{
		int j = m_childindex[i];
		const StrategyTreeNode *child = m_children[j];
		if (format == XmlFormat) 
		{
			if (child == Done()) 
			{
				os << std::setw(indent) << "" << "<case feedback=\""
					<< Feedback(i) << "\">" << std::endl;
			} 
			else 
			{
				os << std::setw(indent) << "" << "<case "
					<< "feedback=\"" << Feedback(i) << "\" "
					<< "npos=\"" << child->State.NPossibilities << "\" "
					<< "ncand=\"" << child->State.NCandidates << "\" "
					<< "guess=\"" << child->State.Guess << "\">"
					<< std::endl;
			}
		} 
		else 
		{
			os << std::setw(indent) << "" 
				<< State.Guess << ":" << Feedback(i) << std::endl;
		}
		if (child != Done()) 
		{
			child->WriteToFile(os, format, indent + 2);
		}
		if (format == XmlFormat) 
		{
			os << std::setw(indent) << "" << "</case>" << std::endl;
		}
	}
}

void StrategyTreeNode::WriteToFile(
	std::ostream &os, FileFormat format, const CodewordRules &rules) const
{
	if (format == XmlFormat) 
	{
		os << "<mmstrat" 
			<< " npegs=" << '"' << rules.pegs() << '"'
			<< " ncolors=\"" << rules.colors() << '"'
			<< " allowrepetition=\"" << std::boolalpha << rules.repeatable() << '"'
			<< ">" << std::endl;
		
		const int max_depth = 100;
		int freq[max_depth+1];
		int total = GetDepthInfo(freq, max_depth);

		os << "<summary totalsteps=\"" << total << "\">" << std::endl;
		for (int i = 0; i <= max_depth; i++) 
		{
			if (freq[i] > 0) 
			{
				os << "  <where"
					<< " steps=\"" << i << '"'
					<< " count=\"" << freq[i] << '"'
					<< "/>" << std::endl;
			}
		}
		os << "</summary>\n";
		os << "<details>\n";
	}
	WriteToFile(os, format, 0);
	if (format == XmlFormat) 
	{
		os << "</details>" << std::endl;
		os << "</mmstrat>" << std::endl;
	}
}

#if 0
StrategyTreeNode* StrategyTreeNode::Single(StrategyTreeMemoryManager *mm, const Codeword& possibility)
{
	StrategyTreeNode *node = Create(mm);
	node->State.NPossibilities = 1;
	node->State.NCandidates = 1;
	node->State.Guess = possibility;
	// TBD
	node->AddChild(compare(CodewordRules(), possibility, possibility), StrategyTreeNode::Done());
	return node;
}
#endif
