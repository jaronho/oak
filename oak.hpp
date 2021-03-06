// a simple tree container, zlib/libpng licensed.
// - rlyeh ~~ listening to Buckethead - The Moltrail #2

#pragma once
#include <cassert>
#include <algorithm>
#include <functional>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

#define OAK_VERSION "1.0.0" // (2015/10/25) Semantic versioning adherence; fix csv() with non-string keys

namespace oak
{
	// config
#ifndef OAK_VERBOSE
	enum { OAK_VERBOSE = false };
#endif

	// tree class
	// [] means read-writeable (so is <<)
	// () means read-only access

	template<typename K, typename V = int, typename P = std::less<K>>
	class tree : public std::map<K, tree<K, V, P>, P> {

		typedef typename std::map<K, tree<K, V, P>, P> map;

		template<typename T>
		T zero() const {
			return std::pair<T, T>().first;
		}

		template<typename T>
		T& invalid() const {
			static T t;
			return t = T(), t;
		}

		V value;
		tree* parent;

	public:
		tree() : map(), value(zero<V>()) {
			parent = this;
		}

		tree(const tree& t) : map(), value(zero<V>()) {
			parent = this;
			operator=(t);
		}

		// tree clone
		tree& operator=(const tree& t) {
			if (this != &t) {
				this->clear();
				get() = zero<V>();
				operator+=(t);
			}
			return *this;
		}

		// tree merge
		tree& operator+=(const tree& t) {
			if (this != &t) {
				for (typename tree::const_iterator it = t.begin(), end = t.end(); it != end; ++it) {
					this->map::insert(*it);
				}
				get() = t.get();
			}
			return (*this);
		}

		// tree search ; const safe find: no insertions on new searches
		const tree& at(const K& t) const {
			typename map::const_iterator find = this->find(t);
			return (this->end() != find ? find->second : invalid<tree>());
		}

		// tree insertion
		tree& insert(const K& t) {
			map& children = *this;
			(children[t] = children[t]).parent = this;
			return children[t];
		}

		// tree remove
		tree& erase(const K& t) {
			typename map::iterator find = this->find(t);
			if (this->end() != find) {
				this->map::erase(t);
			}
			return (*this);
		}

		// recursive values
		V& get() {
			return value;
		}
        
		const V& get() const {
			return value;
		}

		template<typename other>
		tree& set(const other& t) {
			get() = t;
			return (*this);
		}
        
		template<typename other>
		tree& setup(const other& t) {
			if (!is_root()) {
				up().set(t).setup(t);
			}
			return (*this);
		}
        
		template<typename other>
		tree& setdown(const other& t) {
			for (typename tree::iterator it = this->begin(), end = this->end(); it != end; ++it) {
				it->second.set(t).setdown(t);
			}
			return (*this);
		}

		V getdown() const {
			V value = get();
			for (typename tree::const_iterator it = this->begin(), end = this->end(); it != end; ++it) {
				value += it->second.getdown();
			}
			return value;
		}

		// sugars

		tree& clone(const tree& t) {
			return operator=(t);
		}
		tree& assign(const tree& t) {
			return operator=(t);
		}

		template<typename other>
		tree& operator=(const other& t) {
			return set(t);
		}
		template<typename other>
		tree& operator+=(const other& t) {
			return get() += t, *this;
		}

		tree& merge(const tree& t) {
			return operator +=(t);
		}
        
		tree& operator[](const K& t) {
			return insert(t);
		}
        
		const tree& operator()(const K& t) const {
			return at(t);
		}

		bool empty(const K& t) const { // @todo: subempty
			return (this->end() == this->find(t));
		}
        
		bool has(const K& t) const {
			return !empty(t);
		}

		bool is_valid() const {
			return (this != &invalid<tree>());
		}
        
		bool operator!() const {
			return !is_valid();
		}

		const map& children() const {
			return (*this);
		}
        
		map& children() {
			return (*this);
		}

		tree& up() {
			return (*parent);
		}
        
		const tree& up() const {
			return (*parent);
		}

		bool is_root() const {
			return (parent == this);
		}
        
		const tree& root() const {
			if (!is_root()) {
				return parent->root();
			}
			return (*this);
		}
        
		tree& root() {
			if (!is_root()) {
				return parent->root();
			}
			return (*this);
		}

		// tools

		template<typename ostream>
		void csv(ostream& cout = std::cout, const std::string& prefix = std::string(), unsigned depth = 0) const {
			for (typename tree::const_iterator it = this->begin(), end = this->end(); it != end; ++it) {
				cout << prefix << "/" << it->first << "," << it->second.get() << std::endl;
				std::stringstream ss;
				ss << prefix << "/" << it->first;
				it->second.csv(cout, ss.str(), depth + 1);
			}
		}

		std::string as_csv() const {
			std::stringstream ss;
			return csv(ss), ss.str();
		}

		template<typename ostream>
		void print(ostream& cout = std::cout, unsigned depth = 0) const {
			std::string tabs(depth, '\t');
			for (typename tree::const_iterator it = this->begin(), end = this->end(); it != end; ++it) {
				cout << tabs << "[" << this->size() << "] " << it->first << " (" << it->second.get() << ")";
				if (!OAK_VERBOSE) {
					cout << std::endl;
				} else {
					cout << ".t=" << this << ",.p=" << parent << std::endl;
				}
				it->second.print(cout, depth + 1);
			}
		}

		template<typename U, typename ostream>
		void print(const std::map< K, U >& tmap, ostream& cout, unsigned depth = 0) const {
			std::string tabs(depth, '\t');
			for (typename tree::const_iterator it = this->begin(), end = this->end(); it != end; ++it) {
				cout << tabs << "[" << this->size() << "] " << tmap.find(it->first)->second << " (" << it->second.get() << ")";
				if (!OAK_VERBOSE) {
					cout << std::endl;
				} else {
					cout << ".t=" << this << ",.p=" << parent << std::endl;
				}
				it->second.print(tmap, cout, depth + 1);
			}
		}

		template<typename ostream>
		inline friend ostream& operator<<(ostream& os, const tree& self) {
			return self.print(os), os;
		}

		template<typename U>
		tree<U> rekey(const std::map< K, U >& map) const {
			tree<U> utree;
			for (typename tree::const_iterator it = this->begin(), end = this->end(); it != end; ++it) {
				typename std::map< K, U >::const_iterator find = map.find(it->first);
				assert(map.end() != find);
				utree[find->second] += it->second.rekey(map);
				utree[find->second].get() = it->second.get();
			}
			return utree;
		}
        
		template<typename U>
		tree<U> rekey(const std::map< U, K >& map) const {
			// this could be faster
			tree<U> utree;
			for (typename std::map< U, K >::const_iterator it = map.begin(), end = map.end(); it != end; ++it) {
				typename tree::const_iterator find = this->find(it->second);
				if (this->end() == find) {
					continue;
				}
				utree[it->first] += find->second.rekey(map);
				utree[it->first].get() = find->second.get();
			}
			return utree;
		}

		tree collapse() const {
			tree t;
			if (1 == this->size()) {
				return this->begin()->second.collapse();
			} else {
				for (typename tree::const_iterator it = this->begin(), end = this->end(); it != end; ++it) {
					t[it->first] += it->second.collapse();
					t[it->first].get() = it->second.get();
				}
			}
			return t;
		}

		V refresh() {
			V value = !this->size() ? get() : zero<V>();
			for (typename tree::iterator it = this->begin(), end = this->end(); it != end; ++it) {
				value += it->second.refresh();
			}
			return get() = value;
		}

		// c++03 case, user defined operator()
		// c++11 case, accept lambdas as arguments
		template<typename T>
		tree& walk(const T& predicate) {
			for (typename tree::iterator it = this->begin(), it_next = it; it != this->end(); it = it_next) {
				++it_next;
				if (predicate(*this, it)) {
					it->second.walk(predicate);
				}
			}
			return (*this);
		}

		// alias
		template<typename T>
		tree& walk() {
			return walk(T());
		}
	};
}
