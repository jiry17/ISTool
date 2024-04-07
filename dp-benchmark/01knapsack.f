Config SampleSize = 5;

import "dp";
import "compress";

getw = fold (fun item res -> fst item + res) 0;
getv = fold (fun item res -> snd item + res) 0;

fun step lim item sol = 
  if rewrite (getw (unlabel sol)) + fst item <= lim then
    Cons {rewrite (label (Cons {item, unlabel sol})), Cons {sol, Nil}}
  else Cons {sol, Nil};

fun gen lim = function 
  Nil -> sinsert (rewrite (label Nil)) sempty 
| Cons {h, t} -> sstep1 (step lim h) (gen lim t);

fun main lim xs =
  let sols = gen lim xs in 
  sargmax (fun sol -> rewrite (getv (unlabel sol))) sols;

/* int sample_size = 7; */
/* int dp_bool_height_limit = 3; */
/* int single_example_num = 8; */

/* result for 01knapsack */
/* &&(<=(Param1.0,Param0.0),<=(Param0.1,Param1.1)) */
/* &&(<(Param1.0,Param0.0),<(Param0.1,Param1.1)) */