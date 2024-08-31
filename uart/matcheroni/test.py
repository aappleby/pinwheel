#!/usr/bin/python3

import mt_lexer
import mt_parser
import pprint

from mt_lexer import Lexeme
from mt_parser import BaseNode

filename = "mt/scratch.mt"

#---------------------------------------------------------------------------------------------------

def print_indent(indent):
  print("  " * indent, end='')
  #for i in range(indent - 1):
  #  print("┃ ", end="")
  #print("┗ ", end='')

def dump_node(node, indent = 0):
  print(f"{type(node).__name__}{{}}")
  for key, val in node.items():
    print_indent(indent)
    print(f".{key} : ", end='')
    dump_variant(val, indent + 1)

def dump_array(array, indent = 0):
  print(f"Array[{len(array)}]")
  for key, val in enumerate(array):
    print_indent(indent)
    print(f"[{key}] : ", end="")
    dump_variant(val, indent + 1)

def dump_lexeme(lexeme, indent = 0):
  print(f"{lexeme.type.name} = '{lexeme.text}'")

def dump_variant(variant, indent = 0):
  if isinstance(variant, BaseNode):
    dump_node(variant, indent)
  elif isinstance(variant, list):
    dump_array(variant, indent)
  elif isinstance(variant, Lexeme):
    dump_lexeme(variant, indent)
  elif variant is None:
    print(f"<None>")
  else:
    print(f"??? {variant}")

#---------------------------------------------------------------------------------------------------

if __name__ == "__main__":
  print("test begin")

  source = open(filename).read()
  lexemes = mt_lexer.lex_string(source)

  print()
  print("# lexemes")
  for lexeme in lexemes:
    print(lexeme)

  print()
  print("# parsing")
  trees = mt_parser.parse_lexemes(lexemes)

  failed = False
  for tree in trees:
    if isinstance(tree, Lexeme):
      print(f"Parsing failed at {tree}")
      failed = True

  if not failed:
    print()
    print("# trees")
    print("trees : ", end="")
    dump_variant(trees, 1)

  print()

  print("test end")
  pass
