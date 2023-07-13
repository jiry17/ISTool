Item = {Int, Int};
Inductive ItemList = cons {Item, ItemList} | nil Unit;
Inductive List = icons {Int, List} | inil Unit;
Plan = Compress ItemList;
Inductive PlanList = consPlan {Plan, PlanList} | nilPlan Unit;

max = \x: Int. \y: Int. if (< x y) then y else x;

/* A trivial program for 01knapsack */

sumw = fix (
  \f: Plan -> Int. \xs: Plan.
  match xs with
    nil _ -> 0
  | cons {h, t} -> + h.1 (f t)
  end
);

sumv = fix (
  \f: Plan -> Int. \xs: Plan.
  match xs with
    nil _ -> 0
  | cons {h, t} -> + h.2 (f t)
  end
);

mapP = \g: Plan -> Plan. fix (
  \f: PlanList -> PlanList. \xs: PlanList.
  match xs with
    nilPlan _ -> nilPlan unit
  | consPlan {h, t} -> consPlan {g h, f t}
  end
);

mapI = \g: Plan -> Int. fix (
  \f: PlanList -> List. \xs: PlanList.
  match xs with
    nilPlan _ -> inil unit
  | consPlan {h, t} -> icons {g h, f t}
  end
);

filter = \g: Plan -> Bool. fix (
  \f: PlanList -> PlanList. \xs: PlanList.
  match xs with
    nilPlan _ -> nilPlan unit
  | consPlan {h, t} ->
    if g h then consPlan {h, f t} else f t
  end
);

concat = fix (
  \f: PlanList -> PlanList -> PlanList. \xs: PlanList. \ys: PlanList.
  match xs with
    nilPlan _ -> ys
  | consPlan {h, t} -> consPlan {h, f t ys}
  end
);

maximum = fix (
  \f: List -> Int. \xs: List.
  match xs with
    inil _ -> 0
  | icons {h, t} -> max h (f t)
  end
);

extend = \x: Item. \ys: Plan. cons {x, ys};
valid = \c: Int. \xs: Plan. <= (sumw xs) c;

gen = fix (
  \f: ItemList -> PlanList. \xs: ItemList.
  match xs with
    nil _ -> consPlan {nil unit, nilPlan unit}
  | cons {h, t} ->
    let res = f t in
    concat res (mapP (extend h) res)
  end
);

knapsack = \cap: Int. \items: ItemList.
  let sols = gen items in
  let valid_sols = filter (valid cap) sols in
  maximum (mapI sumv valid_sols);

/*items = cons {{2, 2}, cons {{2, 3}, nil unit}};
gen items;
1 1;*/