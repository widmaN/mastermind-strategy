#include <stdlib.h>

#include "StrategyTree.h"

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
	int j = fb.GetValue();
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

void StrategyTreeNode::WriteToFile(
	FILE *fp, 
	FileFormat format,
	int indent) const
{
	assert(fp != NULL);
	assert(indent >= 0 && indent < 200);

	for (int i = 0; i < m_childcount; i++) {
		int j = m_childindex[i];
		const StrategyTreeNode *child = m_children[j];
		if (format == XmlFormat) {
			if (child == Done()) {
				fprintf(fp, "%*s<case feedback=\"%s\">\n",
					indent, "",
					Feedback(i).ToString().c_str());
			} else {
				fprintf(fp, "%*s<case feedback=\"%s\" npos=\"%d\" ncand=\"%d\" guess=\"%s\">\n",
					indent, "",
					Feedback(i).ToString().c_str(),
					child->State.NPossibilities,
					child->State.NCandidates,
					child->State.Guess.ToString().c_str());
			}
		} else {
			fprintf(fp, "%*s%s:%s\n", 
					indent, "",
					State.Guess.ToString().c_str(),
					Feedback(i).ToString().c_str());
		}
		if (child != Done()) {
			child->WriteToFile(fp, format, indent + 2);
		}
		if (format == XmlFormat) {
			fprintf(fp, "%*s</case>\n", indent, "");
		}
	}
}

void StrategyTreeNode::WriteToFile(
	FILE *fp, 
	FileFormat format,
	CodewordRules rules) const
{
	if (format == XmlFormat) {
		fprintf(fp, "<mmstrat npegs=\"%d\" ncolors=\"%d\" allowrepetition=\"%s\">\n",
			rules.length, rules.ndigits, (rules.allow_repetition? "true":"false"));
		
		const int max_depth = 100;
		int freq[max_depth+1];
		int total = GetDepthInfo(freq, max_depth);

		fprintf(fp, "<summary totalsteps=\"%d\">\n", total);
		for (int i = 0; i <= max_depth; i++) {
			if (freq[i] > 0) {
				fprintf(fp, "  <where steps=\"%d\" count=\"%d\"/>\n",
					i, freq[i]);
			}
		}
		fprintf(fp, "</summary>\n");

		fprintf(fp, "<details>\n");
	}
	WriteToFile(fp, format, 0);
	if (format == XmlFormat) {
		fprintf(fp, "</details>\n");
		fprintf(fp, "</mmstrat>\n");
	}
}

StrategyTreeNode* StrategyTreeNode::Single(StrategyTreeMemoryManager *mm, const Codeword& possibility)
{
	StrategyTreeNode *node = Create(mm);
	node->State.NPossibilities = 1;
	node->State.NCandidates = 1;
	node->State.Guess = possibility;
	node->AddChild(possibility.CompareTo(possibility), StrategyTreeNode::Done());
	return node;
}
