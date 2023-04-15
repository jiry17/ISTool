Inductive BTree = empty Unit | node {Int, BTree, BTree};
Inductive Zipper = top Unit | left {Int, BTree, Zipper} | right {Int, BTree, Zipper};

sum = fix (
  \f: BTree -> Int. \t: BTree.
  match t with
    empty _ -> 0
  | node {a, l, r} -> + a (+ (f l) (f r))
  end
);

tree_repr = fix (
  \f: BTree -> Compress BTree. \t: BTree.
  match t with
    empty _ -> empty unit
  | node {a, l, r} -> node {a, f l, f r}
  end
);

repr = fix (
  \f: Zipper -> Compress BTree. \z: Zipper.
  match z with
    top _ -> empty unit
  | left {w, tree, zz} ->
      node {w, tree_repr tree, f zz}
  | right {w, tree, zz} ->
      node {w, f zz, tree_repr tree}
  end
);

main = \z: Zipper. sum (repr z);