#!/usr/bin/python3

#---------------------------------------------------------------------------------------------------

def Capture(pattern):
  """
  Adds the span matched by 'pattern' to the context stack
  """
  def match(span, ctx):
    tail = pattern(span, ctx)
    if not isinstance(tail, Fail):
      ctx.append(span[:len(span) - len(tail)])
    return tail
  return match

def List(pattern):
  """
  Turns all elements added to the context stack after this matcher into a list.
  """
  def match(span, ctx):
    old_len = len(ctx)
    tail = pattern(span, ctx)
    if not isinstance(tail, Fail):
      result = ctx[old_len:]
      del ctx[old_len:]
      ctx.append(result)
    return tail
  return match

def Dict(pattern):
  """
  Turns all elements added to the context stack after this matcher into a dict.
  """
  def match(span, ctx):
    old_len = len(ctx)
    tail = pattern(span, ctx)
    if not isinstance(tail, Fail):
      fields = ctx[old_len:]
      del ctx[old_len:]
      result = {}
      for field in fields:
        result[field[0]] = field[1]
      ctx.append(result)
    return tail
  return match

def Field(name, pattern):
  """
  Turns the top of the context stack into a (name, value) tuple
  """
  def match(span, ctx):
    tail = pattern(span, ctx)
    if not isinstance(tail, Fail):
      field = (name, ctx.pop())
      ctx.append(field)
    return tail
  return match
