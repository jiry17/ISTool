Config NonLinear = true;

Inductive Nat = z Unit | s Nat;
Inductive List = nil Unit | cons {Int, List};
Inductive CList = cnil Unit | ccons {Nat, Int, CList};

repeat = \w: Int. \xs: List. fix (
  \f: Nat -> List. \n: Nat.
  match n with
    z _ -> xs
  | s m -> cons {w, f m}
  end
);

value = fix (
  \f: Nat -> Int. \n: Nat.
  match n with
    z _ -> 0
  | s m -> + 1 (f m)
  end
);

spec = fix (
  \f: List -> Int. \xs: List.
  match xs with
    nil _ -> 0
  | cons {h, t} -> + h (f t)
  end
);

nat_repr = fix (
  \f: Nat -> Compress Nat. \n: Nat.
  match n with
    z _ -> z unit
  | s m -> s (f m)
  end
);

repr = fix (
  \f: CList -> Compress List. \xs: CList.
  match xs with
    cnil _ -> nil unit
  | ccons {n, h, t} ->
      repeat h (f t) (nat_repr n)
  end
);

main = \xs: CList. spec (repr xs);