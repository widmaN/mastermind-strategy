#include <stdlib.h>

#include "StrategyTree.h"

using namespace Mastermind;

StrategyTreeNode::StrategyTreeNode(const Codeword &guess)
{
	m_guess = guess;
	memset(m_children, 0, sizeof(m_children));
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
	m_guess = Codeword::Empty();
}

void StrategyTreeNode::AddChild(Feedback fb, StrategyTreeNode *child)
{
	int i = fb.GetValue();
	if (m_children[i] != NULL && m_children[i] != Done()) {
		delete m_children[i];
	}
	m_children[i] = child;
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
				fprintf(fp, "%*s<case guess=\"%s\" feedback=\"%s\">\n",
						indent, "",
						m_guess.ToString().c_str(),
						Feedback(i).ToString().c_str());
			} else {
				fprintf(fp, "%*s%s:%s\n", 
						indent, "",
						m_guess.ToString().c_str(),
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
	FileFormat format) const
{
	if (format == XmlFormat) {
		fprintf(fp, "<game>\n");
	}
	WriteToFile(fp, format, 0);
	if (format == XmlFormat) {
		fprintf(fp, "</game>\n");
	}
}