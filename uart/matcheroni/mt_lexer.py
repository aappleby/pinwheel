#!/usr/bin/python3

import matcheroni
from matcheroni import *
from enum import Enum
import mt_constants
import re

#---------------------------------------------------------------------------------------------------

class Lexeme:
  def __init__(self, type, span):
    self.type = type
    self.text = span

  def __repr__(self):
    span = self.text.encode('unicode_escape').decode('utf-8')
    if len(span) > 40:
      span = span[:40] + "..."

    match self.type:
      case LexemeType.LEX_NEWLINE:
        return f"{self.type.name}"
      case _:
        return f"{self.type.name}({span})"

#---------------------------------------------------------------------------------------------------

def strcmp(str1, str2):
    if str1 < str2:
        return -1
    elif str1 > str2:
        return 1
    else:
        return 0

def atom_cmp(a, b, base = matcheroni.atom_cmp):
  if isinstance(a, Lexeme) and isinstance(b, Lexeme):
    if a.type.value != b.type.value:
      return b.type.value - a.type.value
    return strcmp(a.text, b.text)

  if isinstance(a, Lexeme) and isinstance(b, LexemeType):
    return b.value - a.type.value
  return base(a, b)

matcheroni.atom_cmp = atom_cmp

#---------------------------------------------------------------------------------------------------

LexemeType = Enum(
  'LexemeType',
  [
    "LEX_SPACE",
    "LEX_NEWLINE",
    "LEX_STRING",
    "LEX_KEYWORD",
    "LEX_IDENT",
    "LEX_COMMENT",
    "LEX_FLOAT",
    "LEX_INT",
    "LEX_PUNCT",
    "LEX_CHAR",
  ]
)

def lex_to_color(lex_type):
  match lex_type:
    case LexemeType.LEX_SPACE    : return 0x804040
    case LexemeType.LEX_NEWLINE  : return 0x404080
    case LexemeType.LEX_STRING   : return 0x4488AA
    case LexemeType.LEX_KEYWORD  : return 0x0088FF
    case LexemeType.LEX_IDENT    : return 0xCCCC40
    case LexemeType.LEX_COMMENT  : return 0x66AA66
    case LexemeType.LEX_FLOAT    : return 0xFF88AA
    case LexemeType.LEX_INT      : return 0xFF8888
    case LexemeType.LEX_PUNCT    : return 0x808080
    case LexemeType.LEX_CHAR     : return 0x44DDDD
  return 0xFF00FF

#---------------------------------------------------------------------------------------------------

match_space = Some(Atoms(' ', '\t'))

match_newline = Seq(Opt(Atom('\r')), Atom('\n'))

sign = Atoms('+','-')

@cache
def Ticked(c):
  return Seq(Opt(Atom('\'')), c)

nondigit     = Oneof(Range('a', 'z'), Range('A', 'Z'), Atom('_'))

dec_digit    = Range('0', '9')
dec_digits   = Seq(dec_digit, Any(Ticked(dec_digit)))
dec_constant = dec_digits

hex_prefix   = Oneof(Lit("0x"), Lit("0X"))
hex_digit    = Oneof(Range('0','9'), Range('a','f'), Range('A', 'F'))
hex_digits   = Seq(hex_digit, Any(Ticked(hex_digit)))
hex_constant = Seq(hex_prefix, hex_digits)

bin_prefix   = Oneof(Lit("0b"), Lit("0B"))
bin_digit    = Atoms('0', '1')
bin_digits   = Seq(bin_digit, Any(Ticked(bin_digit)))
bin_constant = Seq(bin_prefix, bin_digits)

float_suffix = Seq(Atoms('e', 'E'), Opt(sign), dec_digits)

match_int = Oneof(
  dec_constant,
  hex_constant,
  bin_constant
)

match_float = Seq(
  dec_digits,
  Atom('.'),
  dec_digits,
  Opt(float_suffix)
)

match_string = Seq(
  Atom('"'),
  Any(
    Lit("\\\""),
    NotAtom("\"")
  ),
  Atom('"')
)

match_comment = Oneof(
  Seq(Lit("//"), Until(Atom('\n'))),
  Seq(Lit("/*"), Until(Lit("*/")), Lit("*/"))
)

match_punct = Charset("-,;:!?.()[]{}<>*/&#%^+=|~@")

match_char = Seq(
  Atom('\''),
  NotAtoms('\'', '\\', '\n'),
  Atom('\'')
)

match_ident = Seq(nondigit, Any(dec_digit, nondigit))

def match_keyword(span, ctx):
  tail = match_ident(span, ctx)
  if isinstance(tail, Fail):
    return tail
  lexeme = span[:len(span) - len(tail)]
  return tail if lexeme in mt_constants.mt_keywords else Fail(span)

#---------------------------------------------------------------------------------------------------

def MatchToLex(pattern, type):
  def match(span, ctx):
    tail = pattern(span, ctx)
    if not isinstance(tail, Fail):
      ctx.append(Lexeme(type, span[:len(span) - len(tail)]))
    return tail
  return match

#---------------------------------------------------------------------------------------------------

def next_lexeme(span, ctx):
  matchers = [
    match_space,
    #MatchToLex(match_newline,  LexemeType.LEX_NEWLINE),
    match_newline,
    MatchToLex(match_string,   LexemeType.LEX_STRING),
    MatchToLex(match_char,     LexemeType.LEX_CHAR),
    MatchToLex(match_keyword,  LexemeType.LEX_KEYWORD),
    MatchToLex(match_ident,    LexemeType.LEX_IDENT),
    #MatchToLex(match_comment,  LexemeType.LEX_COMMENT),
    match_comment,
    MatchToLex(match_float,    LexemeType.LEX_FLOAT),
    MatchToLex(match_int,      LexemeType.LEX_INT),
    MatchToLex(match_punct,    LexemeType.LEX_PUNCT),
  ]

  for matcher in matchers:
    tail = matcher(span, ctx)
    if not isinstance(tail, Fail):
      return tail

  raise ValueError(f"Don't know how to lex '{span[:8]}...'")

def lex_string(source):
  span = source
  ctx = []
  while span:
    span = next_lexeme(span, ctx)
  return ctx


#---------------------------------------------------------------------------------------------------

import doctest
doctest.testmod()
