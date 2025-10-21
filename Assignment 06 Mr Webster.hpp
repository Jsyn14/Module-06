// dictionary.hpp
#pragma once
#include <compare>
#include <memory>
#include <vector>
#include <stdexcept>

template <class T>
using List = std::vector<T>;

template <class Key, class Value>
struct Pair {
    Key   first{};
    Value second{};
    auto operator<=>(const Pair&) const = default;
};

template <class Key, class Value>
class Dictionary {
    struct Node {
        Pair<Key,Value> kv;
        std::unique_ptr<Node> left, right;
        explicit Node(Pair<Key,Value> p): kv(std::move(p)) {}
    };
    std::unique_ptr<Node> root_;
    std::size_t n_ = 0;
    List<Key>   ks_;
    List<Value> vs_;

public:
    Dictionary() = default;
    Dictionary(const Dictionary& o) { root_ = clone(o.root_); n_ = o.n_; }
    Dictionary& operator=(const Dictionary& o) {
        if (this != &o) { root_ = clone(o.root_); n_ = o.n_; }
        return *this;
    }
    Dictionary(Dictionary&&) noexcept = default;
    Dictionary& operator=(Dictionary&&) noexcept = default;
    ~Dictionary() = default;

    Value& operator[](const Key& k) {
        return sub_or_ins(root_, k, /*create*/true);
    }
    Value& at(const Key& k) {
        Node* t = find(root_.get(), k);
        if (!t) throw std::out_of_range("key not found");
        return t->kv.second;
    }
    const Value& at(const Key& k) const {
        const Node* t = find(root_.get(), k);
        if (!t) throw std::out_of_range("key not found");
        return t->kv.second;
    }

    bool insert(const Pair<Key,Value>& kv) { return ins_unique(root_, kv); }
    bool insert(const Key& k, const Value& v) { return insert(Pair<Key,Value>{k,v}); }

    bool erase(const Key& k) { return erase_rec(root_, k); }
    void clear() { root_.reset(); n_ = 0; ks_.clear(); vs_.clear(); }

    List<Key>& keys()   { ks_.clear();   ks_.reserve(n_); inorder(root_.get(), [this](auto* x){ ks_.push_back(x->kv.first);  }); return ks_; }
    List<Value>& values(){ vs_.clear();  vs_.reserve(n_); inorder(root_.get(), [this](auto* x){ vs_.push_back(x->kv.second); }); return vs_; }

    bool isEmpty() const noexcept { return n_ == 0; }
    std::size_t size() const noexcept { return n_; }

private:
    static std::unique_ptr<Node> clone(const std::unique_ptr<Node>& p) {
        if (!p) return nullptr;
        auto q = std::make_unique<Node>(p->kv);
        q->left  = clone(p->left);
        q->right = clone(p->right);
        return q;
    }

    static Node* find(Node* t, const Key& k) {
        while (t) {
            if (k < t->kv.first) t = t->left.get();
            else if (t->kv.first < k) t = t->right.get();
            else return t;
        }
        return nullptr;
    }

    Value& sub_or_ins(std::unique_ptr<Node>& t, const Key& k, bool create) {
        if (!t) {
            if (!create) throw std::out_of_range("key not found");
            t = std::make_unique<Node>(Pair<Key,Value>{k, Value{}});
            ++n_;
            return t->kv.second;
        }
        if (k < t->kv.first)       return sub_or_ins(t->left,  k, create);
        if (t->kv.first < k)       return sub_or_ins(t->right, k, create);
        return t->kv.second;
    }

    bool ins_unique(std::unique_ptr<Node>& t, const Pair<Key,Value>& kv) {
        if (!t) { t = std::make_unique<Node>(kv); ++n_; return true; }
        if (kv.first < t->kv.first)  return ins_unique(t->left, kv);
        if (t->kv.first < kv.first)  return ins_unique(t->right, kv);
        return false; // key exists
    }

    static Node* min_node(Node* t) {
        while (t && t->left) t = t->left.get();
        return t;
    }

    bool erase_rec(std::unique_ptr<Node>& t, const Key& k) {
        if (!t) return false;
        if (k < t->kv.first)  return erase_rec(t->left, k);
        if (t->kv.first < k)  return erase_rec(t->right, k);
        // found
        if (!t->left) { t = std::move(t->right); --n_; return true; }
        if (!t->right){ t = std::move(t->left);  --n_; return true; }
        // two children: copy successor then erase successor
        Node* s = min_node(t->right.get());
        t->kv = s->kv;
        return erase_rec(t->right, s->kv.first); // size-- happens in recursive delete
    }

    template<class F>
    static void inorder(Node* t, F&& f) {
        if (!t) return;
        inorder(t->left.get(), f);
        f(t);
        inorder(t->right.get(), f);
    }
};
