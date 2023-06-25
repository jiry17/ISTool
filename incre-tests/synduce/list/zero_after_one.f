Inductive BList = nil Unit | cons {Bool, BList};
Inductive CList = emp Unit | single Bool | concat {CList, CList};

cat_list = fix (
  \f: BList -> BList -> BList. \xs: BList. \ys: BList.
  match xs with
    nil _ -> ys
  | cons {hd, tl} -> cons {hd, f tl ys}
  end
);

repr = fix (
  \f: CList -> Compress BList. \xs: CList.
  match xs with
    emp _ -> nil unit
  | single a -> cons {a, nil unit}
  | concat {a, b} -> cat_list (f a) (f b)
  end
);

spec = \xs: BList. (fix (
  \f: BList -> {Bool, Bool, Bool}. \xs: BList.
  match xs with
    nil _ -> {false, false, false}
  | cons {hd, tl} ->
    let result = f tl in
    let new_seen1 = or result.1 hd in
    let new_res = or result.2 (and result.1 (not hd)) in
    let new_aux = or result.3 (not hd) in
    {new_seen1, new_res, new_aux}
  end
) xs).2;

main = \xs: CList. spec (repr xs);
