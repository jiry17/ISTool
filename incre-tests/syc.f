Inductive Nat = zero Unit | succ Nat;

nat_to_int = fix (lambda f: Nat -> Int.
lambda x: Nat.
match x with
  zero _ -> 0
| succ y -> + 1 (f y)
end);

double = fix (lambda f: Nat -> Nat.
lambda x: Nat.
match x with
  zero _ -> zero unit
|  succ y -> succ (succ (f y))
end);


lhs = \x: Nat. nat_to_int (double x);
rhs = \x: Nat. + (nat_to_int x) (nat_to_int x);
div2 = \x: Int. / x 2;

func0 = fix (lambda f: Nat -> Nat.
lambda x: Nat.
match x with
  zero _ -> zero unit
| succ y -> succ (succ (f y))
end
);

func1 = fix (lambda f: Nat -> Compress Nat.
lambda x: Nat.
match x with
  zero _ -> let var2 = 0
    in
    zero unit
    | succ y -> succ (succ (f y))
    end
);

func2 = lambda var1: Nat. (nat_to_int (func1 var1));

