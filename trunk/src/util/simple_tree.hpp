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

	//typedef TDepth TSize;

	struct node_t
	{
		T data;     // user data associated with this node
		//TSize size; // 1 + number of children of this node
		TDepth depth;
		//node_t(const T& _data) : data(_data), size(1) { }
		node_t(const T& _data, TDepth _depth) : data(_data), depth(_depth) { }
	};

	std::vector<node_t> _nodes;


public:

	typedef simple_tree<T,TDepth> self_type;

	/// Represents an iterator for the nodes in a simple tree.
	template <bool IsConst>
	class node_iterator
	{
		typedef typename std::conditional<IsConst,
			const self_type, self_type>::type Tree;

		Tree * _tree;
		size_t _index;

		/// Returns the depth of this node (root = 0).
		TDepth depth() const { return _tree->_nodes[_index].depth; }

		friend class simple_tree<T,TDepth>;

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

		/// Type of pointer to data.
		typedef typename std::conditional<IsConst,
			const value_type *, value_type *>::type pointer;

		/// Constructs an unspecified iterator.
		node_iterator() : _tree(0), _index(0) { }

		/// Copy-constructs an iterator.
		node_iterator(const node_iterator<false> &other)
			: _tree(other.tree()), _index(other.index()) { }

		/// Constructs an iterator that points to a specific node in a tree.
		node_iterator(Tree *tree, size_t index)
			: _tree(tree), _index(index) { }

		/// Returns a pointer to the underlying strategy tree.
		Tree * tree() const { return _tree; }

		/// Returns the internal index of this node (root = 0).
		size_t index() const { return _index; }

		/// Advances the iterator to the next sibling of this node. If there
		/// is no next sibling, the advanced iterator points to a position 
		/// where the next sibling would have been nserted.
		node_iterator& operator ++ ()
		{
			TDepth d = _tree->_nodes[_index].depth;
			size_t i = _index + 1;
			while (i < _tree->_nodes.size() && _tree->_nodes[i].depth > d)
				++i;
			_index = i;
			return *this;
		}

		/// Returns a reference to the node data.
		reference operator * () const { return _tree->_nodes[_index].data; }

		/// Returns a pointer to the node data.
		pointer operator -> () const { return &_tree->_nodes[_index].data; }

		/// Tests whether the iterator is empty.
		bool operator ! () const { return _tree == 0; }

		/// Tests whether the iterator is not empty.
		operator void* () const { return (_tree == 0)? 0 : (void*)this; }

		/// Tests whether two iterators are equal.
		template <bool IsConst2>
		bool operator == (const node_iterator<IsConst2> &it) const
		{
			return (index() == it.index()) && (tree() == it.tree());
		}

		/// Tests whether two iterators are not equal.
		template <bool IsConst2>
		bool operator != (const node_iterator<IsConst2> &it) const
		{
			return ! operator == (it);
		}

		/// Returns an iterator to the first child of this node.
		/// If this node contains no child, returns <code>child_end()</code>.
		node_iterator child_begin() const
		{
			return node_iterator(_tree, _index + 1);
		}

		/// Returns an iterator to one past the last child of this node.
		node_iterator child_end() const
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

#if 0
	/// Creates a tree or branch with the given root data.
	simple_tree(const T& root_data, TDepth root_depth = 0)
	{
		_nodes.push_back(node_t(root_data, root_depth));
	}
#endif

	/// Creates a tree with the given root data.
	simple_tree(const T& root_data)
	{
		_nodes.push_back(node_t(root_data, 0));
	}

	/// Returns the number of nodes in the tree.
	size_t size() const { return _nodes.size(); }

	/// Returns an iterator to the root of the tree.
	iterator root() { return iterator(this, 0); }

	/// Returns a const iterator to the root of the tree.
	const_iterator root() const { return const_iterator(this, 0); }

	/// Visits all nodes under a given root in natural order.
	template <bool IsConst, class Func>
	void traverse(node_iterator<IsConst> root, Func f) const
	{
		TDepth root_depth = _nodes[root.index()].depth;
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
			if (_nodes[i].depth <= root_depth)
				break;
			f(_nodes[i].depth - root_depth, it);
		}
	}

	/**
	 * Inserts a new node as the last child of an existing node.
	 *
	 * @param where Iterator to the parent node.
	 * @param data The node data.
	 * @returns An iterator to the added node.
	 *
	 * @remarks If the child is added to the last sibling of any level, all
	 *      iterators are still valid. Otherwise, only the iterators on the
	 *      parent path of the child is guaranteed to be valid.
	 */
	iterator insert_child(const_iterator where, const T& data)
	{
		assert(where._tree == this);

		// For now, we only support adding to the end of the tree.
		assert((++const_iterator(where)).index() == _nodes.size());

		// Append the child to the end of the tree.
		_nodes.push_back(node_t(data, where.depth() + 1));
		return iterator(this, _nodes.size() - 1);
	}

	/**
	 * Inserts another tree as child of an existing node.
	 *
	 * @param where Iterator to the parent node.
	 * @param subtree A subtree to insert.
	 * @param has_root Flag indicating whether the subtree is rooted. If this
	 *      argument is @c true, the root node of the subtree is inserted as
	 *      a child of the existing node. If this argument is @c false, the
	 *      level-1 nodes of the subtree are inserted as the children.
	 * @returns An iterator to the added node.
	 *
	 * @remarks If the child is added to the last sibling of any level, all
	 *      iterators are still valid. Otherwise, only the iterators on the
	 *      parent path of the child is guaranteed to be valid.
	 */
	iterator insert_child(
		const_iterator where,
		const simple_tree &subtree, 
		bool has_root = true)
	{
		assert(where._tree == this);

		// For now, we only support adding to the end of the tree.
		assert((++const_iterator(where)).index() == _nodes.size());

		iterator ret(this, _nodes.size());

		// Append the subtree to the end of the tree. We need to update the
		// depth field of the tree.
		size_t offset = has_root ? 0 : 1;
		_nodes.reserve(_nodes.size() + subtree._nodes.size() - offset);
		size_t base_depth = where.depth() + 1 - offset;
		size_t n = subtree._nodes.size();
		for (size_t i = offset; i < n; ++i)
		{
			const node_t &child = subtree._nodes[i];
			_nodes.push_back(node_t(child.data, child.depth + (TDepth)base_depth));
		}
		return ret;
	}
};

} // namespace util

#endif // UTILITIES_SIMPLE_TREE_HPP
