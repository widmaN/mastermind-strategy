#ifndef MASTERMIND_STRATEGY_TREE_HPP
#define MASTERMIND_STRATEGY_TREE_HPP

#include <iostream>
#include <vector>
#include <string>

#include "Rules.hpp"
#include "Codeword.hpp"
#include "Feedback.hpp"

namespace Mastermind {

/**
 * Represents a guessing strategy of the code breaker.
 *
 * The strategy is internally stored as a tree. Each node in the tree
 * represents a <i>state</i> (or situation) of the game. Each state 
 * is identified by a sequence of constraints consisting of the guesses
 * made and responses received so far. The constraints are read from 
 * the root node to the given node.
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

	class Node
	{
		// Basic fields that identify the state.
		unsigned long _guess;          // stored in compact format
		unsigned char _response;       
		unsigned char _depth;          // 0=root, 1=first guess, etc.

#if 0
		// The guess to make according to the strategy.
		unsigned long  suggestion;     // stored in compact format
#endif

#if 0
		// Debugging fields.
		unsigned short npossibilities; // number of remaining possibilities
		unsigned short ncandidates;    // number of canonical guesses
#endif

		// Auxiliary field for fast traverse.
		//unsigned int  _next_sibling; // offset to next sibling, 0=none
		//unsigned int    _child_count;  // number of child nodes

		/// Constructs a node with the basic fields.
#if 0
		Node(unsigned char _depth, unsigned long _guess, unsigned char _feedback)
			: guess(_guess), feedback(_feedback), depth(_depth),
			suggestion(0), npossibilities(0), ncandidates(0), next_sibling(0)
		{ }
#endif
		friend class StrategyTree;

	public:

		/// Constructs an empty node suitable to be used as a root node.
		Node() : _depth(0) { }

		/// Constructs a node with the given depth, guess and response.
		Node(int depth, const Codeword &guess, const Feedback &response)
			: _guess(Codeword::pack(guess)), _response(response.value()),
			_depth(depth) // , _child_count(0) 
		{ 
		}

		int depth() const { return _depth; }

		Codeword guess() const { return Codeword::unpack(_guess); }

		Feedback response() const { return _response; }
	};

private:
	Rules _rules;
	std::vector<Node> _nodes;

	// Maintain an index to the first element of each depth in the
	// latest branch.
	//std::vector<size_t> _latest;

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

	const Rules& rules() const { return _rules; }

	const std::vector<Node>& nodes() const { return _nodes; }

	/// Returns the depth of the last node in the tree.
	int currentDepth() const 
	{
		return _nodes.empty()? -1 : _nodes.back()._depth;
	}

#if 0
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
#endif

	/// Returns the number of nodes in the tree.
	size_t size() const { return _nodes.size(); }

	/// Appends a node to the end of the tree.
	/// The <code>depth</code> argument specifies the logical position
	/// of the node.
	/// Returns the index of the added node.
	size_t append(const Node &node) // rename as push?
	{
		assert(node._depth >= 0 && node._depth <= currentDepth() + 1);

		// Append the node to the tree.
		size_t index = _nodes.size();
		_nodes.push_back(node);

		// Update the indices to the latest branch.
#if 0
		size_t depth = node._depth;
		if (depth >= _latest.size())
		{
			_latest.push_back(index);
		}
		else
		{
			_nodes[_latest[depth]].next_sibling = index;
			_latest.erase(_latest.begin() + depth + 1, _latest.end());
		}
#endif
		return index;
	}

#if 0
	/// Remove all nodes starting from _pos_ and till the end of the tree.
	void truncate(size_t pos)
	{
		_nodes.erase(_nodes.begin() + pos, _nodes.end());
	}
#endif

	/// Erase all nodes in the range [begin,end).
	void erase(size_t first, size_t last)
	{
		_nodes.erase(_nodes.begin() + first, _nodes.begin() + last);
	}

	unsigned int getDepthInfo(unsigned int depth_freq[], unsigned int count) const;

};

class StrategyTreeInfo
{
	// depth_freq[i] = number of secrets revealed using (i) guesses
	std::vector<unsigned int> _depth_freq;
	unsigned int _total_depth;
	unsigned int _total_secrets;
	std::string _name;
	double _time;

public:
	
	StrategyTreeInfo(
		const std::string &name, 
		const StrategyTree &tree, 
		double time)
		: _total_depth(0), _total_secrets(0), _name(name), _time(time)
	{
		Feedback perfect = Feedback::perfectValue(tree.rules());
		// unsigned int total = 0;
		// std::fill(depth_freq + 0, depth_freq + count, 0);
		for (size_t i = 0; i < tree.size(); ++i)
		{
			if (tree.nodes()[i].response() == perfect)
			{
				unsigned int d = tree.nodes()[i].depth();
				if (d >= _depth_freq.size())
				{
					_depth_freq.resize(d+1);
				}
				++_depth_freq[d];
				++_total_secrets;
				_total_depth += d;
			}
		}
	}

	std::string name() const
	{
		return _name;
	}

	double time() const { return _time; }

	int max_depth() const
	{
		return _depth_freq.size() - 1;
	}

	unsigned int count_depth(int depth) const
	{
		return (depth >= 0 && depth <= max_depth())?
			_depth_freq[depth] : 0;
	}

	unsigned int total_depth() const { return _total_depth; }

	unsigned int total_secrets() const { return _total_secrets; }

	double average_depth() const 
	{
		return (double)total_depth() / total_secrets();
	}
};

std::ostream& operator << (std::ostream &os, const StrategyTreeInfo &info);

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
