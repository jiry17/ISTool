Inductive Nat = zero Unit | succ Nat;

@Combine plus = fix(
  \f: Nat -> Nat -> Nat.
  \x: Nat. \y: Nat.
  match y with
    succ y0 -> succ (f x y0)
  | zero _ -> x
  end);

@Combine mult = fix(
  \f: Nat -> Nat -> Nat.
  \x: Nat.
  \y: Nat.
  match y with
    succ y0 -> plus x (f x y0)
  | zero _ -> zero unit
  end);

@AsCompress compress = \z: Nat. \a: Nat. \b: Nat. mult z (mult a b);

@Input aa: Nat;
@Input bb: Nat;
repr_nat = fix (
  \f: Nat -> Compress Nat.
  \n: Nat.
  match n with
    zero _ -> zero unit
  | succ n0 -> succ (f n0)
end);

lhs = \z: Nat. (mult (mult aa bb) (repr_nat z));
