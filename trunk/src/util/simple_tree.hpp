#ifndef UTILITIES_SIMPLE_TREE_HPP
#define UTILITIES_SIMPLE_TREE_HPP

#include <vector>
#include <cassert>
#include <iterator>
#include <type_traits>

namespace util {

#if 0
/// Defines the order for traversing a tree.
enum TraversalOrder
{
	NativeOrder = 0,
	PreOrder = 1,
	InOrder = 2,
	PostOrder = 3,
	BreadthFirst = 4,
};
#endif

/**
 * Represents a simple tree which only allows adding children to the last
 * node of each depth.
 */
template <class T, class TDepth = size_t>
class simple_tree
{
protected:

	struct node_t
	{
		T data;
		TDepth depth;
		node_t(const T& _data, TDepth _depth) : data(_data), depth(_depth) { }
	};

	std::vector<node_t> _nodes;

	typedef simple_tree<T,TDepth> self_type;

public:
	typedef TDepth depth_type;

	/// Represents an iterator for the nodes in a simple tree.
	template <bool IsConst>
	class node_iterator
	{
		typedef typename std::conditional<IsConst,
			const self_type, self_type>::type Tree;

		// typedef simple_tree_iterator<T,TDepth,IsConst> self_type;

		Tree * _tree;
		size_t _index;

	public:

		/// Iterator category.
		typedef std::forward_iterator_tag iterator_category;

		/// Type of the data associated with each node.
		typedef T value_type;

		/// Type of iterator difference.
		typedef std::ptrdiff_t difference_type;

		/// Type of reference to data.
		typedef typename std::conditional<IsConst,
			const value_type &, value_type &>::type reference;

		/// Type of const reference to data.
		typedef const value_type & const_reference;

		/// Type of pointer to data.
		typedef typename std::conditional<IsConst,
			const value_type *, value_type *>::type pointer;

		/// Type of const pointer to data.
		typedef const value_type * const_pointer;

		/// Constructs an unspecified iterator.
		node_iterator() : _tree(0), _index(0) { }

		/// Copy-constructs an iterator.
		node_iterator(const node_iterator<false> &other)
			: _tree((Tree*)other.tree()), _index(other.index()) { }

		/// Constructs an iterator that points to a specific node in a tree.
		node_iterator(Tree *tree, size_t index)
			: _tree(tree), _index(index) { }

		/// Returns a pointer to the underlying strategy tree.
		Tree * tree() { return _tree; }

		/// Returns a const pointer to the underlying strategy tree.
		const Tree * tree() const { return _tree; }

		/// Returns the internal index of this node (root = 0).
		size_t index() const { return _index; }

		/// Returns the depth of this node (root = 0).
		TDepth depth() const { return _tree->_nodes[_index].depth; }

		/// Advances to the next sibling. If there is no next sibling, the
		/// iterator stops at a position where the next sibling would have
		/// inserted.
		node_iterator& operator ++ ()
		{
			TDepth d = depth();
			size_t i = _index + 1;
			while (i < _tree->_nodes.size() && _tree->_nodes[i].depth > d)
				++i;
			_index = i;
			return *this;
		}

		/// Returns a reference to the node data.
		reference operator * () { return _tree->_nodes[_index].data; }

		/// Returns a const reference to the node data.
		const_reference operator * () const { return _tree->_nodes[_index].data; }

		/// Returns a pointer to the node data.
		pointer operator -> () { return &_tree->_nodes[_index].data; }

		/// Returns a const pointer to the node data.
		const_pointer operator -> () const { return &_tree->_nodes[_index].data; }

		/// Tests whether the iterator is empty.
		bool operator ! () const { return _tree == 0; }

		/// Tests whether the iterator is not empty.
		operator void* () const { return (_tree == 0)? 0 : (void*)this; }

		/// Tests whether two iterators are equal.
		template <bool IsConst2>
		bool operator == (const node_iterator<IsConst2> &it)
		{
			return (index() == it.index()) && (tree() == it.tree());
		}

		/// Tests whether two iterators are not equal.
		template <bool IsConst2>
		bool operator != (const node_iterator<IsConst2> &it)
		{
			return ! operator == (it);
		}

		/// Returns an iterator to the first child of this node.
		/// If this node contains no child, returns <code>child_end()</code>.
		node_iterator child_begin()
		{
			return node_iterator(_tree, _index + 1);
		}

		/// Returns an iterator to one past the last child of this node.
		node_iterator child_end()
		{
			node_iterator it(*this);
			return ++it;
		}
	};

public:

	/// Type of the data associated with the nodes in the tree.
	typedef T value_type;

	/// Type of the depth of a node.
	typedef TDepth depth_type;

	/// Type of a node iterator.
	typedef node_iterator<false> iterator;

	/// Type of a const node iterator.
	typedef node_iterator<true> const_iterator;

public:

	/// Creates a tree or branch with the given root data.
	simple_tree(const T& root_data, TDepth root_depth = 0)
	{
		_nodes.push_back(node_t(root_data, root_depth));
	}

	/// Returns the number of nodes in the tree.
	size_t size() const { return _nodes.size(); }

	/// Returns an iterator to the root of the tree.
	iterator root() { return iterator(this, 0); }

	/// Returns a const iterator to the root of the tree.
	const_iterator root() const { return const_iterator(this, 0); }

	/// Appends a node to the end of the tree.
	///
	/// @param data The node data.
	/// @param depth Specifies the logical position of the node in the tree.
	///      This depth must be greater than the root depth of the tree
	///      and smaller than or equal to one plus the depth of the last
	///      node in the tree.
	/// @returns Index of the added node.
	size_t append(const T& data, TDepth depth)
	{
		assert(depth > _nodes[0].depth);
		assert(depth <= _nodes.back().depth + 1);

		_nodes.push_back(node_t(data, depth));
		return _nodes.size() - 1;
	}

	/// Appends a branch to the end of the tree.
	///
	/// @param subtree The branch to append. The depth of the root node of
	///      the branch must be greater than the root depth of the tree
	///      and smaller than or equal to one plus the depth of the last node
	///      in the tree.
	void append(const simple_tree &subtree)
	{
		assert(subtree._nodes[0].depth > _nodes[0].depth);
		assert(subtree._nodes[0].depth <= _nodes.back().depth + 1);
		_nodes.insert(_nodes.end(), subtree._nodes.begin(), subtree._nodes.end());
	}

#if 0
	/// Adds a child to the given node. This invalidates all iterators
	/// that are not on the parent path of this child.
	node_iterator add_child(const T& data, node_iterator where)
	{

	}
#endif

	/// Visits all nodes under a given root in natural order.
	template <bool IsConst, class Func>
	void traverse(node_iterator<IsConst> root, Func f) const
	{
		TDepth root_depth = root.depth();
		size_t root_index = root.index();
		size_t count = size();
		if (root_index >= count)
			return;

		// Visit the root node.
		f(0, root);

		// Traverse children in pre-order.
		for (size_t i = root_index + 1; i < count; ++i)
		{
			node_iterator<IsConst> it(this, i);
			if (it.depth() <= root_depth)
				break;
			f(it.depth() - root_depth, it);
		}
	}

};

} // namespace util

#endif // UTILITIES_SIMPLE_TREE_HPP
