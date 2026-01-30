/**
 * implement a container like std::linked_hashmap
 */
#ifndef SJTU_LINKEDHASHMAP_HPP
#define SJTU_LINKEDHASHMAP_HPP

// only for std::equal_to<T> and std::hash<T>
#include <functional>
#include <cstddef>
#include "utility.hpp"
#include "exceptions.hpp"

namespace sjtu {
    /**
     * In linked_hashmap, iteration ordering is differ from map,
     * which is the order in which keys were inserted into the map.
     * You should maintain a doubly-linked list running through all
     * of its entries to keep the correct iteration order.
     *
     * Note that insertion order is not affected if a key is re-inserted
     * into the map.
     */
    
template<
	class Key,
	class T,
	class Hash = std::hash<Key>, 
	class Equal = std::equal_to<Key>
> class linked_hashmap {
public:
	/**
	 * the internal type of data.
	 * it should have a default constructor, a copy constructor.
	 * You can use sjtu::linked_hashmap as value_type by typedef.
	 */
	typedef pair<const Key, T> value_type;

private:
	struct LinkNode {
		LinkNode *order_prev;
		LinkNode *order_next;
		LinkNode() : order_prev(nullptr), order_next(nullptr) {}
	};
	struct Node : LinkNode {
		value_type data;
		Node *next_in_bucket;
		Node(const Key &k, const T &v) : LinkNode(), data(k, v), next_in_bucket(nullptr) {}
	};

	static const size_t INIT_CAPACITY = 16;
	static const double LOAD_FACTOR;

	Node **buckets;
	LinkNode *order_head;
	LinkNode *order_tail;
	size_t bucket_capacity;
	size_t num_elements;
	Hash hasher;
	Equal key_equal;

	size_t get_bucket_index(const Key &key) const {
		return hasher(key) % bucket_capacity;
	}

	void init_empty() {
		order_head = new LinkNode();
		order_tail = new LinkNode();
		order_head->order_next = order_tail;
		order_tail->order_prev = order_head;
	}

	void rehash() {
		size_t new_capacity = bucket_capacity * 2;
		Node **new_buckets = new Node*[new_capacity];
		for (size_t i = 0; i < new_capacity; i++) new_buckets[i] = nullptr;

		Node *cur = static_cast<Node*>(order_head->order_next);
		while (cur != static_cast<Node*>(order_tail)) {
			size_t idx = hasher(cur->data.first) % new_capacity;
			cur->next_in_bucket = new_buckets[idx];
			new_buckets[idx] = cur;
			cur = static_cast<Node*>(cur->order_next);
		}

		delete[] buckets;
		buckets = new_buckets;
		bucket_capacity = new_capacity;
	}

public:
	/**
	 * see BidirectionalIterator at CppReference for help.
	 *
	 * if there is anything wrong throw invalid_iterator.
	 *     like it = linked_hashmap.begin(); --it;
	 *       or it = linked_hashmap.end(); ++end();
	 */
	class const_iterator;
	class iterator {
	private:
		LinkNode *node;
		linked_hashmap *map_ptr;
		Node *get_node() const { return static_cast<Node*>(node); }
	public:
		// The following code is written for the C++ type_traits library.
		// Type traits is a C++ feature for describing certain properties of a type.
		// For instance, for an iterator, iterator::value_type is the type that the 
		// iterator points to. 
		// STL algorithms and containers may use these type_traits (e.g. the following 
		// typedef) to work properly. 
		// See these websites for more information:
		// https://en.cppreference.com/w/cpp/header/type_traits
		// About value_type: https://blog.csdn.net/u014299153/article/details/72419713
		// About iterator_category: https://en.cppreference.com/w/cpp/iterator
		using difference_type = std::ptrdiff_t;
		using value_type = typename linked_hashmap::value_type;
		using pointer = value_type*;
		using reference = value_type&;
		using iterator_category = std::output_iterator_tag;


		iterator() : node(nullptr), map_ptr(nullptr) {}
		iterator(LinkNode *n, linked_hashmap *m) : node(n), map_ptr(m) {}
		iterator(const iterator &other) : node(other.node), map_ptr(other.map_ptr) {}
		/**
		 * TODO iter++
		 */
		iterator operator++(int) {
			if (map_ptr == nullptr || node == nullptr) throw invalid_iterator();
			if (node == map_ptr->order_tail) throw invalid_iterator();
			iterator tmp = *this;
			node = node->order_next;
			return tmp;
		}
		/**
		 * TODO ++iter
		 */
		iterator & operator++() {
			if (map_ptr == nullptr || node == nullptr) throw invalid_iterator();
			if (node == map_ptr->order_tail) throw invalid_iterator();
			node = node->order_next;
			return *this;
		}
		/**
		 * TODO iter--
		 */
		iterator operator--(int) {
			if (map_ptr == nullptr || node == nullptr) throw invalid_iterator();
			if (node == map_ptr->order_head->order_next) throw invalid_iterator();
			iterator tmp = *this;
			node = node->order_prev;
			return tmp;
		}
		/**
		 * TODO --iter
		 */
		iterator & operator--() {
			if (map_ptr == nullptr || node == nullptr) throw invalid_iterator();
			if (node == map_ptr->order_head->order_next) throw invalid_iterator();
			node = node->order_prev;
			return *this;
		}
		/**
		 * a operator to check whether two iterators are same (pointing to the same memory).
		 */
		value_type & operator*() const {
			if (map_ptr == nullptr || node == nullptr || node == map_ptr->order_tail || node == map_ptr->order_head) throw invalid_iterator();
			return get_node()->data;
		}
		bool operator==(const iterator &rhs) const { return node == rhs.node && map_ptr == rhs.map_ptr; }
		bool operator==(const const_iterator &rhs) const { return node == rhs.node && map_ptr == rhs.map_ptr; }
		/**
		 * some other operator for iterator.
		 */
		bool operator!=(const iterator &rhs) const { return !(*this == rhs); }
		bool operator!=(const const_iterator &rhs) const { return !(*this == rhs); }

		/**
		 * for the support of it->first. 
		 * See <http://kelvinh.github.io/blog/2013/11/20/overloading-of-member-access-operator-dash-greater-than-symbol-in-cpp/> for help.
		 */
		value_type* operator->() const noexcept { return &(get_node()->data); }

		friend class linked_hashmap;
		friend class const_iterator;
	};
 
	class const_iterator {
	private:
		const LinkNode *node;
		const linked_hashmap *map_ptr;
		const Node *get_node() const { return static_cast<const Node*>(node); }
	public:
		using difference_type = std::ptrdiff_t;
		using value_type = typename linked_hashmap::value_type;
		using pointer = const value_type*;
		using reference = const value_type&;
		using iterator_category = std::output_iterator_tag;

		const_iterator() : node(nullptr), map_ptr(nullptr) {}
		const_iterator(const LinkNode *n, const linked_hashmap *m) : node(n), map_ptr(m) {}
		const_iterator(const const_iterator &other) : node(other.node), map_ptr(other.map_ptr) {}
		const_iterator(const iterator &other) : node(other.node), map_ptr(other.map_ptr) {}

		const_iterator operator++(int) {
			if (map_ptr == nullptr || node == nullptr) throw invalid_iterator();
			if (node == map_ptr->order_tail) throw invalid_iterator();
			const_iterator tmp = *this;
			node = node->order_next;
			return tmp;
		}
		const_iterator & operator++() {
			if (map_ptr == nullptr || node == nullptr) throw invalid_iterator();
			if (node == map_ptr->order_tail) throw invalid_iterator();
			node = node->order_next;
			return *this;
		}
		const_iterator operator--(int) {
			if (map_ptr == nullptr || node == nullptr) throw invalid_iterator();
			if (node == map_ptr->order_head->order_next) throw invalid_iterator();
			const_iterator tmp = *this;
			node = node->order_prev;
			return tmp;
		}
		const_iterator & operator--() {
			if (map_ptr == nullptr || node == nullptr) throw invalid_iterator();
			if (node == map_ptr->order_head->order_next) throw invalid_iterator();
			node = node->order_prev;
			return *this;
		}
		const value_type & operator*() const {
			if (map_ptr == nullptr || node == nullptr || node == map_ptr->order_tail || node == map_ptr->order_head) throw invalid_iterator();
			return get_node()->data;
		}
		bool operator==(const iterator &rhs) const { return node == rhs.node && map_ptr == rhs.map_ptr; }
		bool operator==(const const_iterator &rhs) const { return node == rhs.node && map_ptr == rhs.map_ptr; }
		bool operator!=(const iterator &rhs) const { return !(*this == rhs); }
		bool operator!=(const const_iterator &rhs) const { return !(*this == rhs); }
		const value_type* operator->() const noexcept { return &(get_node()->data); }

		friend class linked_hashmap;
	};

public:
	/**
	 * TODO two constructors
	 */
	linked_hashmap() : bucket_capacity(INIT_CAPACITY), num_elements(0) {
		buckets = new Node*[INIT_CAPACITY];
		for (size_t i = 0; i < INIT_CAPACITY; i++) buckets[i] = nullptr;
		init_empty();
	}
	linked_hashmap(const linked_hashmap &other) : bucket_capacity(other.bucket_capacity), num_elements(0) {
		buckets = new Node*[bucket_capacity];
		for (size_t i = 0; i < bucket_capacity; i++) buckets[i] = nullptr;
		init_empty();
		for (const_iterator it = other.cbegin(); it != other.cend(); ++it) {
			insert(*it);
		}
	}
 
	/**
	 * TODO assignment operator
	 */
	linked_hashmap & operator=(const linked_hashmap &other) {
		if (this == &other) return *this;
		clear();
		delete[] buckets;
		delete static_cast<LinkNode*>(order_head);
		delete static_cast<LinkNode*>(order_tail);
		bucket_capacity = other.bucket_capacity;
		num_elements = 0;
		buckets = new Node*[bucket_capacity];
		for (size_t i = 0; i < bucket_capacity; i++) buckets[i] = nullptr;
		init_empty();
		for (const_iterator it = other.cbegin(); it != other.cend(); ++it) {
			insert(*it);
		}
		return *this;
	}
 
	/**
	 * TODO Destructors
	 */
	~linked_hashmap() {
		clear();
		delete[] buckets;
		delete static_cast<LinkNode*>(order_head);
		delete static_cast<LinkNode*>(order_tail);
	}
 
	/**
	 * TODO
	 * access specified element with bounds checking
	 * Returns a reference to the mapped value of the element with key equivalent to key.
	 * If no such element exists, an exception of type `index_out_of_bound'
	 */
	T & at(const Key &key) {
		iterator it = find(key);
		if (it == end()) throw index_out_of_bound();
		return it->second;
	}
	const T & at(const Key &key) const {
		const_iterator it = find(key);
		if (it == cend()) throw index_out_of_bound();
		return it->second;
	}
 
	/**
	 * TODO
	 * access specified element 
	 * Returns a reference to the value that is mapped to a key equivalent to key,
	 *   performing an insertion if such key does not already exist.
	 */
	T & operator[](const Key &key) {
		iterator it = find(key);
		if (it != end()) return it->second;
		pair<iterator, bool> p = insert(value_type(key, T()));
		return p.first->second;
	}
 
	/**
	 * behave like at() throw index_out_of_bound if such key does not exist.
	 */
	const T & operator[](const Key &key) const {
		return at(key);
	}
 
	/**
	 * return a iterator to the beginning
	 */
	iterator begin() { return iterator(static_cast<LinkNode*>(order_head->order_next), this); }
	const_iterator cbegin() const { return const_iterator(static_cast<const LinkNode*>(order_head->order_next), this); }
 
	/**
	 * return a iterator to the end
	 * in fact, it returns past-the-end.
	 */
	iterator end() { return iterator(static_cast<LinkNode*>(order_tail), this); }
	const_iterator cend() const { return const_iterator(static_cast<const LinkNode*>(order_tail), this); }
 
	/**
	 * checks whether the container is empty
	 * return true if empty, otherwise false.
	 */
	bool empty() const { return num_elements == 0; }
 
	/**
	 * returns the number of elements.
	 */
	size_t size() const { return num_elements; }
 
	/**
	 * clears the contents
	 */
	void clear() {
		Node *cur = static_cast<Node*>(order_head->order_next);
		while (cur != static_cast<Node*>(order_tail)) {
			Node *nxt = static_cast<Node*>(cur->order_next);
			delete cur;
			cur = nxt;
		}
		order_head->order_next = order_tail;
		order_tail->order_prev = order_head;
		for (size_t i = 0; i < bucket_capacity; i++) buckets[i] = nullptr;
		num_elements = 0;
	}
 
	/**
	 * insert an element.
	 * return a pair, the first of the pair is
	 *   the iterator to the new element (or the element that prevented the insertion), 
	 *   the second one is true if insert successfully, or false.
	 */
	pair<iterator, bool> insert(const value_type &value) {
		iterator it = find(value.first);
		if (it != end()) return pair<iterator, bool>(it, false);

		if (num_elements + 1 > bucket_capacity * LOAD_FACTOR) rehash();

		Node *new_node = new Node(value.first, value.second);
		size_t idx = get_bucket_index(value.first);
		new_node->next_in_bucket = buckets[idx];
		buckets[idx] = new_node;

		new_node->order_prev = order_tail->order_prev;
		new_node->order_next = order_tail;
		order_tail->order_prev->order_next = new_node;
		order_tail->order_prev = new_node;

		num_elements++;
		return pair<iterator, bool>(iterator(static_cast<LinkNode*>(new_node), this), true);
	}
 
	/**
	 * erase the element at pos.
	 *
	 * throw if pos pointed to a bad element (pos == this->end() || pos points an element out of this)
	 */
	void erase(iterator pos) {
		if (pos.map_ptr != this || pos.node == nullptr) throw invalid_iterator();
		if (pos.node == order_tail || pos.node == order_head) throw invalid_iterator();

		Node *cur = static_cast<Node*>(pos.node);
		size_t idx = get_bucket_index(cur->data.first);
		Node *prev = nullptr;
		Node *p = buckets[idx];
		while (p != cur) {
			prev = p;
			p = p->next_in_bucket;
		}
		if (prev) prev->next_in_bucket = cur->next_in_bucket;
		else buckets[idx] = cur->next_in_bucket;

		cur->order_prev->order_next = cur->order_next;
		cur->order_next->order_prev = cur->order_prev;

		delete cur;
		num_elements--;
	}
 
	/**
	 * Returns the number of elements with key 
	 *   that compares equivalent to the specified argument,
	 *   which is either 1 or 0 
	 *     since this container does not allow duplicates.
	 */
	size_t count(const Key &key) const { return find(key) != cend() ? 1 : 0; }
 
	/**
	 * Finds an element with key equivalent to key.
	 * key value of the element to search for.
	 * Iterator to an element with key equivalent to key.
	 *   If no such element is found, past-the-end (see end()) iterator is returned.
	 */
	iterator find(const Key &key) {
		size_t idx = get_bucket_index(key);
		Node *p = buckets[idx];
		while (p != nullptr) {
			if (key_equal(p->data.first, key)) return iterator(p, this);
			p = p->next_in_bucket;
		}
		return end();
	}
	const_iterator find(const Key &key) const {
		size_t idx = get_bucket_index(key);
		Node *p = buckets[idx];
		while (p != nullptr) {
			if (key_equal(p->data.first, key)) return const_iterator(p, this);
			p = p->next_in_bucket;
		}
		return cend();
	}
};

template<class Key, class T, class Hash, class Equal>
const double linked_hashmap<Key, T, Hash, Equal>::LOAD_FACTOR = 0.75;

}

#endif
