#!/usr/bin/python3

"""
from matcher import *
from c_lexer import *

#---------------------------------------------------------------------------------------------------

parse_statement = Capture(Until(Atom('\n')))

parse_section = Seq(
  Opt(lex_space),
  Atom('['),
  Opt(lex_space),
  Field("name", Capture(lex_ident)),
  Opt(lex_space),
  Atom(']'),
  Field(
    "contents",
    List(Any(parse_statement))
  ),
)

parse_decl = Seq(
  Opt(lex_space),
  Dict(Seq(
    Field("type", Capture(lex_ident)),
    Opt(lex_space),
    Field("name", Capture(lex_ident)),
    Opt(lex_space),
    Atom('='),
    Opt(lex_space),
    Field("value", Capture(lex_int)),
    Opt(lex_space),
    Atom(';')
  )),
  Opt(lex_space),
)

#---------------------------------------------------------------------------------------------------
"""

if __name__ == "__main__":

  ctx = []
  tail = None

  #tail = pattern("int x = 2; end", ctx)

  print(f"Ctx:  {ctx}")
  print(f"Tail: {tail}")
  pass
