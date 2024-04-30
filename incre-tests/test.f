Config SampleIntMin = 0;
Config ComposeNum = 5;
Config TermNum = 5;
import "compress";
import "list";

/* (Start Time, Smallest Delay After Its Departure) */
data Runway = Empty | Last Runway * (Int * Int);
RunwayIndex = Int;
first_runway = 0;
second_runway = 1;

Plan = Reframe (List (RunwayIndex * (Int * Int)));

/* Generating all possible solutions */
add_plan :: (RunwayIndex * (Int * Int)) -> Plan -> Plan;
fun add_plan decision xs = Cons {decision, xs};

gen :: (Runway * Runway) -> List (Plan);
fun gen = function
| {Empty, Empty} -> Cons {Nil, Nil}
| {Last {xs, x} , Empty} ->
  map (add_plan {first_runway, x}) (gen {xs, Empty})
| {Empty, Last {ys, y}} ->
  map (add_plan {second_runway, y}) (gen {Empty, ys})
| {(Last {xs', x})@xs, (Last {ys', y})@ys} ->
  let x_sols = map (add_plan {first_runway, x}) (gen {xs', ys}) in
  let y_sols = map (add_plan {second_runway, y}) (gen {xs, ys'}) in
  cat x_sols y_sols;

/* Calc objective function */
fun earliest_dep_time = function
| Nil -> {0, 0}
| Cons {{index, {start, delay}}, t} ->
  let tres = earliest_dep_time t in
  if index == first_runway then
    let dep_time = max (fst tres) start in
    {dep_time + delay, max (snd tres) dep_time}
  else
    let dep_time = max (snd tres) start in
    {max (fst tres) dep_time, dep_time + delay};

fun obj' = function
| Nil -> 0
| Cons {{index, {start, _}}, t} ->
  let dep_time = earliest_dep_time t in
  let tres = obj' t in
  if index == first_runway then
    tres + (fst dep_time) - start
  else
    tres + (snd dep_time) - start;

obj :: Plan -> Int;
fun obj items = obj' items;

fun main runways = minimum (map obj (gen runways));