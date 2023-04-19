Inductive List = elt Int | cons {Int, List};
Inductive NList = line List | ncons {List, NList};
Inductive CNList = sglt List | cat {CNList, CNList};

max = \a: Int. \b: Int. if < a b then b else a;
min = \a: Int. \b: Int. if > a b then b else a;

cton = fix (
  \f: CNList -> NList.
  let dec = fix (
    \g: CNList -> CNList -> NList. \l: CNList. \c: CNList.
    match c with
      sglt x -> ncons {x, f l}
    | cat {x, y} -> g (cat {y, l}) x
    end
  ) in \c: CNList.
  match c with
    sglt x -> line x
  | cat {x, y} -> dec y x
  end
);

lmax = fix (
  \f: List -> Int. \xs: List.
  match xs with
    elt a -> a
  | cons {hd, tl} -> max hd (f tl)
  end
);

aux = fix (
  \f: Int -> NList -> Bool. \prev: Int. \xs: NList.
  match xs with
    line a -> >= prev (lmax a)
  | ncons {hd, tl} -> and (>= prev (lmax hd)) (f (lmax hd) tl)
  end
);

is_sorted = fix (
  \f: NList -> Bool. \xs: NList.
  match xs with
    line a -> true
  | ncons {hd, tl} -> aux (lmax hd) tl
  end
);

interval = fix (
  \f: List -> {Int, Int}. \xs: List.
  match xs with
    elt a -> {a,a}
  | cons {hd, tl} ->
    let result = f tl in
    {min hd result.1, max hd result.2}
  end
);

spec = fix (
  \f: NList -> {Int, Int, Bool}. \xs: NList.
  match xs with
    line a -> 
    let result = interval a in
    {result.1, result.2, true}
  | ncons {hd, tl} ->
    let r1 = f tl in
    let r2 = interval hd in
    {min r1.1 r2.1, max r1.2 r2.2, and r1.3 (and (<= r1.1 r2.1) (<= r1.2 r2.2))}
  end
);

target = fix (
  \f: CNList -> Compress CNList.
  let tsum = fix (
    \g: List -> Compress List. \xs: List.
    match xs with
      elt x -> elt x
    | cons {h, t} -> cons {h, g t}
    end
  ) in \c: CNList.
  match c with
    sglt x -> sglt (tsum x)
  | cat {l, r} -> cat {f l, f r}
  end
);

main = \c: CNList. spec (cton (target c));