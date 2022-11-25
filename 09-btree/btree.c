#include <solution.h>
#include <malloc.h>
#include <string.h>

struct btree_node
{
	int is_leaf;  /* is this a leaf node? */
	int num_keys; /* how many keys does this node contain? */
	int min_keys;
	int *keys;
	struct btree_node **kids; /* kids[i] holds nodes < keys[i] */
};

struct btree
{
	int min_keys;
	struct btree_node *root;
};

struct btree_node *btree_node_alloc(unsigned int L)
{
	struct btree_node *node = (struct btree_node *)malloc(sizeof(struct btree_node));
	node->min_keys = L;
	node->is_leaf = 1;
	node->num_keys = 0;
	node->keys = NULL;
	node->kids = NULL;
	return node;
}

struct btree *btree_alloc(unsigned int L)
{
	struct btree *tree = malloc(sizeof(struct btree *));
	tree->min_keys = L;
    tree->root = btree_node_alloc(L);
	return tree;
}

void btree_node_free(struct btree_node *node)
{
	printf("node %p\n", node);
	if (!node)
	{
		return;
	}
	if (!node->is_leaf)
	{
		for (int i = 0; i <= node->num_keys; i++)
		{
			printf("%d\n", i);
			btree_node_free(node->kids[i]);
		}
	}
	free(node->kids);
	free(node->keys);
	free(node);
}
void btree_free(struct btree *t)
{
	if (!t)
	{
		return;
	}
	if (!t->root)
	{
		free(t);
		return;
	}
	btree_node_free(t->root);
	free(t);
	return;
}

static bool search_key(int n, const int *a, int key)
{
	int lo;
	int hi;
	int mid;

	/* invariant: a[lo] < key <= a[hi] */
	lo = -1;
	hi = n;

	while (lo + 1 < hi)
	{
		mid = (lo + hi) / 2;
		if (a[mid] == key)
		{
			return mid;
		}
		else if (a[mid] < key)
		{
			lo = mid;
		}
		else
		{
			hi = mid;
		}
	}

	return hi;
}

static struct btree_node *insert_internal(struct btree_node *b, int key, int *median)
{
	int pos;
	int mid;
	struct btree_node *b2;

	pos = search_key(b->num_keys, b->keys, key);

	if (pos < b->num_keys && b->keys[pos] == key)
	{
		/* nothing to do */
		return 0;
	}

	if (b->is_leaf)
	{

		/* everybody above pos moves up one space */
		memmove(&b->keys[pos + 1], &b->keys[pos], sizeof(*(b->keys)) * (b->num_keys - pos));
		b->keys[pos] = key;
		b->num_keys++;
	}
	else
	{

		/* insert in child */
		b2 = insert_internal(b->kids[pos], key, &mid);

		/* maybe insert a new key in b */
		if (b2)
		{

			/* every key above pos moves up one space */
			memmove(&b->keys[pos + 1], &b->keys[pos], sizeof(*(b->keys)) * (b->num_keys - pos));
			/* new kid goes in pos + 1*/
			memmove(&b->kids[pos + 2], &b->kids[pos + 1], sizeof(*(b->keys)) * (b->num_keys - pos));

			b->keys[pos] = mid;
			b->kids[pos + 1] = b2;
			b->num_keys++;
		}
	}

	/* we waste a tiny bit of space by splitting now
	 * instead of on next insert */
	if (b->num_keys >= 2 * b->min_keys)
	{
		mid = b->num_keys / 2;

		*median = b->keys[mid];

		/* make a new node for keys > median */
		/* picture is:
		 *
		 *      3 5 7
		 *      a b c d
		 *
		 * becomes
		 *          (5)
		 *      3        7
		 *      a b      c d
		 */
		b2 = malloc(sizeof(*b2));

		b2->num_keys = b->num_keys - mid - 1;
		b2->is_leaf = b->is_leaf;

		memmove(b2->keys, &b->keys[mid + 1], sizeof(*(b->keys)) * b2->num_keys);
		if (!b->is_leaf)
		{
			memmove(b2->kids, &b->kids[mid + 1], sizeof(*(b->kids)) * (b2->num_keys + 1));
		}

		b->num_keys = mid;

		return b2;
	}
	else
	{
		return 0;
	}
}

void btree_insert(struct btree *t, int x)
{
	struct btree_node *b1; /* new left child */
	struct btree_node *b2; /* new right child */
	struct btree_node *b = t->root;
	int median;

	b2 = insert_internal(b, x, &median);

	if (b2)
	{
		/* basic issue here is that we are at the root */
		/* so if we split, we have to make a new root */

		b1 = malloc(sizeof(*b1));

		/* copy root to b1 */
		memmove(b1, b, sizeof(*b));

		/* make root point to b1 and b2 */
		b->num_keys = 1;
		b->is_leaf = 0;
		b->keys[0] = median;
		b->kids[0] = b1;
		b->kids[1] = b2;
	}
}

void btree_delete(struct btree *t, int x)
{
	(void)t;
	(void)x;
	return;
}

bool btree_search(struct btree_node *b, int x) {
	
	int pos;
	/* have to check for empty tree */
	if (b->num_keys == 0)
	{
		return 0;
	}

	/* look for smallest position that key fits below */
	pos = search_key(b->num_keys, b->keys, x);

	if (pos < b->num_keys && b->keys[pos] == x)
	{
		return 1;
	}
	else
	{
		return (!b->is_leaf && btree_search(b->kids[pos], x));
	}
}
bool btree_contains(struct btree *t, int x)
{
	return btree_search(t->root, x);
	
}

struct btree_iter
{
	struct btree *tree;
	struct btree_node **path;
	int cur_depth;
	int *path_indexes;
};

struct btree_iter *btree_iter_start(struct btree *t)
{
	struct btree_iter *i = (struct btree_iter *)malloc(sizeof(struct btree_iter));
	i->tree = t;
	int max_depth = 0;
	struct btree_node *cur_node = t->root;
	while (!cur_node->is_leaf)
	{
		cur_node = cur_node->kids[0];
		max_depth += 1;
	}
	max_depth += 1;
	// printf("depth %d\n", max_depth);
	i->path = malloc(max_depth * sizeof(struct btree_node *));
	i->path_indexes = (int *)malloc(max_depth * sizeof(int *));

	i->cur_depth = 0;
	i->path[0] = t->root;
	i->path_indexes[0] = -1;
	return i;
}

void btree_iter_end(struct btree_iter *i)
{
	free(i->path);
	free(i->path_indexes);
	free(i);
}

bool btree_iter_next(struct btree_iter *i, int *x)
{
	// if(!i->path[i->current_depth]->is_leaf){
	// 	i->path[i->current_depth + 1] = i->path[i->current_depth]->children[i->indexes[i->current_depth]+1];
	// 	i->indexes[i->current_depth + 1] = 0;
	// 	i->current_depth += 1;
	// }
	if (i->path[i->cur_depth]->is_leaf)
	{
		if (i->path_indexes[i->cur_depth] < i->path[i->cur_depth]->num_keys - 1)
		{
			i->path_indexes[i->cur_depth] += 1;
		}
		else
		{
			i->cur_depth -= 1;
			//если самый правый сын
			while (i->path_indexes[i->cur_depth] == i->path[i->cur_depth]->num_keys)
			{
				if (i->cur_depth == 0)
				{
					return false;
				}
				else
				{
					i->cur_depth -= 1;
				}
			}
		}
	}
	else
	{
		i->path_indexes[i->cur_depth] += 1;
		while (!i->path[i->cur_depth]->is_leaf)
		{
			i->path[i->cur_depth + 1] = i->path[i->cur_depth]->kids[i->path_indexes[i->cur_depth]];
			i->path_indexes[i->cur_depth + 1] = 0;
			i->cur_depth += 1;
		}
	}

	*x = i->path[i->cur_depth]->keys[i->path_indexes[i->cur_depth]];
	return true;
}