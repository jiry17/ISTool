Inductive Tree = telt Int | tnode {Int, Tree, Tree};
Inductive PTree = pelt Int | pnode {Int, List}
    with List = elt PTree | cons {PTree, List};

max = \a: Int. \b: Int. if < a b then b else a;

repr = fix (
  \f: Tree -> PTree. \x: Tree.
  match x with
    telt a -> pelt a
  | tnode {a, l, r} ->
    let repr_list = fix (
        \g: Tree -> List. \y: Tree.
        match y with
          telt a -> let z = pelt a in elt z
        | tnode {a, l, r} -> cons {pelt a, cons {f l, g r}}
        end
    ) in
    pnode {a, cons {f l, repr_list r}}
  end
);

spec = fix (
  \f: PTree -> {Int, Int}. \x: PTree.
  match x with
    pelt a -> {a, a}
  | pnode {a, l} -> 
    let maxh_aux = fix (
        \g: List -> Int. \y: List.
        match y with
          elt a -> (f a).1
        | cons {hd, tl} -> + (f hd).1 (g tl)
        end
    ) in
      let res = maxh_aux l in
        {max a res, + a res}
  end
);

target = fix (
  \f: Tree -> Compress Tree. \t: Tree.
  match t with
    telt w -> t
  | tnode {w, l, r} -> tnode {w, f l, f r}
  end
);

main = \t: Tree. spec (repr (target t));