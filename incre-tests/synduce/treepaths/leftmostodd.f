Inductive BTree = empty Unit | node {Int, BTree, BTree};
Inductive Zipper = top Unit | left {Int, BTree, Zipper} | right {Int, BTree, Zipper};

max = \a: Int. \b: Int. if (< a b) then b else a;

mod2 = \x: Int. - x (* (/ x 2) 2);

spec = fix (
  \f: BTree -> {Bool, Int}. \t: BTree.
  match t with
    empty _ -> {false, 1}
  | node {a, l, r} ->
    let result = f l in
    if result.1 then result else
        if (== 1 (mod2 a)) then {true, a} else (f r)
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
    let tw = spec tree in
      node {w, f zz, tree_repr tree}
  end
);

main = \z: Zipper. spec (repr z);