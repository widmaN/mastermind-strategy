#include <iostream>
#include <iomanip>
#include <cassert>

//#include <stdlib.h>
#include <algorithm>
#include <utility>
#include "util/io_format.hpp"

#include "StrategyTree.hpp"
#include "Algorithm.hpp"

using namespace Mastermind;

///////////////////////////////////////////////////////////////////////////
// StrategyTreeNode implementation
//

namespace Mastermind {

std::ostream& operator << (std::ostream &os, const StrategyTreeInfo &info)
{
	const int max_display = 10;

	// Display header.
	if (os.iword(util::header_index()))
	{
		os << "Stategy Statistics" << std::endl
		   << "--------------------" << std::endl
		   << "Strategy: Total   Avg    1    2    3    4    5    6    7    8    9   >9   Time" << std::endl;
		os << util::noheader;
	}

	// Display label.
	os  << std::setw(8) << info.name() << ":"
		<< std::setw(6) << info.total_depth() << " "
		<< std::setw(5) << std::setprecision(3)
		<< std::fixed << info.average_depth() << ' ';

	// Display each frequency.
	unsigned int running_total = 0;
	for (int d = 1; d <= max_display; d++) 
	{
		unsigned int count;
		if (d < max_display)
		{
			count = info.count_depth(d);
			running_total += d * count;
		}
		else
		{
			count = info.total_depth() - running_total;
		}
		if (count > 0)
			os << std::setw(4) << count << ' ';
		else
			os << "   - ";
	}

	// Display time
	os  << std::fixed << std::setw(6) << std::setprecision(2) 
		<< info.time() << std::endl;

	return os;
}

#if 0
void StrategyTree::AddChild(const Node &node)
{
#if 1
	nodes.push_back(node);
#else

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
#endif
}
#endif

unsigned int StrategyTree::getDepthInfo(
	unsigned int depth_freq[], 
	unsigned int count) const
{
	Feedback perfect = Feedback::perfectValue(_rules);
	unsigned int total = 0;
	std::fill(depth_freq + 0, depth_freq + count, 0);
	for (size_t i = 0; i < _nodes.size(); ++i)
	{
		if (_nodes[i].response() == perfect)
		{
			unsigned int d = _nodes[i].depth();
			total += d;
			++depth_freq[std::min(d, count) - 1];
		}
	}
	return total;
}

#if 0
template <>
void WriteToFile<TextFormat>(std::ostream &os, const StrategyTree &tree)
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
#endif

template <>
void WriteToFile<XmlFormat>(std::ostream &os, const StrategyTree &tree)
{
	Rules rules = tree.rules();

	// Output XML header.
	os << "<mastermind-strategy" 
		<< " pegs=" << '"' << rules.pegs() << '"'
		<< " colors=\"" << rules.colors() << '"'
		<< " repeatable=\"" << std::boolalpha << rules.repeatable() << '"'
		<< ">" << std::endl;
	
	// Output summary.
	const int max_depth = 100;
	unsigned int freq[max_depth];
	unsigned int total = tree.getDepthInfo(freq, max_depth);

	os << "<summary totalsteps=\"" << total << "\">" << std::endl;
	for (int i = 1; i <= max_depth; i++) 
	{
		if (freq[i-1] > 0) 
		{
			os << "  <where"
				<< " steps=\"" << i << '"'
				<< " count=\"" << freq[i-1] << '"'
				<< "/>" << std::endl;
		}
	}
	os << "</summary>" << std::endl;

	// Output the strategy.
	os << "<details>" << std::endl;

	Feedback target = Feedback::perfectValue(tree.rules());
	int indent = 2;
	int level = 0;
	for (size_t i = 0; i < tree.nodes().size(); ++i)
	{
		const StrategyTree::Node &node = tree.nodes()[i];

		// Close deeper branches.
		for (; level > node.depth(); --level)
		{
			os << std::setw(indent*level) << "" << "</case>" << std::endl;
		}
		level = node.depth();

		if (node.response() == target)
		{
			os << std::setw(indent*level) << "" << "<state guess=\"" 
				<< node.guess() << "\" feedback=\"" << node.response()
				<< "\"/>" << std::endl;
		}
		else 
		{
			os << std::setw(indent*level) << "" << "<state "
				<< "guess=\"" << node.guess() << "\" "
				<< "feedback=\"" << node.response() << "\" "
				//<< "npos=\"" << node.npossibilities << "\" "
				//<< "ncand=\"" << node.ncandidates << "\" "
				//<< "next=\"" << node.suggestion << "\">"
				<< std::endl;
		}
	}

	// Write closing tags.
	os << "</details>" << std::endl;
	os << "</mastermind-strategy>" << std::endl;
}

} // namespace Mastermind

#if 0
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
	std::ostream &os, FileFormat format, const Rules &rules) const
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
	node->AddChild(compare(Rules(), possibility, possibility), StrategyTreeNode::Done());
	return node;
}
#endif
#endif
