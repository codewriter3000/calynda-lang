export const GRAMMAR_LITERALS = `
(* ================================================================ *)
(* 6. PRIMARY EXPRESSIONS                                           *)
(* ================================================================ *)

PrimaryExpression
    = Literal
    | Identifier
    | DiscardExpression                (* _ discard / wildcard      *)
    | "(" Expression ")"              (* parenthesized expression  *)
    | CastExpression                   (* type conversion           *)
    | ArrayLiteral                     (* array literal [1, 2, 3]   *)
    ;

(* The discard expression _ can be used as an assignment target     *)
(* to explicitly ignore a value.                                    *)
(* Example: _ = computeSomething();                                 *)
DiscardExpression
    = "_"
    ;

(* Function-style cast — the callee is a type keyword, so the      *)
(* parser can distinguish this from a regular function call whose   *)
(* callee is an Identifier.                                         *)
(* Example: int32(myFloat)                                          *)
CastExpression
    = PrimitiveType "(" Expression ")"
    ;

ArrayLiteral
    = "[" [ Expression { "," Expression } ] "]"
    ;


(* ================================================================ *)
(* 7. LITERALS                                                      *)
(* ================================================================ *)

Literal
    = IntegerLiteral
    | FloatLiteral
    | BoolLiteral
    | CharLiteral
    | StringLiteral
    | TemplateLiteral
    | NullLiteral
    ;

(* --- Integer literals ------------------------------------------- *)
(* Decimal:  42                                                     *)
(* Binary:   0b1100                                                 *)
(* Hex:      0xAF                                                   *)
(* Octal:    0o741                                                  *)

IntegerLiteral
    = DecimalLiteral
    | BinaryLiteral
    | HexLiteral
    | OctalLiteral
    ;

DecimalLiteral
    = NonZeroDigit { Digit }
    | "0"
    ;

BinaryLiteral
    = "0" ( "b" | "B" ) BinaryDigit { BinaryDigit }
    ;

HexLiteral
    = "0" ( "x" | "X" ) HexDigit { HexDigit }
    ;

OctalLiteral
    = "0" ( "o" | "O" ) OctalDigit { OctalDigit }
    ;

(* --- Float literals --------------------------------------------- *)
(* Examples: 3.14, 1.0e10, 2.5E-3                                  *)

FloatLiteral
    = Digit { Digit } "." Digit { Digit } [ Exponent ]
    | Digit { Digit } Exponent
    ;

Exponent
    = ( "e" | "E" ) [ "+" | "-" ] Digit { Digit }
    ;

(* --- Bool ------------------------------------------------------- *)

BoolLiteral
    = "true" | "false"
    ;

(* --- Char ------------------------------------------------------- *)
(* Single character in single quotes with backslash escapes.        *)

CharLiteral
    = "'" ( CharChar | EscapeSequence ) "'"
    ;

CharChar
    = (* any character except single-quote, backslash, or newline *)
    ;

(* --- String ----------------------------------------------------- *)
(* Double-quoted string with backslash escapes. No interpolation.   *)

StringLiteral
    = '"' { StringChar | EscapeSequence } '"'
    ;

StringChar
    = (* any character except double-quote, backslash, or newline *)
    ;

(* --- Template literal ------------------------------------------- *)
(* Backtick-delimited string with \${expression} interpolation and   *)
(* backslash escaping. Follows JavaScript template literal syntax.  *)
(*                                                                  *)
(* Example: \`Hello \${name}, you are \${age + 1} years old\`           *)

TemplateLiteral
    = "\`" { TemplateChar | EscapeSequence | TemplateInterpolation } "\`"
    ;

TemplateChar
    = (* any character except backtick, backslash, or \${ *)
    ;

TemplateInterpolation
    = "\${" Expression "}"
    ;

(* --- Null ------------------------------------------------------- *)

NullLiteral
    = "null"
    ;

(* --- Escape sequences ------------------------------------------- *)

EscapeSequence
    = "\\" ( "n" | "t" | "r" | "\\" | "'" | '"' | "\`" | "$" | "0" )
    | "\\" "u" HexDigit HexDigit HexDigit HexDigit
    ;


(* ================================================================ *)
(* 8. LEXICAL ELEMENTS                                              *)
(* ================================================================ *)

Identifier
    = IdentifierStart { IdentifierPart }
    ;

IdentifierStart
    = Letter | "_"
    ;

IdentifierPart
    = Letter | Digit | "_"
    ;

Letter
    = "A" | "B" | "C" | "D" | "E" | "F" | "G" | "H" | "I" | "J"
    | "K" | "L" | "M" | "N" | "O" | "P" | "Q" | "R" | "S" | "T"
    | "U" | "V" | "W" | "X" | "Y" | "Z"
    | "a" | "b" | "c" | "d" | "e" | "f" | "g" | "h" | "i" | "j"
    | "k" | "l" | "m" | "n" | "o" | "p" | "q" | "r" | "s" | "t"
    | "u" | "v" | "w" | "x" | "y" | "z"
    ;

Digit
    = "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9"
    ;

NonZeroDigit
    = "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9"
    ;

BinaryDigit
    = "0" | "1"
    ;

OctalDigit
    = "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7"
    ;

HexDigit
    = Digit
    | "A" | "B" | "C" | "D" | "E" | "F"
    | "a" | "b" | "c" | "d" | "e" | "f"
    ;


(* ================================================================ *)
(* 9. COMMENTS (informational — typically handled by the lexer)     *)
(* ================================================================ *)
(*                                                                  *)
(* SingleLineComment = "//" { any character except newline } newline *)
(*                                                                  *)
(* MultiLineComment  = "/*" { any character } "*/"                  *)
(*                                                                  *)
(* Comments are stripped during lexical analysis and do not appear   *)
(* in the parse tree.                                               *)
(* ================================================================ *)


(* ================================================================ *)
(* 10. RESERVED WORDS                                               *)
(* ================================================================ *)
(*                                                                  *)
(* The following identifiers are reserved and cannot be used as     *)
(* variable or function names:                                      *)
(*                                                                  *)
(*   package  import  export  public  private  final  var  start    *)
(*   boot   return   exit    throw   null    true     false  void   *)
(*   static   internal  as  union  manual  arr  asm                 *)
(*   malloc  calloc  realloc  free                                  *)
(*                                                                  *)
(*   int8  int16  int32  int64  uint8  uint16  uint32  uint64       *)
(*   float32  float64  bool  char  string                           *)
(*   byte  sbyte  short  int  long  ulong  uint  float  double      *)
(*                                                                  *)
(* ================================================================ *)
`;
