data N = S N | Z;

fun fib = function
| Z -> 1
| S Z -> 1
| S (S x) -> fib x + fib (S x);

run :: N -> Reframe (N);
fun run = function
| Z -> Z
| S x -> S (run x);

fun main x = fib (run x);