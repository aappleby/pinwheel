#!/usr/bin/python3

from matcheroni import *
from mt_lexer import *
import mt_constants

#---------------------------------------------------------------------------------------------------
# Define our atom types for the parser

def LexToAtom(type, span = None):
  if span is None:
    return Atom(type)
  return Atom(Lexeme(type, span))

ATOM_NEWLINE = LexToAtom(LexemeType.LEX_NEWLINE)
ATOM_STRING  = LexToAtom(LexemeType.LEX_STRING)
ATOM_CHAR    = LexToAtom(LexemeType.LEX_CHAR)
ATOM_KEYWORD = LexToAtom(LexemeType.LEX_KEYWORD)
ATOM_IDENT   = LexToAtom(LexemeType.LEX_IDENT)
ATOM_COMMENT = LexToAtom(LexemeType.LEX_COMMENT)
ATOM_FLOAT   = LexToAtom(LexemeType.LEX_FLOAT)
ATOM_INT     = LexToAtom(LexemeType.LEX_INT)
ATOM_PUNC    = LexToAtom(LexemeType.LEX_PUNCT)

ATOM_DASH    = LexToAtom(LexemeType.LEX_PUNCT, "-")
ATOM_COMMA   = LexToAtom(LexemeType.LEX_PUNCT, ",")
ATOM_SEMI    = LexToAtom(LexemeType.LEX_PUNCT, ";")
ATOM_COLON   = LexToAtom(LexemeType.LEX_PUNCT, ":")
ATOM_BANG    = LexToAtom(LexemeType.LEX_PUNCT, "!")
ATOM_QUEST   = LexToAtom(LexemeType.LEX_PUNCT, "?")
ATOM_LPAREN  = LexToAtom(LexemeType.LEX_PUNCT, "(")
ATOM_RPAREN  = LexToAtom(LexemeType.LEX_PUNCT, ")")
ATOM_LBRACE  = LexToAtom(LexemeType.LEX_PUNCT, "[")
ATOM_RBRACE  = LexToAtom(LexemeType.LEX_PUNCT, "]")
ATOM_LBRACK  = LexToAtom(LexemeType.LEX_PUNCT, "{")
ATOM_RBRACK  = LexToAtom(LexemeType.LEX_PUNCT, "}")
ATOM_STAR    = LexToAtom(LexemeType.LEX_PUNCT, "*")
ATOM_FSLASH  = LexToAtom(LexemeType.LEX_PUNCT, "/")
ATOM_AMP     = LexToAtom(LexemeType.LEX_PUNCT, "&")
ATOM_POUND   = LexToAtom(LexemeType.LEX_PUNCT, "#")
ATOM_PERCENT = LexToAtom(LexemeType.LEX_PUNCT, "%")
ATOM_CARET   = LexToAtom(LexemeType.LEX_PUNCT, "^")
ATOM_PLUS    = LexToAtom(LexemeType.LEX_PUNCT, "+")
ATOM_EQ      = LexToAtom(LexemeType.LEX_PUNCT, "=")
ATOM_PIPE    = LexToAtom(LexemeType.LEX_PUNCT, "|")
ATOM_TILDE   = LexToAtom(LexemeType.LEX_PUNCT, "~")
ATOM_LT      = LexToAtom(LexemeType.LEX_PUNCT, "<")
ATOM_GT      = LexToAtom(LexemeType.LEX_PUNCT, ">")
ATOM_DOT     = LexToAtom(LexemeType.LEX_PUNCT, ".")
ATOM_AT      = LexToAtom(LexemeType.LEX_PUNCT, "@")


cap_int   = Capture(ATOM_INT)


KW_MATCH  = Atom(Lexeme(LexemeType.LEX_KEYWORD, "match"))
KW_CASE   = Atom(Lexeme(LexemeType.LEX_KEYWORD, "case"))
KW_RETURN = Atom(Lexeme(LexemeType.LEX_KEYWORD, "return"))
KW_ELSE   = Atom(Lexeme(LexemeType.LEX_KEYWORD, "else"))
KW_IF     = Atom(Lexeme(LexemeType.LEX_KEYWORD, "if"))
KW_SIGNED = Atom(Lexeme(LexemeType.LEX_KEYWORD, "signed"))
KW_UNSIGNED = Atom(Lexeme(LexemeType.LEX_KEYWORD, "unsigned"))

def parse_statement(span, ctx):
  return _parse_statement(span, ctx)

def parse_expression_chain(span, ctx):
  return _parse_expression_chain(span, ctx)

def match_binary_op(span, ctx):
  s = span
  if s[0].text in mt_constants.mt_binops:
    if len(s) >= 2 and (s[0].text + s[1].text) in mt_constants.mt_binops:
      if len(s) >= 3 and (s[0].text + s[1].text + s[2].text) in mt_constants.mt_binops:
        return span[3:]
      return span[2:]
    return span[1:]
  return Fail(span)

#---------------------------------------------------------------------------------------------------

paren_expression = Seq(ATOM_LPAREN, parse_expression_chain, ATOM_RPAREN)

parse_params = List(
  ATOM_LPAREN,
  Cycle(parse_expression_chain, ATOM_COMMA),
  ATOM_RPAREN
)

parse_braces = List(
  Tag("name", Capture(ATOM_IDENT)),
  ATOM_LBRACE,
  Cycle(parse_expression_chain, ATOM_COMMA),
  ATOM_RBRACE
)

parse_ident = Capture(Seq(
  Opt(ATOM_AT),
  ATOM_IDENT,
  Any(Seq(
    ATOM_DOT,
    ATOM_IDENT,
  ))
))

parse_call = Dict(
  Tag("func",   parse_ident),
  Tag("params", parse_params)
)

parse_cast = Dict(
  Tag("type",   Capture(Oneof(KW_SIGNED, KW_UNSIGNED))),
  Tag("params", parse_params)
)

parse_expression_unit = Oneof(
  Tag("cast",   parse_cast),
  Tag("call",   parse_call),
  Tag("parens", paren_expression),
  Tag("array",  parse_braces),
  Tag("int",    Capture(ATOM_INT)),
  Tag("float",  Capture(ATOM_FLOAT)),
  Tag("string", Capture(ATOM_STRING)),
  Tag("ident",  Capture(ATOM_IDENT)),
)

_parse_expression_chain = Seq(
  parse_expression_unit,
  Any(Seq(
    Tag("op ", Capture(match_binary_op)),
    parse_expression_unit,
  ))
)

parse_block = Seq(
  ATOM_LBRACK,
  List(Any(parse_statement)),
  ATOM_RBRACK
)

parse_else = Dict(
  KW_ELSE,
  Tag("statements", parse_block),
)

parse_if = Dict(
  KW_IF,
  Tag("condition",  paren_expression),
  Tag("statements", parse_block),
  Tag("else",       Opt(parse_else))
)

parse_case = List(
  KW_CASE,
  Tag("condition",  paren_expression),
  parse_block,
)

parse_match = List(
  KW_MATCH,
  Tag("condition",  paren_expression),
  ATOM_LBRACK,
  Any(Tag("case", parse_case)),
  ATOM_RBRACK
)

parse_section_header = Seq(
  ATOM_LBRACE,
  Dict(Tag("name", parse_ident)),
  ATOM_RBRACE
)

parse_section = Dict(
  Tag("header", parse_section_header),
  Tag("body", Any(parse_statement))
)

parse_type = Seq(
  parse_ident,
  Opt(Tag("array_suffix",
    Seq(
      ATOM_LBRACE,
      Opt(Tag("exp", parse_expression_chain)),
      ATOM_RBRACE,
    )
  ))
)

parse_decl = Dict(
  Tag("name",  parse_ident),
  OptSeq(
    Tag("op", Oneof(
      Capture(ATOM_COLON),
      Capture(Seq(ATOM_LT, ATOM_COLON)),
      Capture(Seq(ATOM_COLON, ATOM_GT)),
    )),
    Tag("type",  parse_type)
  ),
  OptSeq(
    Tag("op", Capture(ATOM_EQ)),
    Tag("value", parse_expression_chain)),
  ATOM_SEMI
)

parse_return = Seq(
  KW_RETURN,
  Opt(parse_expression_chain)
)

_parse_statement = Oneof(
  Tag("match",  parse_match),
  Tag("if",     parse_if),
  Tag("return", parse_return, ATOM_SEMI),

  Tag("call",   parse_call, ATOM_SEMI),
  Tag("decl",   parse_decl),

  ATOM_SEMI,
  ATOM_NEWLINE,
)

parse_top = Oneof(
  Tag("section", parse_section),
  parse_statement,
  ATOM_NEWLINE,
)

#---------------------------------------------------------------------------------------------------

def parse_lexemes(lexemes):
  span = lexemes
  ctx = []
  while span:
    tail = parse_top(span, ctx)
    #print(tail)
    if isinstance(tail, Fail):
      ctx.append(span[0])
      tail = span[1:]
    span = tail
  return ctx

#---------------------------------------------------------------------------------------------------

"""
parse_statement = Capture(Until(Atom('\n')))

parse_statements = List(Any(parse_statement))

parse_section =
Dict(
  ATOM_LBRACE,
  Field("name", cap_ident),
  ATOM_RBRACE,
  Field("contents", parse_statements),
)

parse_decl =
Dict(
  Field("type", cap_ident),
  Field("name", cap_ident),
  OptSeq(
    ATOM_EQ,
    Field("value", cap_int),
    ATOM_SEMI
  )
)
"""

#---------------------------------------------------------------------------------------------------

import doctest
doctest.testmod()
