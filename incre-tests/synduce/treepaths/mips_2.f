Config ComposeNum = 4;

Inductive BTree = empty Unit | node {Int, BTree, BTree};
Inductive Zipper = top Unit | left {Int, BTree, Zipper} | right {Int, BTree, Zipper};

max = \a: Int. \b: Int. if (< a b) then b else a;

spec = (fix (
  \f: {Int, Int} -> BTree -> {Int, Int}. \s: {Int, Int}. \t: BTree.
  match t with
    empty _ -> s
  | node {a, l, r} ->
    let result = f s l in
    f {+ result.1 a, max result.2 (+ result.1 a)} r
  end
)) {0,0};

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

main = \z: Zipper. spec (repr z);