# SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
# SPDX-License-Identifier: MIT License

#------------------------------------------------------------------------------

mt_keywords = [
"break",
"case",
"const",
"default",
"do",
"else",
"enum",
"for",
"if",
"import",
"interface",
"match",
"module",
"port",
"return",
"signed",
"struct",
"type",
"union",
"unsigned",
"void",
"while",
]

#------------------------------------------------------------------------------

mt_binops = [
  "!=",
  "%",
  "%=",
  "&",
  "&&",
  "&=",
  "*",
  "*=",
  "+",
  "+=",
  "-",
  "-=",
  ".",
  "/",
  "/=",
  "::",
  "<",
  "<<",
  "<<=",
  "<=",
  "<=>",
  "=",
  "==",
  ">",
  ">=",
  ">>",
  ">>=",
  "^",
  "^=",
  "|",
  "|=",
  "||",
]

#------------------------------------------------------------------------------

def prefix_precedence(op):
  if op in ["++", "--", "+", "-", "!", "~", "*", "&"]:
    return 3
  return 0

def prefix_assoc(op):
  if op in ["++", "--", "+", "-", "!", "~", "*", "&"]:
    return -2
  return 0;

#------------------------------------------------------------------------------

# FIXME should just compare by indexof in some list or something

def binary_precedence(op):
  if op == "::"  : return 1
  if op == "."   : return 2
  if op == "->"  : return 2
  if op == ".*"  : return 4
  if op == "->*" : return 4
  if op == "*"   : return 5
  if op == "/"   : return 5
  if op == "%"   : return 5
  if op == "+"   : return 6
  if op == "-"   : return 6
  if op == "<<"  : return 7
  if op == ">>"  : return 7
  if op == "<=>" : return 8
  if op == "<"   : return 9
  if op == "<="  : return 9
  if op == ">"   : return 9
  if op == ">="  : return 9
  if op == "=="  : return 10
  if op == "!="  : return 10
  if op == "&"   : return 11
  if op == "^"   : return 12
  if op == "|"   : return 13
  if op == "&&"  : return 14
  if op == "||"  : return 15
  if op == "?"   : return 16
  if op == ":"   : return 16
  if op == "="   : return 16
  if op == "+="  : return 16
  if op == "-="  : return 16
  if op == "*="  : return 16
  if op == "/="  : return 16
  if op == "%="  : return 16
  if op == "<<=" : return 16
  if op == ">>=" : return 16
  if op == "&="  : return 16
  if op == "^="  : return 16
  if op == "|="  : return 16
  if op == ","   : return 17
  return 0

#------------------------------------------------------------------------------

"""
constexpr int binary_assoc(const char* op) {
  if (__builtin_strcmp(op, "::")  == 0) return 1;
  if (__builtin_strcmp(op, ".")   == 0) return 1;
  if (__builtin_strcmp(op, "->")  == 0) return 1;
  if (__builtin_strcmp(op, ".*")  == 0) return 1;
  if (__builtin_strcmp(op, "->*") == 0) return 1;
  if (__builtin_strcmp(op, "*")   == 0) return 1;
  if (__builtin_strcmp(op, "/")   == 0) return 1;
  if (__builtin_strcmp(op, "%")   == 0) return 1;
  if (__builtin_strcmp(op, "+")   == 0) return 1;
  if (__builtin_strcmp(op, "-")   == 0) return 1;
  if (__builtin_strcmp(op, "<<")  == 0) return 1;
  if (__builtin_strcmp(op, ">>")  == 0) return 1;
  if (__builtin_strcmp(op, "<=>") == 0) return 1;
  if (__builtin_strcmp(op, "<")   == 0) return 1;
  if (__builtin_strcmp(op, "<=")  == 0) return 1;
  if (__builtin_strcmp(op, ">")   == 0) return 1;
  if (__builtin_strcmp(op, ">=")  == 0) return 1;
  if (__builtin_strcmp(op, "==")  == 0) return 1;
  if (__builtin_strcmp(op, "!=")  == 0) return 1;
  if (__builtin_strcmp(op, "&")   == 0) return 1;
  if (__builtin_strcmp(op, "^")   == 0) return 1;
  if (__builtin_strcmp(op, "|")   == 0) return 1;
  if (__builtin_strcmp(op, "&&")  == 0) return 1;
  if (__builtin_strcmp(op, "||")  == 0) return 1;

  if (__builtin_strcmp(op, "?")   == 0) return -1;
  if (__builtin_strcmp(op, ":")   == 0) return -1;
  if (__builtin_strcmp(op, "=")   == 0) return -1;
  if (__builtin_strcmp(op, "+=")  == 0) return -1;
  if (__builtin_strcmp(op, "-=")  == 0) return -1;
  if (__builtin_strcmp(op, "*=")  == 0) return -1;
  if (__builtin_strcmp(op, "/=")  == 0) return -1;
  if (__builtin_strcmp(op, "%=")  == 0) return -1;
  if (__builtin_strcmp(op, "<<=") == 0) return -1;
  if (__builtin_strcmp(op, ">>=") == 0) return -1;
  if (__builtin_strcmp(op, "&=")  == 0) return -1;
  if (__builtin_strcmp(op, "^=")  == 0) return -1;
  if (__builtin_strcmp(op, "|=")  == 0) return -1;
  if (__builtin_strcmp(op, ",")   == 0) return 1;
  return 0;
}
"""

def suffix_precedence(op):
  if op in ["++", "--"]: return 2
  return 0

def suffix_assoc(op):
  if op in ["++", "--"] : return 2
  return 0
