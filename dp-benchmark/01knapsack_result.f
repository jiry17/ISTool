fun min x y = if (x < y) then x
else y;

fun max x y = if (x > y) then x
else y;

inf = 100;

fun fst _inp = match _inp with
    {w, _} -> w
;

fun snd _inp = match _inp with
    {_, w} -> w
;

data List a = Cons a * List a | Nil Unit;

fun map f _inp = match _inp with
    Cons {h, t} -> Cons {f h, map f t}
  | Nil _ -> Nil unit
;

fun fold f e _inp = match _inp with
    Cons {h, t} -> f h (fold f e t)
  | Nil _ -> e
;

fun filter f _inp = match _inp with
    Cons {h, t} -> if (f h) then Cons {h, filter f t}
    else filter f t
  | Nil _ -> Nil unit
;

sum = fold (fun h res -> h + res) 0;

maximum = fold (fun h res -> max h res) (-inf);

minimum = fold (fun h res -> min h res) inf;

andall = fold (fun h res -> h and res) false;

orall = fold (fun h res -> h or res) true;

fun head e _inp = match _inp with
    Cons {h, t} -> h
;

fun tails _inp = match _inp with
    (Cons {h, t})@xs -> Cons {xs, tails t}
  | Nil _ -> Cons {Nil unit, Nil unit}
;

fun cat xs ys = match xs with
    Cons {h, t} -> Cons {h, cat t ys}
  | Nil _ -> ys
;

fun concat _inp = match _inp with
    Cons {h, t} -> cat h (concat t)
  | Nil _ -> Nil unit
;

fun prod xs ys = match xs with
    Cons {h, t} -> cat (map (fun w -> Cons {h, w}) ys) (prod t ys)
  | Nil _ -> Nil unit
;

fun prod_pair _inp = match _inp with
    {Cons {h, t}, ys} -> cat (map (fun w -> {h, w}) ys) (prod_pair {t, ys})
  | {Nil _, _} -> Nil unit
;

fun product _inp = match _inp with
    Cons {h, t} -> prod h (product t)
  | Nil _ -> Cons {Nil unit, Nil unit}
;

sempty = Nil unit;

fun sstep f sols = concat (map f (product sols));

fun sstep1 f sol = concat (map f sol);

fun sstep2 f sols = concat (map f (prod_pair sols));

fun smerge xs ys = cat xs ys;

fun sinsert xs s = Cons {xs, s};

fun sargmax obj xs = maximum (map obj xs);

fun sfilter f xs = filter f xs;

getw = fold (fun item res -> fst item + res) 0;

getv = fold (fun item res -> snd item + res) 0;

fun step lim item sol = if (sol.1/2 + fst item <= lim) then Cons {let c0 = item.1/2 in 
  let c1 = item.2/2 in 
    {sol.1/2 + c0, sol.2/2 + c1}, Cons {sol, Nil unit}}
else Cons {sol, Nil unit};

fun gen lim _inp = match _inp with
    Nil _ -> sinsert {0, 0} sempty
  | Cons {h, t} -> sstep1 (step lim h) (gen lim t)
;

fun main lim xs = let sols = (gen lim xs) in 
  sargmax (fun sol -> sol.2/2) sols;
