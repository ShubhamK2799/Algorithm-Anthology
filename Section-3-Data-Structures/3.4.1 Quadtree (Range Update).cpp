/*

Maintain a two-dimensional array while supporting both dynamic queries and
updates of rectangular sub-arrays via the lazy propagation technique. This
implementation uses lazy initialization of nodes to conserve memory while
supporting large indices.

The query operation is defined by the join_values() and join_region() functions
where join_values(x, join_values(y, z)) = join_values(join_values(x, y), z) for
all values x, y, and z in the array. The join_region(v, area) function must be
defined in conjunction to efficiently return the result of join_values() applied
to a rectangular sub-array of area elements. The default definition below
assumes a numerical array type, supporting queries for the "min" of the target
range. Another possible query operation is "sum", for which join_values(a, b)
should return "a + b" and join_region(v, area) should return v*area.

The update operation is defined by the join_value_with_delta() and join_deltas()
functions, which determines the change made to array values. These must satisfy:
- join_deltas(d1, join_deltas(d2, d3)) = join_deltas(join_deltas(d1, d2), d3).
- join_value_with_delta(join_values(rlen*clen), d, m)) should equal
  join_values(join_value_with_delta(v, d, 1), ...(repeated m times)), which is
  a faster implementation thereof.
- if a sequence d_1, ..., d_m of deltas is used to update a value v, then
  join_value_with_delta(v, join_deltas(d_1, ..., d_m), 1) should be equivalent
  to m sequential calls to join_value_with_delta(v, d_i, 1) for i = 1..m.
The default definition below supports updates that "set" the chosen array index
to a new value. Another possible update operation is "increment", in which
join_value_with_delta(v, d, area) should be defined to return "v + d*area" and
join_deltas(d1, d2) should be defined to return "d1 + d2".

Time Complexity:
- O(1) per call to the constructor.
- O(max(MAXR, MAXC)) per call to at(), update(), and query().

Space Complexity:
- O(n) for storage of the array elements, where n is the number of updated
  entries in the array.
- O(sqrt(max(MAXR, MAXC))) auxiliary stack space per call to update(), query(),
  and at().

*/

#include <algorithm>  // std::max(), std::min()
#include <cstdlib>  // NULL

template<class T> class quadtree {
  static const int MAXR = 1000000000;
  static const int MAXC = 1000000000;

  static T join_values(const T &a, const T &b) {
    return std::min(a, b);
  }

  static T join_region(const T &v, int area) {
    return v;
  }

  static T join_value_with_delta(const T &v, const T &d, int area) {
    return d;
  }

  static T join_deltas(const T &d1, const T &d2) {
    return d2;  // For "set" updates, the more recent delta prevails.
  }

  struct node_t {
    T value, delta;
    bool pending;
    node_t *child[4];

    node_t(const T &v) {
      value = v;
      pending = false;
      for (int i = 0; i < 4; i++) {
        child[i] = NULL;
      }
    }
  };

  node_t *root;
  T init;

  // Helper variables.
  int tgt_r1, tgt_c1, tgt_r2, tgt_c2;
  T res, delta;
  bool found;

  void update_delta(node_t *&n, const T &d, int area) {
    if (n == NULL) {
      n = new node_t(join_region(init, area));
    }
    n->delta = n->pending ? join_deltas(n->delta, d) : d;
    n->pending = true;
  }

  void update_delta(node_t *&n, int r1, int c1, int r2, int c2) {
    if (n->pending) {
      int rmid = (r1 + r2)/2, cmid = (c1 + c2)/2;
      int rlen = r2 - r1 + 1, clen = c2 - c1 + 1;
      n->value = join_value_with_delta(n->value, delta, rlen*clen);
      if (rlen*clen > 1) {
        int rlen1 = rmid - r1 + 1, rlen2 = rlen - rlen1;
        int clen1 = cmid - c1 + 1, clen2 = clen - clen1;
        update_delta(n->child[0], delta, rlen1*clen1);
        update_delta(n->child[1], delta, rlen2*clen1);
        update_delta(n->child[2], delta, rlen1*clen2);
        update_delta(n->child[3], delta, rlen2*clen2);
      }
    }
    n->pending = false;
  }

  void update(node_t *&n, int r1, int c1, int r2, int c2) {
    if (n == NULL) {
      n = new node_t(join_region(init, (r2 - r1 + 1)*(c2 - r1 + 1)));
    }
    update_delta(n, r1, c1, r2, c2);
    if (tgt_r2 < r1 || tgt_r1 > r2 || tgt_c2 < c1 || tgt_c1 > c2) {
      return;
    }
    if (tgt_r1 <= r1 && r2 <= tgt_r2 && tgt_c1 <= c1 && c2 <= tgt_c2) {
      n->pending = true;
      update_delta(n, r1, c1, r2, c2);
      return;
    }
    int rmid = (r1 + r2)/2, cmid = (c1 + c2)/2;
    update(n->child[0], r1, c1, rmid, cmid);
    update(n->child[1], rmid + 1, c1, r2, cmid);
    update(n->child[2], r1, cmid + 1, rmid, c2);
    update(n->child[3], rmid + 1, cmid + 1, r2, c2);
    bool found = false;
    for (int i = 0; i < 4; i++) {
      n->value = found ? join_values(n->value, n->child[i]->value)
                       : n->child[i]->value;
      found = true;
    }
  }

  void query(node_t *n, int r1, int c1, int r2, int c2) {
    if (tgt_r2 < r1 || tgt_r1 > r2 || tgt_c2 < c1 || tgt_c1 > c2) {
      return;
    }
    if (n == NULL) {
      int rlen = std::min(r2, tgt_r2) - std::max(r1, tgt_r1) + 1;
      int clen = std::min(c2, tgt_c2) - std::max(c1, tgt_c1) + 1;
      T v = join_region(init, rlen*clen);
      res = found ? join_values(res, v) : v;
      found = true;
      return;
    }
    update_delta(n, r1, c1, r2, c2);
    if (tgt_r1 <= r1 && r2 <= tgt_r2 && tgt_c1 <= c1 && c2 <= tgt_c2) {
      res = found ? join_values(res, n->value) : n->value;
      found = true;
      return;
    }
    int rmid = (r1 + r2)/2, cmid = (c1 + c2)/2;
    query(n->child[0], r1, c1, rmid, cmid);
    query(n->child[1], rmid + 1, c1, r2, cmid);
    query(n->child[2], r1, cmid + 1, rmid, c2);
    query(n->child[3], rmid + 1, cmid + 1, r2, c2);
  }

  static void clean_up(node_t *n) {
    if (n == NULL) {
      return;
    }
    for (int i = 0; i < 4; i++) {
      clean_up(n->child[i]);
    }
    delete n;
  }

public:
  quadtree(const T &v = T()) {
    root = NULL;
    init = v;
  }

  ~quadtree() {
    clean_up(root);
  }

  void update(int r1, int c1, int r2, int c2, const T &d) {
    tgt_r1 = r1;
    tgt_c1 = c1;
    tgt_r2 = r2;
    tgt_c2 = c2;
    delta = d;
    update(root, 0, 0, MAXR, MAXC);
  }

  void update(int r, int c, const T &d) {
    update(r, c, r, c, d);
  }

  T query(int r1, int c1, int r2, int c2) {
    tgt_r1 = r1;
    tgt_c1 = c1;
    tgt_r2 = r2;
    tgt_c2 = c2;
    found = false;
    query(root, 0, 0, MAXR, MAXC);
    return found ? res : join_region(init, (r2 - r1 + 1)*(c2 - c1 + 1));
  }

  T at(int r, int c) {
    return query(r, c, r, c);
  }
};

/*** Example Usage and Output:

Values:
7 6 9
0 4 9
9 9 9

***/

#include <cassert>
#include <iostream>
using namespace std;

int main() {
  quadtree<int> t(0);
  t.update(0, 0, 7);
  t.update(0, 1, 6);
  t.update(1, 1, 4);
  t.update(2, 1, 1);
  t.update(2, 2, 4);
  t.update(0, 2, 3, 2, 9);
  t.update(2, 0, 2, 2, 9);
  cout << "Values:" << endl;
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      cout << t.at(i, j) << " ";
    }
    cout << endl;
  }
  assert(t.query(0, 0, 0, 1) == 6);
  assert(t.query(0, 0, 1, 0) == 0);
  assert(t.query(1, 1, 2, 2) == 4);
  assert(t.query(0, 0, 1000000000, 1000000000) == 0);
  t.update(0, 500000000, 0, 500000000, -100);
  assert(t.query(0, 0, 1000000000, 1000000000) == -100);
  return 0;
}