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
	class StrategyTreeState
	{
	public:
		Codeword Guess;
		int NPossibilities;
		int NCandidates;
	};

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
		StrategyTreeState m_state;
		// Codeword m_guess;
		StrategyTreeNode* m_children[256];
		int m_depth;
		int m_totaldepth;
		int m_hits;

	private:
		int FillDepthInfo(int depth, int depth_freq[], int max_depth) const;

	public:
		StrategyTreeState State;

	public:
		static StrategyTreeNode* Done() { return (StrategyTreeNode*)(-1); }
		static StrategyTreeNode* Single(const Codeword& possibility);

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
		void WriteToFile(FILE *fp, FileFormat format, int indent) const;

	public:
		//StrategyTreeNode(const Codeword &guess);
		StrategyTreeNode();
		~StrategyTreeNode();

		//Codeword GetGuess() const { return m_guess; }
		//void SetGuess(const Codeword &guess) { m_guess = guess; }

		void AddChild(Feedback fb, StrategyTreeNode *child);

		int GetDepthInfo(int depth_freq[], int max_depth) const;

		int GetDepth() const { return m_depth; }
		int GetTotalDepth() const { return m_totaldepth; }

		/// Outputs the strategy tree to a file.
		void WriteToFile(FILE *fp, FileFormat format, CodewordRules rules) const;
	};

	class StrategyTree : public StrategyTreeNode
	{
	public:
		

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