#pragma once

#include <stdio.h>
#include <ostream>

#include "MMConfig.h"
#include "Codeword.h"
#include "PoolMemoryManager.h"

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
	class StrategyTreeState
	{
	public:
		Codeword Guess;
		int NPossibilities;
		int NCandidates;
	};

	class StrategyTreeNode;
	typedef Utilities::PoolMemoryManager<StrategyTreeNode, 16> StrategyTreeMemoryManager;

	/// Represents a guessing strategy for the code breaker.
	///
	/// The strategy is internally represented by a tree. Each node in the 
	/// tree represents a <i>state</i> of the game. Some attributes 
	/// of a state include:
	/// - number of rounds played so far
	/// - number of remaining possibilities left
	/// - number of distinct guesses that the code breaker may make
	/// - next guess to make by the code breaker
	///
	/// These attributes are stored in StrategyTreeNode.
	///
	/// Each edge in the tree represents a feedback that corresponds to 
	/// a guess made by the code breaker.
	class StrategyTreeNode
	{
	private:
		// StrategyTreeMemoryManager *m_mm;
		// StrategyTreeNode* m_children[256];
		unsigned short m_depth; // depth of the tree (i.e. number of edges)
		unsigned short m_hits; // number of successes (i.e. number of leaves)
		unsigned int m_totaldepth; // total number of steps taken

		unsigned char m_childcount;
		unsigned char m_childindex[64];
		StrategyTreeNode *m_children[64];

		//StrategyTreeNode *m_firstchild;
		//StrategyTreeNode *m_nextsibling;

	protected:
		StrategyTreeNode() { }
		~StrategyTreeNode() { }

	private:
		int FillDepthInfo(int depth, int depth_freq[], int max_depth) const;

	public:
		StrategyTreeState State;

	public:
		static StrategyTreeNode* Done() { return (StrategyTreeNode*)(-1); }
		static StrategyTreeNode* Single(StrategyTreeMemoryManager *mm, const Codeword& possibility);

		/// Defines the file format of the strategy output.
		enum FileFormat
		{
			/// Default format
			DefaultFormat = 0,
			/// Plain text format
			PlainFormat = 0,
			/// XML format
			XmlFormat = 1,
		};
		
	protected:
		//StrategyTreeNode();
		//~StrategyTreeNode();

	protected:
		void WriteToFile(std::ostream &os, FileFormat format, int indent) const;

	public:
		static StrategyTreeNode* Create(StrategyTreeMemoryManager *mm);
		static void Destroy(StrategyTreeMemoryManager *mm, StrategyTreeNode *node);

		//StrategyTreeNode(const Codeword &guess);
		//StrategyTreeNode();
		//~StrategyTreeNode();

		//Codeword GetGuess() const { return m_guess; }
		//void SetGuess(const Codeword &guess) { m_guess = guess; }

		void AddChild(Feedback fb, StrategyTreeNode *child);

		int GetDepthInfo(int depth_freq[], int max_depth) const;

		int GetDepth() const { return m_depth; }
		int GetTotalDepth() const { return m_totaldepth; }

		/// Outputs the strategy tree to a file.
		void WriteToFile(std::ostream &os, FileFormat format, const CodewordRules &rules) const;
	};

	class StrategyTree : public StrategyTreeNode
	{
	public:
		~StrategyTree() { }

	private:
		//StrategyTreeNode *m_root;

		
	public:

		// Creates an empty strategy tree.
		/*StrategyTree(StrategyTreeNode *root)
		{
			m_root = root;
		}

		// Destructor.
		~StrategyTree()
		{
			if (m_root != NULL) {
				delete m_root;
				m_root = NULL;
			}
		}*/

	public:

		//StrategyTreeNode *GetRoot() { return m_root; }

		/// Outputs the strategy tree to a file.
		//void WriteToFile(FILE *fp, FileFormat format) const;

		//int GetDepthInfo(int freq[], int max_depth) const;
	};

}