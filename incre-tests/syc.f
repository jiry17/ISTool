Config SampleIntMin = 0;
Config SampleIntMax = 1;
Config SampleSize = 6; /*Reduce the time of random testing*/

Inductive List = cons {Int, List} | single Int;
Input = {List, List};

head = \xs: List.
  match xs with
    cons {h, t} -> h
  | single h -> h
  end;

length = fix (
  \f: List -> Int. \xs: List.
  match xs with
    single _ -> 1
  | cons {h, t} -> + 1 (f t)
  end
);

tail = \xs: List.
  match xs with
    cons {h, t} -> t
  end;

list_eq = fix (
  \f: List -> List -> Bool. \xs: List. \ys: List.
  match {xs, ys} with
    {cons {xh, xt}, cons {yh, yt}} ->
      if == xh yh then f xt yt else false
  | {single x, single y} -> == x y
  | _ -> false
  end
);

list_lt = fix (
  \f: List -> List -> Bool. \xs: List. \ys: List.
  match {xs, ys} with
    {cons {xh, xt}, cons {yh, yt}} ->
      if == xh yh then f xt yt
      else if < xh yh then true
      else false
  | {single x, single y} -> < x y
  | {single x, cons {y, _}} -> <= x y
  | {cons {x, _}, single y} -> < x y
  end
);

search = fix(
  \f: Input -> Reframe {Int, List, List}. \ps: Input.
  match ps with
    {single x, single y} -> {1, cons {x, single y}, single y}
  | {cons {xh, xt}, cons {yh, yt}} ->
      let res = f {xt, yt} in 
      if list_lt (cons {yh, res.3}) res.2 then {1, cons {xh, (cons {yh, res.3})}, (cons {yh, res.3})}
      else if list_eq (cons {yh, res.3}) res.2 then {+ 1 res.1, cons {xh, res.2}, (cons {yh, res.3})}
      else {res.1, cons {xh, res.2}, (cons {yh, res.3})}
  end 
);

length = fix (
  \f: List -> Int. \xs: List.
  match xs with
    single _ -> 1
  | cons {h, t} -> + 1 (f t)
  end
);

main = \inp: Input.
  if == (length inp.1) (length inp.2) then (search inp).1
  else 0;