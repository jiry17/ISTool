import "compress";
import "list";

hd :: (List Int) -> Int;
fun hd = function
| (Cons {h, t}) -> h
| Nil -> 0;

tl :: (List Int) -> (List Int);
fun tl = function
| (Cons {h, t}) -> t
| Nil -> Nil;

ifib :: Int -> Reframe (List Int);
fun ifib n = 
  if n <= 2 then Cons {1, Cons {1, Nil}} else 
    let xs = ifib (n - 1) in
    let x = hd xs + hd (tl xs) in Cons {x, xs};

rev:: (List Int) -> (List Int);
fun rev = function
| Nil -> Nil
| Cons {h, t} -> cat (rev t) (Cons {h, Nil});

fun main n = sum (rev (ifib n));