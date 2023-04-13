Inductive Tree = leaf Int | node {Tree, Tree};
Inductive List = cons {Int, List} | nil Unit;

sum = fix (
  \f: List->Int. \l: List.
  match l with 
    cons {h, t} -> + h (f t)
  | nil _ -> 0
  end
);

fold_list = lambda f: Int->List->List. lambda x: List. lambda w0: List.  
  fix (lambda g: List->List. lambda x: List.
    match x with 
      cons {h, t} -> f h (g t)
    | _ -> w0
    end) x;

concat = lambda x: List. lambda y: List.
  fold_list (lambda h: Int. lambda t: List. cons {h, t}) x y;

t2list = fix (
  \f: Tree->Compress List. \t: Tree.
  match t with
    leaf v -> cons {v, nil unit}
  | node {t1, t2} -> concat (f t1) (f t2)
  end
);

trans = \f: List->Int. \t: Tree. f (t2list t);

run = trans sum;