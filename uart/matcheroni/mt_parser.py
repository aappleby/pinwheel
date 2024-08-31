#!/usr/bin/python3

from matcheroni import *
from mt_lexer import *
import mt_constants

#---------------------------------------------------------------------------------------------------

class BaseNode:
    def items(self):
        return self.__dict__.items()

    def keys(self):
        return self.__dict__.keys()

    def __contains__(self, key):
        return key in self.__dict__

    def __getitem__(self, key):
        return self.__dict__[key]

    def __setitem__(self, key, val):
        self.__dict__[key] = val

    def __getattr__(self, key):
        return self.__dict__[key]

    def __setattr__(self, key, val):
        self.__dict__[key] = val

    def __repr__(self):
        return type(self).__name__ + ":" + self.__dict__.__repr__()

class BlockNode(BaseNode):      pass
class CallNode(BaseNode):       pass
class CaseNode(BaseNode):       pass
class ConstNode(BaseNode):      pass
class DeclNode(BaseNode):       pass
class DefaultNode(BaseNode):    pass
class ElseNode(BaseNode):       pass
class ExpressionNode(BaseNode): pass
class IfNode(BaseNode):         pass
class LambdaNode(BaseNode):     pass
class MatchNode(BaseNode):      pass
class OperatorNode(BaseNode):   pass
class ReturnNode(BaseNode):     pass
class SectionNode(BaseNode):    pass
class TypeNode(BaseNode):       pass

#---------------------------------------------------------------------------------------------------
# Define our atom types for the parser

def LexToAtom(type, span = None):
  if span is None:
    return Atom(type)
  return Atom(Lexeme(type, span))

ATOM_STRING   = LexToAtom(LexemeType.LEX_STRING)
ATOM_CHAR     = LexToAtom(LexemeType.LEX_CHAR)
ATOM_KEYWORD  = LexToAtom(LexemeType.LEX_KEYWORD)
ATOM_IDENT    = LexToAtom(LexemeType.LEX_IDENT)
ATOM_COMMENT  = LexToAtom(LexemeType.LEX_COMMENT)
ATOM_FLOAT    = LexToAtom(LexemeType.LEX_FLOAT)
ATOM_INT      = LexToAtom(LexemeType.LEX_INT)
ATOM_PUNC     = LexToAtom(LexemeType.LEX_PUNCT)
ATOM_OP       = LexToAtom(LexemeType.LEX_OP)

PUNCT_NEWLINE = LexToAtom(LexemeType.LEX_NEWLINE)

PUNCT_COMMA   = LexToAtom(LexemeType.LEX_PUNCT, ",")
PUNCT_SEMI    = LexToAtom(LexemeType.LEX_PUNCT, ";")
PUNCT_DOT     = LexToAtom(LexemeType.LEX_PUNCT, ".")
PUNCT_LPAREN  = LexToAtom(LexemeType.LEX_PUNCT, "(")
PUNCT_RPAREN  = LexToAtom(LexemeType.LEX_PUNCT, ")")
PUNCT_LBRACE  = LexToAtom(LexemeType.LEX_PUNCT, "[")
PUNCT_RBRACE  = LexToAtom(LexemeType.LEX_PUNCT, "]")
PUNCT_LBRACK  = LexToAtom(LexemeType.LEX_PUNCT, "{")
PUNCT_RBRACK  = LexToAtom(LexemeType.LEX_PUNCT, "}")
PUNCT_AT      = LexToAtom(LexemeType.LEX_PUNCT, "@")

KW_MATCH      = LexToAtom(LexemeType.LEX_KEYWORD, "match")
KW_CASE       = LexToAtom(LexemeType.LEX_KEYWORD, "case")
KW_DEFAULT    = LexToAtom(LexemeType.LEX_KEYWORD, "default")
KW_RETURN     = LexToAtom(LexemeType.LEX_KEYWORD, "return")
KW_ELSE       = LexToAtom(LexemeType.LEX_KEYWORD, "else")
KW_IF         = LexToAtom(LexemeType.LEX_KEYWORD, "if")
KW_SIGNED     = LexToAtom(LexemeType.LEX_KEYWORD, "signed")
KW_UNSIGNED   = LexToAtom(LexemeType.LEX_KEYWORD, "unsigned")

#---------------------------------------------------------------------------------------------------

def match_assignop(span, ctx):
  if len(span) and span[0].type == LexemeType.LEX_OP and span[0].text in mt_constants.mt_assignops:
    return span[1:]
  return Fail(span)

def match_declop(span, ctx):
  if len(span) and span[0].type == LexemeType.LEX_OP and span[0].text in mt_constants.mt_declops:
    return span[1:]
  return Fail(span)

def match_binop(span, ctx):
  if len(span) and span[0].type == LexemeType.LEX_OP and span[0].text in mt_constants.mt_binops:
    return span[1:]
  return Fail(span)

match_ident = Seq(
  Opt(PUNCT_AT),
  ATOM_IDENT,
  Any(Seq(
    PUNCT_DOT,
    ATOM_IDENT,
  ))
)

#---------------------------------------------------------------------------------------------------
# Forward decls

def parse_statement(span, ctx):
  return _parse_statement(span, ctx)

def parse_expression_chain(span, ctx):
  return _parse_expression_chain(span, ctx)

def parse_expression(span, ctx):
  return _parse_expression(span, ctx)

def parse_decl(span, ctx):
  return _parse_decl(span, ctx)

parse_ident = Capture(match_ident)

parse_exp_or_decl = Oneof(
  parse_expression,
  parse_decl,
)

#---------------------------------------------------------------------------------------------------

parse_paren_tuple = List(
  PUNCT_LPAREN,
  Oneof(
    PUNCT_RPAREN,
    Seq(
      Any(
        Seq(parse_expression, PUNCT_COMMA),
        Seq(parse_decl,       PUNCT_COMMA),
      ),
      Oneof(
        Seq(parse_expression, PUNCT_RPAREN),
        Seq(parse_decl,       PUNCT_RPAREN)
      )
    )
  )

  #Cycle(parse_exp_or_decl, PUNCT_COMMA),
  #PUNCT_RPAREN
)

parse_bracket_tuple = List(
  PUNCT_LBRACE,
  Cycle(parse_exp_or_decl, PUNCT_COMMA),
  PUNCT_RBRACE
)

parse_block = Seq(
  PUNCT_LBRACK,
  List(Any(parse_statement)),
  PUNCT_RBRACK
)

parse_tuple = Oneof(
  parse_paren_tuple,
  parse_bracket_tuple
)

#---------------------------------------------------------------------------------------------------


#def parse_block(span, ctx):
#  tail = PUNCT_LBRACK(span, ctx)
#  if isinstance(tail, Fail): return Fail(span)
#
#  tail = List(Any(parse_statement))(tail, ctx)
#  if isinstance(tail, Fail): return Fail(span)
#
#  pass

parse_call = Node(CallNode,
  Field("func",   Capture(Oneof(match_ident, ATOM_KEYWORD))),
  Field("params", parse_tuple)
)

parse_lambda = Node(LambdaNode,
  Field("params", parse_tuple),
  Field("body",   parse_block)
)

parse_const = Oneof(
  Capture(ATOM_INT),
  Capture(ATOM_FLOAT),
  Capture(ATOM_STRING),
  parse_ident,
)

parse_expression_unit = Oneof(
  parse_lambda,   # (){}
  parse_call,     # identifier()
  parse_tuple,    # ()
  parse_block,    # {}
  parse_const,    # int | float | string | identifier
)

parse_binop = Capture(match_binop)


# unit op unit op unit...
_parse_expression_chain = List(
  parse_expression_unit,
  Any(Seq(parse_binop, parse_expression_unit))
)

_parse_expression = Node(ExpressionNode,
  Field("exp", parse_expression_chain)
)

parse_else = Node(ElseNode,
  KW_ELSE,
  Field("block", parse_block),
)

parse_if = Node(IfNode,
  KW_IF,
  Field("condition",  parse_tuple),
  Field("block",      parse_block),
  #Field("else",       Opt(parse_else))
)

parse_case = Node(CaseNode,
  KW_CASE,
  Field("condition",  parse_tuple),
  Field("block",      parse_block),
)

parse_default = Node(DefaultNode,
  KW_DEFAULT,
  Field("block", parse_block),
)

parse_match = Node(MatchNode,
  KW_MATCH,
  Field("condition", parse_tuple),
  PUNCT_LBRACK,
  Field("body",
    List(Any(Oneof(
      Field("case",    parse_case),
      Field("default", parse_default),
    )))
  ),
  PUNCT_RBRACK
)

parse_type = Node(TypeNode,
  Field("base", Oneof(
    parse_call,
    parse_tuple,
    parse_ident
  )),
  Opt(Field("suffix", parse_tuple))
)

_parse_decl = Node(DeclNode,
  Field("name",    parse_ident),
  OptSeq(
    Field("dir",   Capture(match_declop)),
    Field("type",  parse_type)
  ),
  OptSeq(
    Field("eq",    Capture(match_assignop)),
    Field("val",   parse_expression)
  ),
)

parse_return = Node(ReturnNode,
  KW_RETURN,
  Field("val", Opt(parse_expression))
)

_parse_statement = Oneof(
  parse_match,
  parse_if,
  Seq(parse_return,     PUNCT_SEMI),
  Seq(parse_call,       PUNCT_SEMI),
  Seq(parse_expression, PUNCT_SEMI),
  Seq(parse_decl,       PUNCT_SEMI),

  PUNCT_SEMI,
  PUNCT_NEWLINE,
)

parse_section = Node(SectionNode,
  PUNCT_LBRACE,
  Field("name", parse_ident),
  PUNCT_RBRACE,
  Field("body", List(Any(parse_statement)))
)

parse_top = Oneof(
  parse_section,
  PUNCT_NEWLINE
)

#---------------------------------------------------------------------------------------------------

def parse_lexemes(lexemes):
  span = lexemes
  ctx = []
  while span:
    tail = parse_top(span, ctx)
    if isinstance(tail, Fail):
      ctx.append(span[0])
      tail = span[1:]
    span = tail
  return ctx

#---------------------------------------------------------------------------------------------------

import doctest
doctest.testmod()
