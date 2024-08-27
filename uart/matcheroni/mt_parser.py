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

cap_ident = Capture(ATOM_IDENT)
cap_int   = Capture(ATOM_INT)


KW_MATCH  = Atom(Lexeme(LexemeType.LEX_KEYWORD, "match"))
KW_CASE   = Atom(Lexeme(LexemeType.LEX_KEYWORD, "case"))
KW_RETURN = Atom(Lexeme(LexemeType.LEX_KEYWORD, "return"))
KW_ELSE   = Atom(Lexeme(LexemeType.LEX_KEYWORD, "else"))
KW_IF     = Atom(Lexeme(LexemeType.LEX_KEYWORD, "if"))

def parse_statement(span, ctx):
  return _parse_statement(span, ctx)

def parse_expression_chain(span, ctx):
  return _parse_expression_chain(span, ctx)

def match_binary_op(span, ctx):
  if span[0].text in mt_constants.mt_binops:
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
  ATOM_LBRACE,
  Cycle(parse_expression_chain, ATOM_COMMA),
  ATOM_RBRACE
)

parse_call = Dict(
  Field("func", cap_ident),
  Field("params", parse_params)
)

parse_expression_unit = Oneof(
  Field("call", parse_call),
  paren_expression,
  Field("array", parse_braces),
  Capture(ATOM_INT),
  Capture(ATOM_FLOAT),
  Capture(ATOM_IDENT),
)

_parse_expression_chain = Seq(
  Field("exp", parse_expression_unit),
  Any(Seq(
    Field("op ", Capture(match_binary_op)),
    Field("exp", parse_expression_unit),
  ))
)

parse_block = Seq(
  ATOM_LBRACK,
  List(Any(parse_statement)),
  ATOM_RBRACK
)

parse_else = Dict(
  KW_ELSE,
  Field("statements", parse_block),
)

parse_if = Dict(
  KW_IF,
  Field("condition",  paren_expression),
  Field("statements", parse_block),
  Field("else",       Opt(parse_else))
)

parse_case = List(
  KW_CASE,
  Field("condition",  paren_expression),
  #Field("statements", parse_block),
  parse_block,
)

parse_match = List(
  KW_MATCH,
  Field("condition",  paren_expression),
  ATOM_LBRACK,
  Any(Field("case", parse_case)),
  ATOM_RBRACK
)

parse_section_header = Seq(
  ATOM_LBRACE,
  Dict(Field("name", cap_ident)),
  ATOM_RBRACE
)

parse_section = Dict(
  Field("header", parse_section_header),
  Field("body", Any(parse_statement))
)

parse_decl = Dict(
  Field("name",  cap_ident),
  OptSeq(ATOM_COLON, Field("type",  cap_ident)),
  OptSeq(ATOM_EQ,    Field("value", parse_expression_chain)),
  ATOM_SEMI
)

parse_return = Seq(
  KW_RETURN,
  Field("value", parse_expression_chain)
)

_parse_statement = Oneof(
  Field("match",  parse_match),
  Field("if",     parse_if),
  Field("return", parse_return, ATOM_SEMI),

  Field("call",   parse_call, ATOM_SEMI),
  Field("decl",   parse_decl),

  ATOM_SEMI,
  ATOM_NEWLINE,
)

parse_top = Oneof(
  Field("section", parse_section),
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
