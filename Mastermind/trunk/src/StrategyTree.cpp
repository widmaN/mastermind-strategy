#include <stdlib.h>

#include "StrategyTree.h"

using namespace Mastermind;

StrategyTree::StrategyTree()
{
	m_root = new node_t;
	m_root->parent = NULL;
	m_root->prev_sibling = NULL;
	m_root->next_sibling = NULL;
	m_root->first_child = NULL;
	m_current = m_root;
}

StrategyTree::~StrategyTree()
{
	Clear();
	delete m_root;
}

void StrategyTree::DeleteNode(node_t *node) 
{
	assert(node != NULL);
	while (node->first_child != NULL) {
		 DeleteNode(node->first_child);
	}
	if (node->prev_sibling != NULL) {
		node->prev_sibling->next_sibling = node->next_sibling;
	}
	if (node->next_sibling != NULL) {
		node->next_sibling->prev_sibling = node->prev_sibling;
	}
	if (node->parent->first_child == node) {
		node->parent->first_child = node->next_sibling;
	}
	delete node;
}

void StrategyTree::Clear()
{
	while (m_root->first_child != NULL) {
		DeleteNode(m_root->first_child);
	}
	m_current = m_root;
}

void StrategyTree::Push(Codeword &guess, Feedback feedback)
{
	// Create new node
	node_t *pn = new node_t;
	pn->feedback = feedback;
	pn->guess = guess;
	pn->first_child = NULL;
	pn->next_sibling = NULL;
	pn->prev_sibling = NULL;
	pn->parent = m_current;

	// Insert the node
	if (m_current->first_child == NULL) {
		m_current->first_child = pn;
	} else {
		node_t *p = m_current->first_child;
		for (; p->next_sibling; p = p->next_sibling);
		pn->prev_sibling = p;
		p->next_sibling = pn;
	}

	// Set current
	m_current = pn;
}

void StrategyTree::Pop()
{
	assert(m_current->parent != NULL);
	m_current = m_current->parent;
}

void StrategyTree::WriteNodeToFile(FILE *fp, const node_t *node, int indent) const
{
	assert(fp != NULL);
	assert(node != NULL);
	assert(indent >= 0 && indent < 200);

	// Output this node
	if (!node->guess.IsEmpty()) {
		char sindent[250];
		memset(sindent, ' ', sizeof(sindent));
		sindent[indent] = '\0';
		fprintf(fp, "%s%s:%s\n", sindent, 
			node->guess.ToString().c_str(),
			node->feedback.ToString().c_str());
	} else {
		indent = indent - 2;
	}

	// Output children
	for (node_t *p = node->first_child; p != NULL; p = p->next_sibling) {
		WriteNodeToFile(fp, p, indent + 2);
	}
}

void StrategyTree::WriteToFile(FILE *fp) const
{
	WriteNodeToFile(fp, m_root, 0);
}

int StrategyTree::FillDepthInfo(const node_t *node, int depth, int freq[], int max_depth) const
{
	if (node->first_child == 0) { // this is the leaf
		if (depth > max_depth) {
			freq[max_depth]++;
		} else {
			freq[depth]++;
		}
		return depth;
	} else {
		int total = 0;
		for (const node_t *p = node->first_child; p != NULL; p = p->next_sibling) {
			total += FillDepthInfo(p, depth + 1, freq, max_depth);
		}
		return total;
	}
}

int StrategyTree::GetDepthInfo(int freq[], int max_depth) const
{
	memset(freq, 0, sizeof(int)*(max_depth+1));
	return FillDepthInfo(m_root, 0, freq, max_depth);
}

//void StrategyTree::Remove(Feedback fb, Codeword &guess)
//{
//}
