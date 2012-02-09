#ifndef MASTERMIND_STRATEGY_TREE_HPP
#define MASTERMIND_STRATEGY_TREE_HPP

#include <iostream>
#include <vector>

#include "Rules.hpp"
#include "Codeword.hpp"
#include "Feedback.hpp"

namespace Mastermind {

/**
 * Represents a guessing strategy of the code breaker.
 *
 * The strategy is internally stored as a tree. Each node in the tree
 * represents a <i>state</i> of the game. The state is identified by
 * the series of constraints consisting of the guesses made and 
 * feedbacks received so far. The constraints are read from the root
 * node to the given node.
 *
 * For example, 
 * [Root] <- this state contains all the codewords
 *   [1234:0A0B] <- this state contains the parent state filtered by
 *                  1234:0A0B
 *     ...
 *     [3456:1A0B] <- this state contains the parent state filtered 
 *                    by 3456:1A0B
 *     ...
 *
 * When we build a strategy tree, we use a recursive algorithm which
 * always appends a node to the tree. This is critical to enable
 * a simple storage pattern. That is, we can store the tree linearly
 * as a vector, and label the depth of each state. Such storage
 * is simple to understand and implement. It can even be output
 * directly to a binary file and memory-mapped if the tree is big.
 *
 * Associated with each state/node, a few attributes are attached,
 * such as
 *   - the number of remaining possibilities;
 *   - the number of candidate guesses; and, most importantly,
 *   - the next guess to make.
 * These attributes are stored in <code>StrategyTree::Node</code>.
 */
class StrategyTree
{
public:
	struct Node
	{
		// Basic fields that identify the state.
		unsigned long  guess;          // stored in compact format
		unsigned char  feedback;       
		unsigned char  depth;          // 0=root, 1=first guess, etc.

		// The guess to make according to the strategy.
		unsigned long  suggestion;     // stored in compact format

#ifndef NTEST
		// Debugging fields.
		unsigned short npossibilities; // number of remaining possibilities
		unsigned short ncandidates;    // number of candidate guesses
#endif

		// Auxiliary field for fast traverse.
		unsigned int   next_sibling;   // index to next sibling

		/// Constructs a node with the basic fields.
		Node(unsigned char _depth, unsigned long _guess, unsigned char _feedback)
			: guess(_guess), feedback(_feedback), depth(_depth),
			suggestion(0), npossibilities(0), ncandidates(0), next_sibling(0)
		{ }
	};

private:
	Rules _rules;
	std::vector<Node> _nodes;

	// Maintain an index to the first element of each depth in the
	// latest branch.
	std::vector<size_t> _latest;

public:

	StrategyTree(const Rules &rules) : _rules(rules)
	{
#if 0
		// Add a root node.
		Node root;
		root.depth = 0;
		root.nchildren = 0;
		root.guess = 0;
		root.npossibilities = rules.size();
		root.ncandidates = 0;
		_nodes.push_back(root);
#endif
	}

	Rules rules() const { return _rules; }

	const std::vector<Node>& nodes() const { return _nodes; }

	int currentDepth() const 
	{
		return _nodes.empty()? -1 : _nodes.back().depth;
	}

	/// Append a node to the end of the tree.
	/// The <code>depth</code> member of the node determines where
	/// the node is inserted logically.
	void append(const Node &node) // rename as push?
	{
		assert(node.depth >= 0 && node.depth <= currentDepth() + 1);
		_nodes.push_back(node);

		// Update the indices to the latest branch.
		unsigned int index = (unsigned int)_nodes.size() - 1;
		if (node.depth >= _latest.size())
		{
			_latest.push_back(index);
		}
		else
		{
			size_t d = node.depth;
			_nodes[_latest[d]].next_sibling = index;
			_latest.resize(d + 1);
		}
	}

	unsigned int getDepthInfo(unsigned int depth_freq[], unsigned int count) const;

	//int GetDepth() const { return m_depth; }
	//int GetTotalDepth() const { return m_totaldepth; }

	// StrategyTreeMemoryManager *m_mm;
	// StrategyTreeNode* m_children[256];
	//unsigned short m_depth; // depth of the tree (i.e. number of edges)
	//unsigned short m_hits; // number of successes (i.e. number of leaves)
	//unsigned int m_totaldepth; // total number of steps taken

	//unsigned char m_childcount;
	//unsigned char m_childindex[64];
	//StrategyTreeNode *m_children[64];

	//StrategyTreeNode *m_firstchild;
	//StrategyTreeNode *m_nextsibling;
};

/// Defines the file format to serialize a stratey tree.
enum FileFormat
{
	DefaultFormat = 0,
	TextFormat = 0,
	XmlFormat = 1,
	BinaryFormat = 2,
};

/// Outputs the strategy tree to a file.
template <FileFormat Format>
void WriteToFile(std::ostream &os, const StrategyTree &tree);

template <> void WriteToFile<TextFormat>(std::ostream &, const StrategyTree &);
template <> void WriteToFile<XmlFormat>(std::ostream &, const StrategyTree &);
template <> void WriteToFile<BinaryFormat>(std::ostream &, const StrategyTree &);

} // namespace Mastermind

#endif // MASTERMIND_STRATEGY_TREE_HPP
