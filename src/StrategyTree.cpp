#include <iostream>
#include <iomanip>
#include <cassert>

//#include <stdlib.h>
#include <algorithm>
#include <utility>
#include <map>
#include "util/io_format.hpp"

#include "StrategyTree.hpp"
#include "Algorithm.hpp"
#include "Engine.hpp"

using namespace Mastermind;

///////////////////////////////////////////////////////////////////////////
// StrategyTreeNode implementation
//

namespace Mastermind {

std::ostream& operator << (std::ostream &os, const StrategyTreeInfo &info)
{
	const int max_display = 9;

	// Display header.
	if (os.iword(util::header_index()))
	{
		//os << "Stategy Statistics" << std::endl;
		//os << "--------------------" << std::endl;
		os << "Strategy  Total   Avg 1     2     3     4     5     6     7     8    >8   Time" << std::endl;
		os << util::noheader;
	}

	// Display label.
	os  << std::setw(8) << info.name()
		<< std::setw(7) << info.total_depth() << " "
		<< std::setw(5) << std::setprecision(3)
		<< std::fixed << info.average_depth();

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
		os << std::setw(d == 1? 2 : 6);
		if (count > 0)
			os << count;
		else
			os << '-';
	}

	// Display time
	os  << std::fixed << std::setw(7) << std::setprecision(1)
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

#if 0
//typedef partition<Feedback,256,CodewordRange> CodewordPartition;
struct CodewordCell : public CodewordRange
{
	Feedback feedback;
	CodewordCell(Feedback fb, CodewordIterator first, CodewordIterator last)
		: CodewordRange(first, last), feedback(fb) { }
};

typedef std::vector<CodewordCell> CodewordPartition;

static CodewordPartition
partition(Engine &e, CodewordRange secrets, const Codeword &guess)
{
	FeedbackFrequencyTable freq = e.partition(secrets, guess);
	CodewordRange range(secrets.begin(), secrets.begin());
	CodewordPartition cells;
	for (size_t j = 0; j < freq.size(); ++j)
	{
		if (freq[j] > 0)
		{
			Feedback fb((unsigned char)j);
			range = CodewordRange(range.end(), range.end() + freq[j]);
			cells.push_back(CodewordCell(fb, range.begin(), range.end()));
		}
	}
	return cells;
}
#endif

/*
Example:
1296 (1123: 2 (2311), 44A, 222B, 276C, 81D; 4 (1312), 84E, 230F, 182G; 5 (1321: 1, 0, 0, 0,
0; 2 (1132), 0, 0, 0; 1, 0, 0; 0; 1), 40H, 105I; 20J; 1)

A = (2345: 0, 6 (3412), 10 (3612: 1, 0, 0, 0, 0; 1, 1, 2 (4211), 0; 1, 0, 2 (3411); 1; 1),
    2 (3611), 0; 0, 6 (3215), 8 (2461: 0, 1, 1,0, 0; 0, 0, 1, 1; 0, 1, 2 (2231); 1; 0), 4 (2611); 0,
    2 (2314), 4 (2316); 2 (2315); 0)
*/

#define PARSING_ERROR(msg) err << is.tellg() << ": " << msg << std::endl
#define PARSING_WARNING(msg) err << is.tellg() << ": warning: " << msg << std::endl

/// Reads a situation from a compact format strategy.
static bool ReadSituation_TextFormat(
	std::istream &is,      // input stream
	StrategyTree &tree,    // the tree to build
	std::ostream &err,     // a stream to display error
	bool read_count,       // whether to read number of remaining possibilities
	Engine &e,             // algorithm engine
	CodewordRange secrets, // list of remaining possibilities
	std::map<char,CodewordRange> &placeholders
	)
{
	// Read number of remaining possibilities.
	if (read_count)
	{
		size_t n;
		if (!(is >> n))
		{
			PARSING_ERROR("expecting number of possibilities");
			return false;
		}
		if (n != secrets.size())
		{
			PARSING_WARNING("mismatch in number of possibilities: got "
				<< n << ", expecting " << secrets.size());
			// return false;
		}
	}

	// Read strategy for this situation.
	// Three formats are allowed following the number of possibilities:
	// - empty: this situation is trivial and not elaborated;
	// - [A-Z]: this situation is expanded later by the given identifier;
	// - (...): this situation is elaborated in-place.
	char c;
	if (!(is >> c))
		return true;

	if (c >= 'A' && c <= 'Z') // [A-Z] identifies a sub-situation
	{
		placeholders.insert(std::make_pair(c, secrets));
		return true;
	}
	else if (c == '(') // in-place strategy display
	{
		// Read the guess.
		Codeword guess;
		if (!(is >> guess))
		{
			is.clear();
			PARSING_ERROR("expecting guess");
			return false;
		}
		if (!guess.valid(e.rules()))
		{
			PARSING_ERROR("invalid guess: " << guess);
			return false;
		}

		// The next character determines the following format:
		// ) means this guess partitions the remaining possibilities
		//   into singleton cells, and this guess is one of the
		//   remaining possibilities;
		// * means this guess partitions the remaining possibilities
		//   into singleton cells, but this guess is not one of the
		//   remaining possibilities;
		// x means this guess partitions the remaining possibilities
		//   into cells with no more than two possibilities;
		// : means none of the above trivial scenarios are true, so
		//   more details follow.
		// Note that * and x are treated as indications only and
		// ignored.
		if (!(is >> c))
		{
			PARSING_ERROR("expecting one of ')', '*', 'x', ':'");
			return false;
		}
		if (c == '*' || c == 'x') // ignore indicative flags
		{
			if (!(is >> c))
			{
				PARSING_ERROR("expecting ')' or ':'");
				return false;
			}
		}

		if (c == ')')
		{
			// Trivial situation, nothing elaborated.
			return true;
		}
		else if (c == ':')
		{
			// List of situations in the following order:
			// a04,a03,a02,a01,a00;a13,a12,a11,a10;a22,a21,a20;a30;a40

			// Let's partition the remaining secrets first.
			FeedbackFrequencyTable freq = e.partition(secrets, guess);
			CodewordIterator parts[257];
			parts[0] = secrets.begin();
			for (size_t j = 0; j < freq.size(); ++j)
			{
				parts[j+1] = parts[j] + freq[j];
			}

			int p = e.rules().pegs();
			for (int nA = 0; nA <= p; ++nA)
			{
				for (int nB = (p - nA); nB >= 0; --nB)
				{
					// Read number of remaining possibilities.
					size_t n;
					if (!(is >> n))
					{
						is.clear();
						PARSING_ERROR("expecting number of possibilities");
						return false;
					}

					// Note that (p-1,1) is an impossible situation, in which
					// the number of remaining possibilities is always zero.
					// Thus this situation is omitted by Irving (1979), etc.
					// However, in Knuth(1976), this redundant situation a31=0
					// is explicitly shown; so we tweak a bit to be compatible
					// with his format.
					if (nA == p - 1 && nB == 1)
					{
						// Eat the next token if it is "0,".
						if (n == 0)
						{
							char c1;
							if (is >> c1)
							{
								if (c1 == ',')
									continue;
								else
									is.unget();
							}
						}
						--nB;
					}

					// Check sub-situation size.
					size_t j = Feedback(nA, nB).value();
					CodewordRange cell(parts[j], parts[j+1]);
					if (n != cell.size())
					{
						PARSING_WARNING("mismatch in number of possibilities: "
							<< "got " << n << ", expecting " << cell.size());
						// return false;
					}

					// Read sub-state.
					if (!ReadSituation_TextFormat(is, tree, err, false, e,
						cell, placeholders))
						return false;

					// Read separator.
					char sp_want = (nB > 0)? ',' : (nA < p)? ';' : ')';
					char sp_read;
					if (!(is >> sp_read) || (sp_read != sp_want))
					{
						PARSING_ERROR("expecting '" << sp_want << "', got '"
							<< sp_read << "'");
						return false;
					}
				}
			}
			return true;
		}
		else // bad syntax
		{
			PARSING_ERROR("expecting ')' or ':', got '" << c << "'");
			return false;
		}
	}
	else // trivial, non-elaborated situation
	{
		is.unget();
		return true;
	}
}

/*
Example:
1296 (1123: 2 (2311), 44A, 222B, 276C, 81D; 4 (1312), 84E, 230F, 182G; 5 (1321: 1, 0, 0, 0,
0; 2 (1132), 0, 0, 0; 1, 0, 0; 0; 1), 40H, 105I; 20J; 1)

A = (2345: 0, 6 (3412), 10 (3612: 1, 0, 0, 0, 0; 1, 1, 2 (4211), 0; 1, 0, 2 (3411); 1; 1),
    2 (3611), 0; 0, 6 (3215), 8 (2461: 0, 1, 1,0, 0; 0, 0, 1, 1; 0, 1, 2 (2231); 1; 0), 4 (2611); 0,
    2 (2314), 4 (2316); 2 (2315); 0)
*/
static bool ReadStrategy_TextFormat(std::istream &is, StrategyTree &tree, std::ostream &err)
{
	// Create a list of all the secrets.
	Engine e(tree.rules());
	CodewordList secrets(e.generateCodewords());
	CodewordRange all(secrets);

	// Create a map of situation placeholders (symbol table).
	std::map<char,CodewordRange> placeholders;
	std::map<char,bool> defined;

	// Read root situation.
	if (!ReadSituation_TextFormat(is, tree, err, true, e, all, placeholders))
		return false;

	// Read each sub-situation identified by [A-Z]. For example,
	// A = (2345: 0, ....
	char id;
	while (is >> id)
	{
		char c;
		if (!(is >> c) || (c != '='))
		{
			PARSING_ERROR("expecting '='");
			return false;
		}
		if (placeholders.find(id) == placeholders.end())
		{
			PARSING_ERROR("undeclared situation identifier '" << id << "'");
			return false;
		}
		if (defined[id])
		{
			PARSING_ERROR("situation " << id << " is already defined");
			return false;
		}
		CodewordRange cell = placeholders.at(id);
		if (!ReadSituation_TextFormat(is, tree, err, false, e, cell, placeholders))
			return false;
		defined[id] = true;
	}

	// Check for declared but undefined sub-situations.
	if (defined.size() < placeholders.size())
	{
		is.clear();
		PARSING_ERROR("some situations are not defined");
		return false;
	}

	return true;
}

std::istream& operator >> (std::istream &is, StrategyTree &tree)
{
	if (!ReadStrategy_TextFormat(is, tree, std::cerr))
	{
		// Set fail bit.
	}
	return is;
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
