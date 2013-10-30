#ifndef _WHEEL_RBTREE_H_
#define _WHEEL_RBTREE_H_

#include <multi_queue.h>
#include <custom_new.h>
#include <cstdlib>

namespace wheels
{
    /*
     *	_CMP(l,r) >0 gt; =0 eq; <0 lt
     */
    template<typename _K, typename _V, typename _CMP, typename _Alloc>
    class llrbtree_t
    {
    private:
        enum qid_t
        {
            lfree = 0,
            lused = 1,
        };

        enum color_t
        {
            rb_red = 0,
            rb_black = 1,
        };

        struct node_t
        {
            node_t()
            {
                reset();
            }

            node_t(const _K& k, const _V& v):
                key_(k), val_(v)
            {
                reset();
            }

            inline void reset()
            {
                left_ = NULL;
                right_ = NULL;
                size_ = 1;
                color_ = rb_red;
            }

            node_t* left_;
            node_t* right_;
            int size_;
            int color_;
            _K key_;
            _V val_;
        };

        // root node
        node_t* root_;
        _Alloc* alloc_;
        _CMP key_comp_;
    public:
        llrbtree_t()
        {
            initialize();
        }

        virtual ~llrbtree_t()
        {
            finalize();
        }

        int initialize(_Alloc* allocator)
        {
            root_ = NULL;
            alloc_ = allocator;
            return 0;
        }

        void finalize()
        {
            root_ = NULL;
        }

        const node_t* get_root() const { return root_; }

        node_t* get_root() { return root_; }

        int insert(const _K& k, const _V& v)
        {
            node_t* node = new(*alloc_) node_t(k, v);
            if (NULL == node)
            {
                // out of memory?
                return -1;
            }

            root_ = insert(root_, *node);
            root_->color_ = rb_black;
            return 0;
        }

        int remove(const _K& key)
        {
            if (NULL == root_)
            {
                return -1;
            }

            // find node by key first
            node_t* n = get_by_key(root_, key);
            if (NULL == n)
            {
                return -1;
            }

            if (!is_red(root_->left_) && !is_red(root_->right_))
            {
                root_->color_ = rb_red;
            }

            // remove node from tree
            root_ = remove(*root_, key);
            if (root_)
            {
                root_->color_ = rb_black;
            }

            // dealloc node
            operator delete(n, *alloc_);
            return 0;
        }

        _V* get_by_key(const _K& key)
        {
            node_t* n = get_by_key(root_, key);
            return n? &n->val_: NULL;
        }

        _V* get_ceiling(const _K& key)
        {
            node_t* n = get_ceiling(root_, key);
            return n? &n->val_: NULL;
        }

        _V* get_floor(const _K& key)
        {
            node_t* n = get_floor(root_, key);
            return n? &n->val_: NULL;
        }

        _V* get_by_rank(int rank)
        {
            node_t* n = get_by_rank(root_, rank);
            return n? &n->val_: NULL;
        }

        int get_rank(const _K& key) const { return get_rank(root_, key); }

        _K* get_min_key()
        {
            if (NULL == root_) return NULL;
            node_t& m = get_min(*root_);
            return &m.key_;
        }
    private:
        // deny copy-cons
        llrbtree_t(const llrbtree_t& c) {}
        // flip color
        void flip(node_t* root)
        {
            if (NULL == root) return;
            root->color_ = (rb_red == root->color_)? rb_black: rb_red;
        }

        // insert node under root, return new root
        node_t* insert(node_t* root, node_t& node)
        {
            if (NULL == root)
            {
                node.color_ = rb_red;
                node.size_ = 1;
                return &node;
            }

            int cmp = key_comp_(node.key_, root->key_);
            if (cmp < 0)
            {
                root->left_ = insert(root->left_, node);
            }
            else if (cmp > 0)
            {
                root->right_ = insert(root->right_, node);
            }
            else
            {
                // TODO: dup key, report error. for now we do nothing
                return root;
            }

            // handle non left-lean and multi nodes situation
            if (is_red(root->right_) && !is_red(root->left_))
            {
                root = rotate_left(*root);
            }

            if (is_red(root->left_) && is_red(root->left_->left_))
            {
                root = rotate_right(*root);
            }

            if (is_red(root->left_) && is_red(root->right_))
            {
                flip_color(*root);
            }

            update_size(*root);
            return root;
        }

        // rotate-left root, return new root
        node_t* rotate_left(node_t& root)
        {
            node_t* rchild = root.right_;
            if (NULL == rchild || !is_red(rchild))
            {
                // error
                return NULL;
            }

            root.right_ = rchild->left_;
            rchild->left_ = &root;
            rchild->color_ = root.color_;
            root.color_ = rb_red;
            rchild->size_ = root.size_;
            update_size(root);
            return rchild;
        }

        // rotate-right root, return new root
        node_t* rotate_right(node_t& root)
        {
            // lchild must be red and cant be null
            node_t* lchild = root.left_;
            if (NULL == lchild || !is_red(lchild))
            {
                //error
                return NULL;
            }

            root.left_ = lchild->right_;
            lchild->right_ = &root;
            lchild->color_ = root.color_;
            root.color_ = rb_red;
            lchild->size_ = root.size_;
            update_size(root);
            return lchild;
        }

        // move red node down, make sure left child has red. return new root
        node_t* move_red_left(node_t& root)
        {
            // move red down to leftchild, create a 4-node
            if (!is_red(&root) || is_red(root.left_) || is_red(root.left_->left_))
            {
                // fatal, should not happen
                abort();
            }

            flip_color(root);
            if (is_red(root.right_->left_))
            {
                // if a 5-node created, return one to parent and split a 2-node and 3-node
                root.right_ = rotate_right(*root.right_);
                node_t* n = rotate_left(root);
                flip_color(*n);
                return n;
            }

            return &root;
        }

        // move red node down, make sure right child has red
        node_t* move_red_right(node_t& root)
        {
            if (!is_red(&root) || is_red(root.right_) || is_red(root.right_->left_))
            {
                // fatal, should not happen
                abort();
            }

            flip_color(root);
            if (is_red(root.left_->left_))
            {
                node_t *n = rotate_right(root);
                flip_color(*n);
                return n;
            }

            return &root;
        }

        // flip color
        void flip_color(node_t& root)
        {
            flip(&root);
            flip(root.left_);
            flip(root.right_);
        }

        // remove node by key and return new root, NULL if tree is emtpy after deleting. if key does not exist, do nothing and return root
        node_t* remove(node_t& root, const _K& key)
        {
            // ensure root isred || root.left isred during recurse
            node_t* n = &root;
            if (key_comp_(key, n->key_) < 0)
            {
                // delete under left child, adjust if l==black && ll==black
                if (n->left_)
                {
                    if (!is_red(n->left_) && !is_red(n->left_->left_))
                    {
                        n = move_red_left(*n);
                    }

                    // left cant be null
                    n->left_ = remove(*n->left_, key);
                }
                // else find nothing, simply return root
            }
            else
            {
                if (is_red(n->left_))
                {
                    // means root==black && root.right==black, adjust to right-lean
                    n = rotate_right(*n);
                }

                if (0 == key_comp_(key, n->key_) && NULL == n->right_)
                {
                    /*
                     *	find the key. right-lean tree right now
                     *  means if null==right, left must be NULL (and h is red)
                     */
                    return NULL;
                }

                if (n->right_)
                {
                    // ensure at least one of rightcchild or its children(right.left) is red, otherwise adjust
                    if (!is_red(n->right_) && !is_red(n->right_->left_))
                    {
                        // right-lean and right==black, so root=red root.left=black root.right=black root.right.left=black
                        n = move_red_right(*n);
                    }

                    if (0 == key_comp_(key, n->key_))
                    {
                        // remove current root n, replace it with min in right
                        node_t &rmin = get_min(*n->right_);
                        n->right_ = remove_min(*n->right_);

                        rmin.left_ = n->left_;
                        rmin.right_ = n->right_;
                        rmin.color_ = n->color_;
                        update_size(rmin);
                        n = &rmin;
                    }
                    else
                    {
                        n->right_ = remove(*n->right_, key);
                    }
                }
            }

            return balance(*n);
        }

        // remove min, ensure either root or root.left is red, return new root
        node_t* remove_min(node_t& root)
        {
            if (NULL == root.left_)
            {
                // means root is red, remove it
                return NULL;
            }

            node_t* n = &root;
            if (!is_red(root.left_) && !is_red(root.left_->left_))
            {
                // means root is red, move to left
                n = move_red_left(*n);
            }

            n->left_ = remove_min(*n->left_);
            return balance(*n);
        }

        // balance the tree
        node_t* balance(node_t& root)
        {
            node_t* n = &root;
            if (is_red(n->right_))
            {
                n = rotate_left(*n);
            }

            if (is_red(n->left_) && is_red(n->left_->left_))
            {
                n = rotate_right(*n);
            }

            if (is_red(n->left_) && is_red(n->right_))
            {
                flip_color(*n);
            }

            update_size(*n);
            return n;
        }

        // is red
        bool is_red(const node_t* node) const { return node && node->color_ == rb_red; }

        // return min node
        node_t& get_min(node_t& root)
        {
            return root.left_? get_min(*root.left_): root;
        }

        // search by rank recursively
        node_t* get_by_rank(const node_t* root, int rank)
        {
            if (NULL == root || rank <= 0 || rank > root->size_)
            {
                return NULL;
            }

            int lsize = get_size(root->left_);
            if (rank == lsize + 1)
            {
                return root;
            }
            else if (rank < lsize)
            {
                return get_by_rank(root->left_, rank);
            }

            return get_by_rank(root->right_, rank - lsize - 1);
        }

        // get rank recursively, return -1 if key not exists
        int get_rank(const node_t* root, const _K& key)
        {
            if (NULL == root)
            {
                return -1;
            }

            int cmp = key_comp_(key, root->key_);
            int lsize = get_size(root->left_);
            if (0 == cmp)
            {
                return lsize + 1;
            }
            else if (cmp < 0)
            {
                return get_rank(root->left_, key);
            }
            else
            {
                int rrank = get_rank(root->right_, key);
                return rrank < 0? -1: (lsize + 1 + rrank);
            }
        }

        // get node size
        int get_size(const node_t* root) const { return root? root->size_: 0; }

        // recaculate node size
        void update_size(node_t& root) { root.size_ = 1 + get_size(root.left_) + get_size(root.right_); }

        // search by key recursively
        node_t* get_by_key(node_t* root, const _K& key)
        {
            if (NULL == root)
            {
                return NULL;
            }

            int cmp = key_comp_(key, root->key_);
            if (cmp < 0)
            {
                return get_by_key(root->left_, key);
            }
            else if (cmp > 0)
            {
                return get_by_key(root->right_, key);
            }

            // found
            return root;
        }

        // search min node which >= key recursively
        node_t* get_floor(node_t* root, const _K& key)
        {
            if (NULL == root)
            {
                return NULL;
            }

            node_t* n = root;
            int cmp = key_comp_(key, root->key_);
            if (cmp > 0)
            {
                n = get_ceiling(root->right_, key);
                n = n? n: root;
            }
            else if (cmp < 0)
            {
                n = get_ceiling(root->left_, key);
                n = n? n: root;
            }

            return key_comp_(key, n->key_) <= 0? n: NULL;
        }

        // search max node which <= key recursively
        node_t* get_ceiling(node_t* root, const _K& key)
        {
            if (NULL == root)
            {
                return NULL;
            }

            node_t* n = root;
            int cmp = key_comp_(key, root->key_);
            if (cmp > 0)
            {
                n = get_floor(root->right_, key);
                n = n? n: root;
            }
            else if (cmp < 0)
            {
                n = get_floor(root->left_, key);
                n = n? n: root;
            }

            return key_comp_(key, n->key_) >= 0? n: NULL;
        }
    };

}

#endif
