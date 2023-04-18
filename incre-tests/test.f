Inductive Mint = cint Int/* | symint Unit*/;
Inductive List = cons {Mint, List} | nil Unit /*| symlist Unit*/;
/*Inductive Tree = leaf Mint | bin {MInt, Tree, Tree} | symTree Unit;*/

max = \x: Int. \y: Int. if (> x y) then x else y;
extract = \x: Mint. match x with cint v -> v end;

repr = fix (\f: List -> Compress List. \x: List.
	match x with
	  nil _ -> x
	| cons {h, t} -> cons {h, f t}
	end
);

append =
fix (lambda f: List -> Mint -> List. lambda l: List. lambda x: Mint.
match l with
  cons {h, t} -> cons {h, (f t x)}
| nil _ -> cons {x, nil unit}
end
);


rev = fix (lambda f: List -> List. lambda l: List.
match l with
  cons {h, t} -> append (f t) h
| nil _ -> nil unit
end
);


sum = fix (lambda f: List -> Int.
lambda l: List.
match l with
  cons {cint h, t} -> + h (f t)
| nil _ -> 0
end
);

msufsum = fix (lambda f: List -> Int. lambda l: List.
match l with
 cons {cint h, t} -> max (sum l) (f t)
| nil _ -> 0
end);

mps = fix (lambda f: List -> Int. lambda l: List.
match l with
  cons {cint h, t} -> max 0 (+ h (f t))
| nil _ -> 0
end);

@Start main = \x: List. \v: Mint.
	let cx = repr x in
		msufsum (append cx v);

x = cons {cint 3, cons {cint 2, nil unit}};
main x (cint 1);

/*
msufsum_comb = lambda l: List. lambda x: Mint. max 0 (+ x (msufsum l));

msufsum (rev (cons {symint unit, symlist unit}));
mps (rev (cons {symint unit, symlist unit}));
sum (incre (cons {symint unit, symlist unit}));
+ (sum (cons {symint unit, symlist unit})) (length (cons {symint unit, symlist unit}));
rev (rev (cons {symint unit, symlist unit}));


rev (rev (cons {symint unit, symlist unit}));

cons {rev (rev (symlist unit)), symint unit};

max 0 (+ (symint unit) (msufsum (rev (symlist unit))));
*/