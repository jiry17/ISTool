import "compress";
import "standard";

/* A language with only + and neg and its interpreter */
data LangA = One | Add LangA * LangA | Neg LangA;

fun evalA = function
| One -> 1
| Add {l, r} -> evalA l + evalA r
| Neg e -> -(evalA e);

/* Another language built on LangA and the translator*/
data LangB = One' | Add' LangB * LangB | Sub LangB * LangB;

transBtoA :: LangB -> Reframe (LangA);
fun transBtoA = function
| One' -> One
| Add' {l, r} -> Add {transBtoA l, transBtoA r}
| Sub {l, r} -> Add {transBtoA l, Neg (transBtoA r)};

/* Synthesize a direct interpreter for LangB */

fun evalB e = evalA (transBtoA e);