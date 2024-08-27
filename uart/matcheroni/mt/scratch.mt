[foo]
// Do something special here
bar : u32 = 2;
baz : u32;
blep;

[bar]
zap = (2 * foo) ^ 8 + (1 + 8);
foo = bar(10, 11, 12);
if (blah) {
  blee = 2;
  florp(10);
  return 7877;
} else {
  x = [1, 2, 3];
  return /* not important */ 7777;
  match (foo) {
    case (bar) {
    }
    case (baz) {
    }
  }
}
