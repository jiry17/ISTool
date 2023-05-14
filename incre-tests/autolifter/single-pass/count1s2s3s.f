Config ExtraGrammar = "AutoLifter";
Config SampleIntMin = 0;
Config SampleIntMax = 5;

Inductive List = cons {Int, List} | nil Unit;

single_pass = \v: List -> Int.
  let run = fix (
    \f: List -> Compress List. \xs: List.
    match xs with
      nil _ -> xs
    | cons {h, t} -> cons {h, f t}
    end
  ) in \xs: List.
  v (run xs);

inf = 100;

/*User provided programs*/

count1s2s3s = fix (
  \f: Bool -> Bool -> List -> Int. \s1: Bool. \s2: Bool. \xs: List.
  match xs with
    nil _ -> 0
  | cons {h, t} ->
    let upd = if and (== h 3) (or s1 s2) then 1 else 0 in
    let s2 = and (== h 2) (or s1 s2) in
    let s1 = == h 1 in
    + upd (f s1 s2 t)
  end
) false false;

/*test2s1 = fix (
  \f: List -> Bool. \xs: List.
  match xs with
    nil _ -> false
  | cons {h, t} ->
    if == h 2 then f t
    else if == h 1 then true
    else false
  end
);

test2s3 = fix (
  \f: List -> Bool. \xs: List.
  match xs with
    nil _ -> false
  | cons {h, t} ->
    if == h 2 then f t
    else if == h 3 then true
    else false
  end
);*/

main = single_pass count1s2s3s;