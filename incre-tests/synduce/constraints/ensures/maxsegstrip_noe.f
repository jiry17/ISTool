Config SampleSize = 20;

Inductive List = elt Int | cons {Int, List};
Inductive NList = line List | ncons {List, NList};
Inductive CNList = sglt List | cat {CNList, CNList};

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

sum = fix (
  \f: List -> Int. \xs: List.
  match xs with
    elt x -> x
  | cons {h, t} -> + h (f t)
  end
);

max = \a: Int. \b: Int. if < a b then b else a;

spec = fix (
  \f: NList -> {Int, Int, Int, Int}. \xs: NList.
  match xs with
    line a ->
      let s = sum a in
        let ms = max s 0 in
          {ms, ms, ms, s}
  | ncons {h, t} ->
    let hsum = sum h in
      let res = f t in
        {
        max (+ res.1 hsum) 0,
        max res.2 (+ res.1 hsum),
        max (+ res.4 hsum) res.3,
        + res.4 hsum
        }
  end
);

target = fix (
  \f: CNList -> Compress CNList. \c: CNList.
  match c with
    sglt x ->
    let info = sum x in /*This invocation is provided in Synduce's template*/
      c
  | cat {l, r} -> cat {f l, f r}
  end
);

main = \c: CNList. spec (cton (target c));