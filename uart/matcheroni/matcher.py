#!/usr/bin/python3

from functools import cache, partial

# FIXME probably need atom_cmp

def atom_cmp(a, b):
  if isinstance(a, (str, int)) and isinstance(b, (str, int)):
    a_value = ord(a) if isinstance(a, str) else a
    b_value = ord(b) if isinstance(b, str) else b
    return b_value - a_value
  raise ValueError(F"Don't know how to compare {type(a)} and {type(b)}")


#---------------------------------------------------------------------------------------------------

class Fail:
  def __init__(self, span):
    self.span = span
  def __repr__(self):
    span = self.span.encode('unicode_escape').decode('utf-8')
    return f"Fail @ '{span}'"
  def __bool__(self):
    return False

#---------------------------------------------------------------------------------------------------

@cache
def Not(P):
  r"""
  >>> Not(Atom('a'))('asdf', {})
  Fail @ 'asdf'
  >>> Not(Atom('a'))('qwer', {})
  'qwer'
  """
  def match(span, ctx):
    tail = P(span, ctx)
    return span if isinstance(tail, Fail) else Fail(span)
  return match

#---------------------------------------------------------------------------------------------------

@cache
def Atom(C):
  r"""
  >>> Atom('a')('asdf', {})
  'sdf'
  >>> Atom('a')('qwer', {})
  Fail @ 'qwer'
  """
  def match(span, ctx):
    return span[1:] if (span and not atom_cmp(span[0], C)) else Fail(span)
  return match

#---------------------------------------------------------------------------------------------------

@cache
def NotAtom(C):
  r"""
  >>> NotAtom('a')('asdf', {})
  Fail @ 'asdf'
  >>> NotAtom('a')('qwer', {})
  'wer'
  """
  def match(span, ctx):
    return span[1:] if (span and atom_cmp(span[0], C)) else Fail(span)
  return match

#---------------------------------------------------------------------------------------------------

@cache
def Atoms(*args):
  r"""
  >>> Atoms('a', 'b')('asdf', {})
  'sdf'
  >>> Atoms('b', 'a')('asdf', {})
  'sdf'
  >>> Atoms('a', 'b')('qwer', {})
  Fail @ 'qwer'
  """
  def match(span, ctx):
    if not span:
      return Fail(span)
    for arg in args:
      if not atom_cmp(span[0], arg):
        return span[1:]
    return Fail(span)
  return match

#---------------------------------------------------------------------------------------------------

@cache
def NotAtoms(*args):
  def match(span, ctx):
    for arg in args:
      if not atom_cmp(span[0], arg):
        return Fail(span)
    return span[1:]
  return match

#---------------------------------------------------------------------------------------------------

def AnyAtom(span, ctx):
  r"""
  >>> AnyAtom('asdf', {})
  'sdf'
  >>> AnyAtom('', {})
  Fail @ ''
  """
  return span[1:] if span else Fail(span)

#---------------------------------------------------------------------------------------------------

@cache
def Range(A, B):
  r"""
  >>> Range('a', 'z')('asdf', {})
  'sdf'
  >>> Range('b', 'y')('asdf', {})
  Fail @ 'asdf'
  >>> Range('a', 'z')('1234', {})
  Fail @ '1234'
  """
  A = ord(A) if isinstance(A, str) else A
  B = ord(B) if isinstance(B, str) else B

  def match(span, ctx):
    if span and atom_cmp(A, span[0]) >= 0 and atom_cmp(span[0], B) >= 0:
      return span[1:]
    return Fail(span)
  return match

#---------------------------------------------------------------------------------------------------

@cache
def Ranges(*args):
  r"""
  >>> Ranges('a', 'z', 'A', 'Z')('asdf', {})
  'sdf'
  >>> Ranges('a', 'z', 'A', 'Z')('QWER', {})
  'WER'
  >>> Ranges('a', 'z', 'A', 'Z')('1234', {})
  Fail @ '1234'
  """

  ranges = []
  for i in range(0, len(args), 2):
    ranges.append(Range(args[i+0], args[i+1]))

  def match(span, ctx):
    for range in ranges:
      tail = range(span, ctx)
      if not isinstance(tail, Fail):
        return tail
    return Fail(span)
  return match

#---------------------------------------------------------------------------------------------------

@cache
def Lit(lit):
  r"""
  >>> Lit('foo')('foobar', {})
  'bar'
  >>> Lit('foo')('barfoo', {})
  Fail @ 'barfoo'
  """
  def match(span, ctx):
    if len(span) < len(lit):
      return Fail(span)
    for i in range(len(lit)):
      if atom_cmp(span[i], lit[i]):
        return Fail(span)
    return span[len(lit):]
  return match

#---------------------------------------------------------------------------------------------------

@cache
def Charset(lit):
  def match(span, ctx):
    return span[1:] if (span and span[0] in lit) else Fail(span)
  return match

#---------------------------------------------------------------------------------------------------

@cache
def Seq(*args):
  r"""
  >>> Seq(Atom('a'), Atom('s'))('asdf', {})
  'df'
  >>> Seq(Atom('a'), Atom('s'))('a', {})
  Fail @ ''
  >>> Seq(Atom('a'), Atom('s'))('qwer', {})
  Fail @ 'qwer'
  """
  def match(span, ctx):
    for arg in args:
      tail = arg(span, ctx)
      if isinstance(tail, Fail):
        return tail
      span = tail
    return span
  return match

#---------------------------------------------------------------------------------------------------

@cache
def Oneof(*args):
  r"""
  >>> Oneof(Atom('a'), Atom('b'))('asdf', {})
  'sdf'
  >>> Oneof(Atom('b'), Atom('a'))('asdf', {})
  'sdf'
  >>> Oneof(Atom('b'), Atom('a'))('qwer', {})
  Fail @ 'qwer'
  """
  def match(span, ctx):
    for arg in args:
      tail = arg(span, ctx)
      if not isinstance(tail, Fail):
        return tail
    return Fail(span)
  return match

#---------------------------------------------------------------------------------------------------

@cache
def Opt(*args):
  r"""
  >>> Opt(Atom('a'))('asdf', {})
  'sdf'
  >>> Opt(Atom('b'), Atom('a'))('asdf', {})
  'sdf'
  >>> Opt(Atom('a'))('qwer', {})
  'qwer'
  """
  def match(span, ctx):
    for pattern in args:
      tail = pattern(span, ctx)
      if not isinstance(tail, Fail):
        return tail
    return span
  return match

#---------------------------------------------------------------------------------------------------

@cache
def Any(*args):
  r"""
  >>> Any(Atom('a'))('aaaasdf', {})
  'sdf'
  >>> Any(Atom('a'))('baaaasdf', {})
  'baaaasdf'
  """
  pattern = Oneof(*args)
  def match(span, ctx):
    while True:
      tail = pattern(span, ctx)
      if isinstance(tail, Fail):
        return span
      span = tail
  return match

#---------------------------------------------------------------------------------------------------

@cache
def Rep(count, P):
  r"""
  >>> Rep(4, Atom('a'))('aaasdf', {})
  Fail @ 'sdf'
  >>> Rep(4, Atom('a'))('aaaasdf', {})
  'sdf'
  >>> Rep(4, Atom('a'))('aaaaasdf', {})
  'asdf'
  """
  def match(span, ctx):
    for _ in range(count):
      tail = P(span, ctx)
      if isinstance(tail, Fail):
        return tail
      span = tail
    return span
  return match

#---------------------------------------------------------------------------------------------------

@cache
def Some(*args):
  r"""
  >>> Some(Atom('a'))('aaaasdf', {})
  'sdf'
  >>> Some(Atom('a'))('baaaasdf', {})
  Fail @ 'baaaasdf'
  """
  pattern = Oneof(*args)
  def match(span, ctx):
    span = pattern(span, ctx)
    if isinstance(span, Fail):
      return span
    while True:
      tail = pattern(span, ctx)
      if isinstance(tail, Fail):
        return span
      span = tail
  return match

#---------------------------------------------------------------------------------------------------
# 'Until' matches anything until we see the given pattern or we hit EOF.
# The pattern is _not_ consumed.

@cache
def Until(P):
  return Any(Seq(Not(P), AnyAtom))

#---------------------------------------------------------------------------------------------------
# Separated = a,   b , c   , d

@cache
def Trim(P):
  return Seq(Opt(isspace), P, Opt(isspace))

@cache
def Cycle(*args):
  def match(span, ctx):
    while True:
      for pattern in args:
        tail = pattern(span, ctx)
        if isinstance(tail, Fail):
          return span
        span = tail
  return match

@cache
def separated(pattern, delim):
  return Seq(
    pattern,
    Any(Seq(Any(isspace), delim, Any(isspace), pattern)),
    # trailing delimiter OK
    Opt(Seq(Any(isspace), delim))
  )

@cache
def comma_separated(pattern):
  return separated(pattern, Atom(','))

#---------------------------------------------------------------------------------------------------
# Joined = a.b.c.d

@cache
def joined(pattern, delim):
  return Seq(
    pattern,
    Any(Seq(delim, pattern))
    # trailing delimiter _not_ OK
  )

@cache
def dot_joined(pattern):
  return joined(pattern, Atom('.'))

#---------------------------------------------------------------------------------------------------
# Delimited spans

@cache
def delimited_span(ldelim, rdelim):
  return Seq(ldelim, Until(rdelim), rdelim)

dquote_span  = delimited_span(Atom('"'),  Atom('"'))   # note - no \" support
squote_span  = delimited_span(Atom('\''), Atom('\''))  # note - no \' support
bracket_span = delimited_span(Atom('['),  Atom(']'))
brace_span   = delimited_span(Atom('{'),  Atom('}'))
paren_span   = delimited_span(Atom('('),  Atom(')'))
angle_span   = delimited_span(Atom('('),  Atom(')'))
comment_span = delimited_span(Lit("/*"),  Lit("*/"))

def test_delimited_span():
  r"""
  >>> paren_span('(asdf)', {})
  ''
  >>> paren_span('asdf)', {})
  Fail @ 'asdf)'
  >>> paren_span('(asdf', {})
  Fail @ ''
  >>> paren_span('asdf', {})
  Fail @ 'asdf'
  """
  pass

#---------------------------------------------------------------------------------------------------
# Delimited lists

isspace = Atoms(' ','\f','\v','\n','\r','\t')

@cache
def delimited_list(ldelim, pattern, rdelim):
  return Seq(ldelim, Any(isspace), comma_separated(pattern), Any(isspace), rdelim)

@cache
def paren_list(pattern):
  r"""
  >>> paren_list(Atom('a'))('(a,a,a), foo', {})
  ', foo'
  """
  return delimited_list(Atom('('), pattern, Atom(')'))

@cache
def bracket_list(pattern):
  return delimited_list(Atom('['), pattern, Atom(']'))

@cache
def brace_list(pattern):
  r"""
  >>> brace_list(Atom('a'))('{a,a,a}, foobar', {})
  ', foobar'
  >>> brace_list(Atom('a'))('{a,b,a}, foobar', {})
  Fail @ 'b,a}, foobar'
  """
  return delimited_list(Atom('{'), pattern, Atom('}'))

#---------------------------------------------------------------------------------------------------

if __name__ == "__main__":
    import doctest
    doctest.testmod()

#---------------------------------------------------------------------------------------------------
