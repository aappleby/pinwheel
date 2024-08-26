#!/usr/bin/python3

import matcher
from matcher import *
from enum import Enum
import c_constants

#---------------------------------------------------------------------------------------------------

LexemeType = Enum(
  'LexemeType',
  [
    "LEX_INVALID",
    "LEX_SPACE",
    "LEX_NEWLINE",
    "LEX_STRING",
    "LEX_KEYWORD",
    "LEX_IDENTIFIER",
    "LEX_COMMENT",
    "LEX_PREPROC",
    "LEX_FLOAT",
    "LEX_INT",
    "LEX_PUNCT",
    "LEX_CHAR",
    "LEX_SPLICE",
    "LEX_FORMFEED",
    "LEX_BOF",
    "LEX_EOF",
    "LEX_LAST"
  ]
)

def lex_to_color(lex_type):
  match lex_type:
    case LexemeType.LEX_INVALID    : return 0x0000FF
    case LexemeType.LEX_SPACE      : return 0x804040
    case LexemeType.LEX_NEWLINE    : return 0x404080
    case LexemeType.LEX_STRING     : return 0x4488AA
    case LexemeType.LEX_KEYWORD    : return 0x0088FF
    case LexemeType.LEX_IDENTIFIER : return 0xCCCC40
    case LexemeType.LEX_COMMENT    : return 0x66AA66
    case LexemeType.LEX_PREPROC    : return 0xCC88CC
    case LexemeType.LEX_FLOAT      : return 0xFF88AA
    case LexemeType.LEX_INT        : return 0xFF8888
    case LexemeType.LEX_PUNCT      : return 0x808080
    case LexemeType.LEX_CHAR       : return 0x44DDDD
    case LexemeType.LEX_SPLICE     : return 0x00CCFF
    case LexemeType.LEX_FORMFEED   : return 0xFF00FF
    case LexemeType.LEX_BOF        : return 0x80FF80
    case LexemeType.LEX_EOF        : return 0x8080FF
    case LexemeType.LEX_LAST       : return 0xFF00FF
  return 0xFF00FF

#---------------------------------------------------------------------------------------------------

def EOL(span, ctx):
  if not span:
    return span
  if span[0] == '\n':
    return span
  return Fail(span)

#---------------------------------------------------------------------------------------------------
# Character types from ctype.h

isalnum  = Ranges('0','9','a','z','A','Z')
isalpha  = Ranges('a', 'z', 'A', 'Z')
isblank  = Atoms(' ', '\t')
iscntrl  = Ranges(0x00, 0x1F, 0x7F, 0x7F)
isdigit  = Range('0', '9')
isgraph  = Range(0x21, 0x7E)
islower  = Range('a', 'z')
isprint  = Range(0x20, 0x7E)
ispunct  = Ranges(0x21, 0x2F, 0x3A, 0x40, 0x5B, 0x60, 0x7B, 0x7E)
isspace  = Atoms(' ','\f','\v','\n','\r','\t')
isupper  = Range('A', 'Z')
isxdigit = Ranges('0','9','a','f','A','F')

#match_space   = Some(Atoms(' ', '\t'))
#match_newline = Seq(Opt(Atom('\r')), Atom('\n'))

lex_newline = Seq(Opt(Atom('\r')), Atom('\n'))

#  using ws = Atoms<' ', '\t'>;
#  using pattern = Some<ws>;

#whitespace = Some(isspace)

lex_space = Some(Atoms(' ', '\t'))

#---------------------------------------------------------------------------------------------------
# Basic UTF8

utf8_ext       = Range(0x80, 0xBF)
utf8_onebyte   = Range(0x00, 0x7F)
utf8_twobyte   = Seq(Range(0xC0, 0xDF), utf8_ext)
utf8_threebyte = Seq(Range(0xE0, 0xEF), utf8_ext, utf8_ext)
utf8_fourbyte  = Seq(Range(0xF0, 0xF7), utf8_ext, utf8_ext, utf8_ext)
utf8_bom       = Seq(Atom(0xEF), Atom(0xBB), Atom(0xBF))

utf8_char = Oneof(
  utf8_twobyte,
  utf8_threebyte,
  utf8_fourbyte
)

#---------------------------------------------------------------------------------------------------
# Integers

sign = Atoms('+','-')

@cache
def Ticked(c):
  return Seq(Opt(Atom('\'')), c)

decimal_digit    = Range('0', '9')
decimal_nonzero  = Range('1', '9')
decimal_constant = Seq(decimal_nonzero, Any(Ticked(decimal_digit)))

hexadecimal_prefix         = Oneof(Lit("0x"), Lit("0X"))
hexadecimal_digit          = Oneof(Range('0','9'), Range('a','f'), Range('A', 'F'))
hexadecimal_digit_sequence = Seq(hexadecimal_digit, Any(Ticked(hexadecimal_digit)));
hexadecimal_constant       = Seq(hexadecimal_prefix, hexadecimal_digit_sequence);

binary_prefix         = Oneof(Lit("0b"), Lit("0B"))
binary_digit          = Atoms('0','1');
binary_digit_sequence = Seq(binary_digit, Any(Ticked(binary_digit)));
binary_constant       = Seq(binary_prefix, binary_digit_sequence);

octal_digit    = Range('0', '7')
octal_constant = Seq(Atom('0'), Any(Ticked(octal_digit)));

unsigned_suffix        = Atoms('u', 'U')
long_suffix            = Atoms('l', 'L')
long_long_suffix       = Oneof(Lit("ll"), Lit("LL"))
bit_precise_int_suffix = Oneof(Lit("wb"), Lit("WB"))

# This is a little odd because we have to match in longest-suffix-first order to ensure we capture
# the entire suffix
integer_suffix = Oneof(
  Seq(unsigned_suffix,  long_long_suffix),
  Seq(unsigned_suffix,  long_suffix),
  Seq(unsigned_suffix,  bit_precise_int_suffix),
  Seq(unsigned_suffix),
  Seq(long_long_suffix,       Opt(unsigned_suffix)),
  Seq(long_suffix,            Opt(unsigned_suffix)),
  Seq(bit_precise_int_suffix, Opt(unsigned_suffix))
)

# GCC allows i or j in addition to the normal suffixes for complex-ified types :/...
complex_suffix = Atoms('i', 'j')

# Octal has to be _after_ bin/hex so we don't prematurely match the prefix
lex_int = Seq(
  Oneof(
    decimal_constant,
    hexadecimal_constant,
    binary_constant,
    octal_constant
  ),
  Seq(
    Opt(complex_suffix),
    Opt(integer_suffix),
    Opt(complex_suffix)
  )
)

#---------------------------------------------------------------------------------------------------
# 6.4.4.2 Floating constants

floating_suffix = Oneof(
  Atom('f'), Atom('l'), Atom('F'), Atom('L'),
  # Decimal floats, GCC thing
  Lit("df"), Lit("dd"), Lit("dl"),
  Lit("DF"), Lit("DD"), Lit("DL")
)

digit = Range('0', '9')

digit_sequence = Seq(digit, Any(Ticked(digit)))

fractional_constant = Oneof(
  Seq(Opt(digit_sequence), Atom('.'), digit_sequence),
  Seq(digit_sequence, Atom('.'))
)

sign = Atoms('+','-')

hexadecimal_digit          = Ranges('0','9','a','f','A','F')
hexadecimal_digit_sequence = Seq(hexadecimal_digit, Any(Ticked(hexadecimal_digit)))
hexadecimal_fractional_constant = Oneof(
  Seq(Opt(hexadecimal_digit_sequence), Atom('.'), hexadecimal_digit_sequence),
  Seq(hexadecimal_digit_sequence, Atom('.'))
)

exponent_part        = Seq(Atoms('e', 'E'), Opt(sign), digit_sequence)
binary_exponent_part = Seq(Atoms('p', 'P'), Opt(sign), digit_sequence)

# GCC allows i or j in addition to the normal suffixes for complex-ified types :/...
complex_suffix = Atoms('i', 'j')

decimal_floating_constant = Oneof(
  Seq( fractional_constant, Opt(exponent_part), Opt(complex_suffix), Opt(floating_suffix), Opt(complex_suffix) ),
  Seq( digit_sequence, exponent_part,           Opt(complex_suffix), Opt(floating_suffix), Opt(complex_suffix) )
)

hexadecimal_prefix         = Oneof(Lit("0x"), Lit("0X"))

hexadecimal_floating_constant = Seq(
  hexadecimal_prefix,
  Oneof(hexadecimal_fractional_constant, hexadecimal_digit_sequence),
  binary_exponent_part,
  Opt(complex_suffix),
  Opt(floating_suffix),
  Opt(complex_suffix)
)

lex_float = Oneof(
  decimal_floating_constant,
  hexadecimal_floating_constant
)

#---------------------------------------------------------------------------------------------------

n_char = Not(Atoms('}','\n'))
n_char_sequence = Some(n_char)
named_universal_character = Seq(Lit("\\N{"), n_char_sequence, Lit("}"))

hex_quad = Rep(4, hexadecimal_digit)

universal_character_name = Oneof(
  Seq( Lit("\\u"), hex_quad ),
  Seq( Lit("\\U"), hex_quad, hex_quad ),
  Seq( Lit("\\u{"), Any(hexadecimal_digit), Lit("}")),
  named_universal_character
)

#---------------------------------------------------------------------------------------------------
# Escape sequences

# This is what's in the spec...
# simple_escape_sequence      = Seq(Atom('\\'),
# Charset("'\"?\\abfnrtv"))

# ...but GCC adds \e and \E, and '\(' '\{' '\[' '\%' are warnings but allowed

simple_escape_sequence      = Seq(Atom('\\'), Charset("'\"?\\abfnrtveE({[%"))

octal_digit = Range('0', '7')
octal_escape_sequence = Oneof(
  Seq(Atom('\\'), octal_digit, Opt(octal_digit), Opt(octal_digit)),
  Seq(Lit("\\o{"), Any(octal_digit), Lit("}"))
)

hexadecimal_digit = Ranges('0','9','a','f','A','F')
hexadecimal_escape_sequence = Oneof(
  Seq(Lit("\\x"), Some(hexadecimal_digit)),
  Seq(Lit("\\x{"), Any(hexadecimal_digit), Lit("}"))
)

escape_sequence = Oneof(
  simple_escape_sequence,
  octal_escape_sequence,
  hexadecimal_escape_sequence,
  universal_character_name
)

#---------------------------------------------------------------------------------------------------
# 5.1.1.2 : Lines ending in a backslash and a newline get spliced together
# with the following line.

# According to GCC it's only a warning to have whitespace between the
# backslash and the newline... and apparently \r\n is ok too?

lex_splice = Seq(
  Atom('\\'),
  Any(Atoms(' ','\t')),
  Opt(Atom('\r')),
  Any(Atoms(' ','\t')),
  Atom('\n')
)

lex_formfeed = Atom('\f')



#---------------------------------------------------------------------------------------------------

lex_preproc = Seq(
  Atom('#'),
  Any(
    lex_splice,
    NotAtom('\n')
  )
)

#---------------------------------------------------------------------------------------------------
# 6.4.5 String literals

# Note, we add splices here since we're matching before preproccessing.

s_char          = Oneof(lex_splice, escape_sequence, NotAtoms('"', '\\', '\n'))
s_char_sequence = Some(s_char)
encoding_prefix = Oneof(Lit("u8"), Atoms('u', 'U', 'L')) # u8 must go first
string_literal  = Seq(Opt(encoding_prefix), Atom('"'), Opt(s_char_sequence), Atom('"'))

#----------------------------------------
# Raw string literals from the C++ spec

encoding_prefix    = Oneof(Lit("u8"), Atoms('u', 'U', 'L')) # u8 must go first

# We ignore backslash in d_char for similar splice-related reasons
#d_char          = NotAtoms(' ', '(', ')', '\\', '\t', '\v', '\f', '\n')
d_char          = NotAtoms(' ', '(', ')', '\t', '\v', '\f', '\n')
d_char_sequence = Some(d_char)
backref_type    = Opt(d_char_sequence)

#r_terminator = Seq(
#  Atom(')'),
#  MatchBackref("raw_delim", char, backref_type),
#  Atom('"')
#)

#r_char          = Seq(Not(r_terminator), AnyAtom)
#r_char_sequence = Some(r_char)

#raw_string_literal = Seq(
#  Opt(encoding_prefix),
#  Atom('R'),
#  Atom('"'),
#  StoreBackref("raw_delim", char, backref_type),
#  Atom('('),
#  Opt(r_char_sequence),
#  Atom(')'),
#  MatchBackref("raw_delim", char, backref_type),
#  Atom('"')
#)

#----------------------------------------

lex_string = Oneof(
  string_literal
  #Ref(match_cooked_string_literal),
  #Ref(match_raw_string_literal)
)

#------------------------------------------------------------------------------
# 6.4.9 Comments

# Single-line comments
single_line_comment = Seq(Lit("//"), Until(EOL))

# Multi-line non-nested comments
multi_line_comment = Seq(Lit("/*"), Until(Lit("*/")), Lit("*/"))

lex_comment = Oneof(single_line_comment, multi_line_comment)

#------------------------------------------------------------------------------
# 6.4.6 Punctuators

# We're just gonna match these one punct at a time
lex_punct = Charset("-,;:!?.()[]{}*/&#%^+(=)|~")

# Yeaaaah, not gonna try to support trigraphs, they're obsolete and have been
# removed from the latest C spec. Also we have to declare them funny to get
# them through the preprocessor...
# trigraphs = Trigraphs(R"(??=)" R"(??()" R"(??/)" R"(??))" R"(??')"
# R"(??()" R"(??!)" R"(??))" R"(??-)")

#---------------------------------------------------------------------------------------------------
# 6.4.4.4 Character constants

# Multi-character character literals are allowed by spec, but their meaning
# is implementation-defined...

c_char             = Oneof(escape_sequence, NotAtoms('\'', '\\', '\n'))
#c_char_sequence    = Some(c_char)

encoding_prefix    = Oneof(Lit("u8"), Atoms('u', 'U', 'L')) # u8 must go first

# The spec disallows empty character constants, but...
#character_constant = Seq( Opt(encoding_prefix), Atom('\''), c_char_sequence, Atom('\'') )

# ...in GCC they're only a warning.
lex_char = Seq( Opt(encoding_prefix), Atom('\''), Any(c_char), Atom('\'') )

#---------------------------------------------------------------------------------------------------
# 6.4.2 Identifiers - GCC allows dollar signs in identifiers?

digit = Range('0', '9')

# Not sure if this should be in here
latin1_ext = Range(128,255)

nondigit = Oneof(
  Range('a', 'z'),
  Range('A', 'Z'),
  Atom('_'),
  Atom('$'),
  universal_character_name,
  latin1_ext,      # One of the GCC test files requires this
  utf8_char        # Lots of GCC test files for unicode
)

lex_ident = Seq(nondigit, Any(digit, nondigit))

def lex_keyword(span, ctx):
  tail = lex_ident(span, ctx)
  if isinstance(tail, Fail):
    return tail
  lexeme = span[:len(span) - len(tail)]
  return tail if lexeme in c_constants.c_keywords else Fail(span)

#---------------------------------------------------------------------------------------------------

def lex_eof(span, ctx):
  r"""
  >>> lex_eof('asdf', {})
  Fail @ 'asdf'
  >>> lex_eof('', {})
  ''

  # doesn't work right in python?
  #>>> lex_eof('\0', {})
  #''
  """
  if not span:
    return span
  return Atom(0)(span, ctx)

#---------------------------------------------------------------------------------------------------
# Match char needs to come before match identifier because of its possible L'_' prefix...

def next_lexeme(span, ctx):
  matchers = [
    (lex_space,    LexemeType.LEX_SPACE),
    (Capture(lex_newline),  LexemeType.LEX_NEWLINE),
    (lex_string,   LexemeType.LEX_STRING),
    (lex_char,     LexemeType.LEX_CHAR),
    (lex_keyword,  LexemeType.LEX_KEYWORD),
    (lex_ident,    LexemeType.LEX_IDENTIFIER),
    (lex_comment,  LexemeType.LEX_PREPROC),
    (lex_preproc,  LexemeType.LEX_NEWLINE),
    (lex_float,    LexemeType.LEX_FLOAT),
    (lex_int,      LexemeType.LEX_INT),
    (lex_punct,    LexemeType.LEX_PUNCT),
    (lex_splice,   LexemeType.LEX_SPLICE),
    (lex_formfeed, LexemeType.LEX_FORMFEED),
    (lex_eof,      LexemeType.LEX_EOF),
  ]

  for matcher in matchers:
    if not isinstance(tail := matcher[0](span, ctx), Fail):
      return (matcher[1], span[:len(span) - len(tail)])

  raise ValueError(f"Don't know how to lex '{span[:8]}...'")

#---------------------------------------------------------------------------------------------------

if __name__ == "__main__":
    import doctest
    doctest.testmod()
    doctest.testmod(matcher)

    source = "int x = -12.0f;"

    while source:
      lexeme = next_lexeme(source, {})
      print(lexeme)
      source = source[len(lexeme[1]):]

    #print(next_lexeme("this_is_an_identifier = 7"))
    #print(next_lexeme("default: slkdfj"))
    #print(next_lexeme(" \t stuff"))
    #print(next_lexeme("\r\nhello world"))
    #print(next_lexeme("\"asdf\" narp"))
    #print(next_lexeme("'q' blerp"))
    #print(next_lexeme("/* comment */ int x"))
    #print(next_lexeme("// comment\nnextline"))
    #print(next_lexeme("#ifdef BLAH blee\nnextline"))
    #print(next_lexeme("1.08348e18f;"))
    #print(next_lexeme("0x8477FFFF;"))
    #print(next_lexeme("-7;"))
    #print(next_lexeme("\\\nasdf;"))
    #print(next_lexeme("\f\nasdf;"))
    #print(next_lexeme(""))

#---------------------------------------------------------------------------------------------------
