#!/usr/bin/python3

from functools import cache

#---------------------------------------------------------------------------------------------------
# FIXME probably need a better way to handle atom_cmp

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
    if isinstance(self.span, str):
      span = self.span.encode('unicode_escape').decode('utf-8')
    else:
      span = str(self.span)
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
    top = len(ctx)
    for arg in args:
      tail = arg(span, ctx)
      if isinstance(tail, Fail):
        del ctx[top:]
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
    top = len(ctx)
    for arg in args:
      tail = arg(span, ctx)
      if not isinstance(tail, Fail):
        return tail
      del ctx[top:]
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

@cache
def OptSeq(*args):
  return Opt(Seq(*args))

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
    top = len(ctx)
    for _ in range(count):
      tail = P(span, ctx)
      if isinstance(tail, Fail):
        del ctx[top:]
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
# Separated = a,b,c,d,

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

#---------------------------------------------------------------------------------------------------
# a,b,c,d,
# trailing delimiter OK

@cache
def Separated(pattern, delim):
  return Cycle(pattern, delim)

#---------------------------------------------------------------------------------------------------
# Joined = a.b.c.d
# trailing delimiter _not_ OK

@cache
def Joined(pattern, delim):
  return Seq(
    pattern,
    Any(Seq(delim, pattern))
  )

#---------------------------------------------------------------------------------------------------
# Delimited spans

@cache
def Delimited(ldelim, rdelim):
  return Seq(ldelim, Until(rdelim), rdelim)

#---------------------------------------------------------------------------------------------------
# Create stuff in the context

@cache
def Capture(pattern):
  """
  Adds the span matched by 'pattern' to the context stack
  """
  def match(span, ctx):
    tail = pattern(span, ctx)
    if not isinstance(tail, Fail):
      token = span[:len(span) - len(tail)]
      if len(token) == 1:
        token = token[0]
      ctx.append(token)
    return tail
  return match

@cache
def List(*args):
  """
  Turns all elements added to the context stack after this matcher into a list.
  """
  def match(span, ctx):
    top = len(ctx)
    tail = span

    for pattern in args:
      tail = pattern(tail, ctx)
      if isinstance(tail, Fail):
        del ctx[top:]
        return tail

    values = ctx[top:]
    del ctx[top:]

    ctx.append(values)
    return tail
  return match

@cache
def Dict(*args):
  """
  Turns all (name, value) tuples added to the context stack after this matcher into a dict.
  """
  def match(span, ctx):
    top = len(ctx)
    tail = span

    for pattern in args:
      tail = pattern(tail, ctx)
      if isinstance(tail, Fail):
        del ctx[top:]
        return tail

    values = ctx[top:]
    del ctx[top:]

    result = {}
    for field in values:
      result[field[0]] = field[1]
    ctx.append(result)
    return tail

  return match

@cache
def Field(name, *args):
  """
  Turns the top of the context stack into a (name, value) tuple
  """
  def match(span, ctx):
    top = len(ctx)
    tail = span

    for pattern in args:
      tail = pattern(tail, ctx)
      if isinstance(tail, Fail):
        del ctx[top:]
        return tail

    values = ctx[top:]
    del ctx[top:]

    if len(values) == 0:
      field = (name, None)
    elif len(values) == 1:
      field = (name, values[0])
    else:
      field = (name, values)

    ctx.append(field)
    return tail

  return match

#---------------------------------------------------------------------------------------------------

import doctest
doctest.testmod()
