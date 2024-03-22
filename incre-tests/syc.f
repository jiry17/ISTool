Config SampleSize = 4;

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
  | succ n0 ->
    let res = f n0 in
    let n = unit in
    let n0 = unit in
    succ res
end);

lhs = \z: Nat. compress (repr_nat z) aa bb;
