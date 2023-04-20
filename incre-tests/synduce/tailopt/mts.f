Inductive List = nil Unit | cons {Int, List};

max = \a: Int. \b: Int. if < a b then b else a;

spec = fix (
  \f: List -> {Int, Int}. \xs: List.
  match xs with
    nil _ -> {0,0}
  | cons {h, t} ->
    let r = f t in
    {+ h r.1, max r.2 (+ h r.1)}
  end
);

snoc = fix (
  \f: List -> Int -> List. \xs: List. \w: Int.
  match xs with
    nil _ -> cons {w, nil unit}
  | cons {h, t} -> cons {h, f t w}
  end
);

repr = (fix (
  \f: Compress List -> List -> Compress List.
  \pre: Compress List. \xs: List.
  match xs with
    nil _ -> pre
  | cons {h, t} -> f (snoc pre h) t
  end
)) (nil unit);

main = \xs: List. spec (repr xs);