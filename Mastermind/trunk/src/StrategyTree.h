#pragma once

#include <stdio.h>

#include "MMConfig.h"
#include "Codeword.h"

// StrategyTree data structure
//
//   state0[root1, root2, root3, ...]
//     |0123->0a0b
//   [state1] 
//            
//
//
namespace Mastermind
{
	class StrategyTree
	{
	private:
		struct node_t {
			Codeword guess;
			Feedback feedback;
			node_t *parent;
			node_t *prev_sibling;
			node_t *next_sibling;
			node_t *first_child;
		};
		node_t *m_root;
		node_t *m_current;

	private:
		void DeleteNode(node_t *node);
		void WriteNodeToFile(FILE *fp, const node_t *node, int indent) const;
		int FillDepthInfo(const node_t *node, int depth, int freq[], int max_depth) const;

	public:
		StrategyTree();
		~StrategyTree();

	public:
		void Clear();
		void Push(Codeword &guess, Feedback feedback);
		void Pop();
		// void Remove(Feedback fb, Codeword &guess);

		void WriteToFile(FILE *fp) const;

		int GetDepthInfo(int freq[], int max_depth) const;
	};
}