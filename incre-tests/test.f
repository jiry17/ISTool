import "compress";

data List a = Cons a * (List a) | Nil;
fun fold f e = function
  Cons {h, t} -> f h (fold f e t)
| Nil -> e;

sum :: List Int -> Int;
sum = fold (fun h res -> h + res) 0;

getw :: Reframe (List Int) -> Int;
fun getw items = sum items;
