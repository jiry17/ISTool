/* global definition */
Inductive IntList=cons {Int, IntList} | nil Unit;
Plan = {IntList};

/* library function */
access = fix (\f: IntList -> Int -> Int.
    \l:IntList. \n: Int.  
    match l with 
        cons {h,t} -> if == n 0 then h 
        else f t (- n 1) 
      | nil _  -> 0
      end
    );
range = fix (\f: Int -> Int -> IntList.
    \a: Int. \b: Int.
    if <= a b then cons {a, f (+ a 1) b} else nil unit
    );
foldSSet = \f: Int -> SSet IntList -> SSet IntList.
     \init: SSet IntList. fix (
      \g: IntList -> SSet IntList. \x: IntList.
        match x with
          cons {i, t} -> f i (g t)
        | nil _ -> init
        end
    );
foldBool = \f: Int -> Bool -> Bool.
     \init: Bool. fix (
      \g: IntList -> Bool. \x: IntList.
        match x with
          cons {i, t} -> f i (g t)
        | nil _ -> init
        end
    );
sumAll = fix ( 
      \f: (Int -> Int)-> IntList -> Int. \s: Int -> Int. \l: IntList.
      match l with
        cons {h, t} -> + (s h) (f s t)
      | nil _ -> 0
      end 
    );
genStep = \c:IntList. \y:{IntList}.
        let sol=y.1 in
        foldSSet (\i:Int. \s:SSet IntList.
            sinsert (cons {i, sol}) s
        ) (sempty IntList) c;
genArray = fix (
      \f: IntList -> IntList -> SSet IntList.
      \c: IntList. \l: IntList.
      match l with
        cons {h, t} -> let sols = f c t in
          sstep (genStep c) {sols}
        | nil _ -> sinsert (nil unit) (sempty IntList)
        end
      );
allHold = \l: IntList. \f: Int->Bool.
        foldBool (\i: Int. \b: Bool. and b (f i)) true l;
makePlan=\plan: {IntList}. sinsert plan (sempty Plan);

/* core function */
gen = \n: Int. 
    let val = genArray (range (- 0 1) 1) (range 1 n) in
    sstep makePlan {val};

constraint1 = \n: Int. \plan: Plan. 
    let val = plan.1 in
    allHold (range 0 (- n 1)) (\i: Int. (or (== (access val i) 1) (== (access val i) (- 0 1))));
constraint2 = \n: Int. \plan: Plan. 
    let val = plan.1 in
    (== (sumAll (\i: Int. (access val i)) (range 0 (- n 1))) 0);
constraint3 = \n: Int. \plan: Plan. 
    let val = plan.1 in
    allHold (range 0 (- n 1)) (\i: Int. (>= (sumAll (\j: Int. (access val j)) (range 0 i)) 0));
sfilter = \f: Plan->Bool. \ss: SSet Plan.
		 sstep (\pl: {Plan}. let plan = pl.1 in
		 if (f plan) then sinsert plan (sempty Plan)
		 else sempty Plan) {ss};
filter = \n: Int. \s0: SSet Plan. 
    let s1= sfilter (constraint1 n) s0 in
    let s2= sfilter (constraint2 n) s1 in
    let s3= sfilter (constraint3 n) s2 in
    s3;

object = \n: Int. \plan: Plan. 
    let val = plan.1 in
    error;
main = \n: Int. 
    let sols = gen n in
    let filtered_sols = filter n sols in
    sargmax 0 (object n) filtered_sols;

/* test */