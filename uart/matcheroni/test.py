#!/usr/bin/python3

import mt_lexer
import mt_parser
import pprint

#---------------------------------------------------------------------------------------------------

def print_tree(tree, indent = 0):
  if isinstance(tree, tuple):
    key = tree[0]
    val = tree[1]
    print("  " * indent, end='')
    print(f"{key}", end = '')

    if isinstance(val, list):
      print("[]:")
      print_tree(val, indent + 1)
    elif isinstance(val, dict):
      print("{}:")
      print_tree(val, indent + 1)
    elif isinstance(val, tuple):
      print("():")
      print_tree(val, indent + 1)
    else:
      print(f": {val}")

  elif isinstance(tree, dict):
    for key, val in tree.items():
      print_tree((key, val), indent)

  elif isinstance(tree, list):
    for val in tree:
      print_tree(val, indent)

  else:
    print("  " * indent, end='')
    print(f"{tree}")

#---------------------------------------------------------------------------------------------------

if __name__ == "__main__":
  print("test begin")

  source = open("mt/scratch.mt").read()
  lexemes = mt_lexer.lex_string(source)

  print()
  print("# lexemes")
  for lexeme in lexemes:
    print(lexeme)

  print()
  print("# parsing")
  trees = mt_parser.parse_lexemes(lexemes)

  print()
  print("# trees")
  for tree in trees:
    #pprint.pprint(tree, sort_dicts=False, indent=2)
    print_tree(tree)

  print()
  print("test end")
  pass













#dquote_span  = Delimited(Atom('"'),  Atom('"'))   # note - no \" support
#squote_span  = Delimited(Atom('\''), Atom('\''))  # note - no \' support
#bracket_span = Delimited(Atom('['),  Atom(']'))
#brace_span   = Delimited(Atom('{'),  Atom('}'))
#paren_span   = Delimited(Atom('('),  Atom(')'))
#angle_span   = Delimited(Atom('('),  Atom(')'))
#comment_span = Delimited(Lit("/*"),  Lit("*/"))
#
#def test_delimited_span():
#  r"""
#  >>> paren_span('(asdf)', {})
#  ''
#  >>> paren_span('asdf)', {})
#  Fail @ 'asdf)'
#  >>> paren_span('(asdf', {})
#  Fail @ ''
#  >>> paren_span('asdf', {})
#  Fail @ 'asdf'
#  """
#  pass
#
#
#
#isspace = Atoms(' ','\f','\v','\n','\r','\t')
#
#@cache
#def delimited_list(ldelim, pattern, rdelim):
#  return Seq(ldelim, Any(isspace), comma_separated(pattern), Any(isspace), rdelim)
#
#@cache
#def paren_list(pattern):
#  r"""
#  >>> paren_list(Atom('a'))('(a,a,a), foo', {})
#  ', foo'
#  """
#  return delimited_list(Atom('('), pattern, Atom(')'))
#
#@cache
#def bracket_list(pattern):
#  return delimited_list(Atom('['), pattern, Atom(']'))
#
#@cache
#def brace_list(pattern):
#  r"""
#  >>> brace_list(Atom('a'))('{a,a,a}, foobar', {})
#  ', foobar'
#  >>> brace_list(Atom('a'))('{a,b,a}, foobar', {})
#  Fail @ 'b,a}, foobar'
#  """
#  return delimited_list(Atom('{'), pattern, Atom('}'))
#
#@cache
#def dot_joined(pattern):
#  return joined(pattern, Atom('.'))
#
#@cache
#def comma_separated(pattern):
#  return separated(pattern, Atom(','))
