Inductive Tree = leaf Int | node {Int, Tree, Tree};

@Input x: Int;

spec = \x: Int. (fix (
  \f: Int -> Tree -> Bool. \y: Int. \t: Tree.
  match t with
    leaf a -> > y a
  | node {a, l, r} ->
    let r1 = (f a l) in
    let r2 = (f a r) in
    and r1 (and r2 (> y a))
  end
)) x;

repr = fix (
  \f: Tree -> Compress Tree. \t: Tree.
  match t with
    leaf a -> leaf a
  | node {a, l, r} -> node {a, f l, f r}
  end
);

main = \t: Tree. spec x (repr t);
