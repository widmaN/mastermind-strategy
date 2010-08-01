#include <stdlib.h>

#include "StrategyTree.h"

using namespace Mastermind;

StrategyTreeNode::StrategyTreeNode()
{
	// m_guess = guess;
	memset(m_children, 0, sizeof(m_children));
	m_depth = 0;
	m_totaldepth = 0;
	m_hits = 0;
}

StrategyTreeNode::~StrategyTreeNode()
{
	for (int i = 0; i < sizeof(m_children)/sizeof(m_children[0]); i++) {
		if (m_children[i] != NULL) {
			if (m_children[i] != Done())
				delete m_children[i];
			m_children[i] = NULL;
		}
	}
	m_depth = 0;
	m_totaldepth = 0;
	m_hits = 0;
	// m_guess = Codeword::Empty();
}

void StrategyTreeNode::AddChild(Feedback fb, StrategyTreeNode *child)
{
	int i = fb.GetValue();
	assert(m_children[i] == NULL);
	m_children[i] = child;

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
	for (int i = 0; i < sizeof(m_children)/sizeof(m_children[0]); i++) {
		const StrategyTreeNode *child = m_children[i];
		if (child == Done()) { // this is the leaf
			if (depth > max_depth) {
				freq[max_depth]++;
			} else {
				freq[depth]++;
			}
			total += depth;
		} else if (child != NULL) {
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

	for (int i = 0; i < sizeof(m_children)/sizeof(m_children[0]); i++) {
		const StrategyTreeNode *child = m_children[i];
		if (child != NULL) {
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

StrategyTreeNode* StrategyTreeNode::Single(const Codeword& possibility)
{
	StrategyTreeNode *node = new StrategyTreeNode();
	node->State.NPossibilities = 1;
	node->State.NCandidates = 1;
	node->State.Guess = possibility;
	node->AddChild(possibility.CompareTo(possibility), StrategyTreeNode::Done());
	return node;
}
