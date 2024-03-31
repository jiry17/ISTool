import "compress";
import "standard";
import "list";

tails' :: (List Int) -> Reframe (List (List Int));
fun tails' = function
  | (Cons {h, t})@xs -> Cons {xs, tails' t}
  | Nil -> Cons {Nil, Nil};

fun mts xs = maximum (map sum (tails' xs));