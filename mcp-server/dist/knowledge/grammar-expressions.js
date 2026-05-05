"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.GRAMMAR_EXPRESSIONS = void 0;
exports.GRAMMAR_EXPRESSIONS = `
(* ================================================================ *)
(* 5. EXPRESSIONS — Precedence (lowest to highest)                  *)
(*                                                                  *)
(*  1. Assignment          =  +=  -=  *=  /=  %=  &=  |=  ^=       *)
(*                          <<=  >>=                                *)
(*  2. Ternary             ?  :                                     *)
(*  3. Logical OR          ||                                       *)
(*  4. Logical AND         &&                                       *)
(*  5. Bitwise OR          |                                        *)
(*  6. Bitwise NAND        ~&                                       *)
(*  7. Bitwise XOR         ^                                        *)
(*  8. Bitwise XNOR        ~^                                      *)
(*  9. Bitwise AND         &                                        *)
(* 10. Equality            ==  !=                                   *)
(* 11. Relational          <  >  <=  >=                             *)
(* 12. Shift               <<  >>                                   *)
(* 13. Additive            +  -                                     *)
(* 14. Multiplicative      *  /  %                                  *)
(* 15. Unary prefix        !  ~  -  +  spawn  ++  --                *)
(* 16. Postfix             ()  []  .  ++  --                         *)
(* 17. Primary             literals, identifiers, _, parens,        *)
(*                          lambdas, casts                          *)
(* ================================================================ *)

Expression
    = LambdaExpression
    | AssignmentExpression
    ;

(* --- Lambda ---------------------------------------------------- *)
(* Single-expression:  (int32 a, int32 b) -> a + b                 *)
(* Block:              (int32 a) -> { return a + 1; }              *)
(* Zero-arg:           () -> 42                                     *)

LambdaExpression
    = "(" [ ParameterList ] ")" "->" LambdaBody
    ;

LambdaBody
    = Block
    | AssignmentExpression
    ;

ParameterList
    = Parameter { "," Parameter }
    ;

Parameter
    = Type [ "..." ] Identifier [ "=" Expression ]             (* varargs \u2014 must be last; optional default value *)
    ;

(* --- Assignment (right-associative) ----------------------------- *)

AssignmentExpression
    = TernaryExpression [ AssignmentOperator AssignmentExpression ]
    ;

AssignmentOperator
    = "=" | "+=" | "-=" | "*=" | "/=" | "%="
    | "&=" | "|=" | "^=" | "<<=" | ">>="
    ;

(* --- Ternary (right-associative) -------------------------------- *)

TernaryExpression
    = LogicalOrExpression [ "?" Expression ":" TernaryExpression ]
    ;

(* --- Logical OR ------------------------------------------------- *)

LogicalOrExpression
    = LogicalAndExpression { "||" LogicalAndExpression }
    ;

(* --- Logical AND ------------------------------------------------ *)

LogicalAndExpression
    = BitwiseOrExpression { "&&" BitwiseOrExpression }
    ;

(* --- Bitwise OR ------------------------------------------------- *)

BitwiseOrExpression
    = BitwiseNandExpression { "|" BitwiseNandExpression }
    ;

(* --- Bitwise NAND ----------------------------------------------- *)

BitwiseNandExpression
    = BitwiseXorExpression { "~&" BitwiseXorExpression }
    ;

(* --- Bitwise XOR ------------------------------------------------ *)

BitwiseXorExpression
    = BitwiseXnorExpression { "^" BitwiseXnorExpression }
    ;

(* --- Bitwise XNOR ---------------------------------------------- *)

BitwiseXnorExpression
    = BitwiseAndExpression { "~^" BitwiseAndExpression }
    ;

(* --- Bitwise AND ------------------------------------------------ *)

BitwiseAndExpression
    = EqualityExpression { "&" EqualityExpression }
    ;

(* --- Equality --------------------------------------------------- *)

EqualityExpression
    = RelationalExpression { ( "==" | "!=" ) RelationalExpression }
    ;

(* --- Relational ------------------------------------------------- *)

RelationalExpression
    = ShiftExpression { ( "<" | ">" | "<=" | ">=" ) ShiftExpression }
    ;

(* --- Shift ------------------------------------------------------ *)

ShiftExpression
    = AdditiveExpression { ( "<<" | ">>" ) AdditiveExpression }
    ;

(* --- Additive --------------------------------------------------- *)

AdditiveExpression
    = MultiplicativeExpression { ( "+" | "-" ) MultiplicativeExpression }
    ;

(* --- Multiplicative --------------------------------------------- *)

MultiplicativeExpression
    = UnaryExpression { ( "*" | "/" | "%" ) UnaryExpression }
    ;

(* --- Unary prefix ----------------------------------------------- *)

SpawnExpr
    = "spawn" UnaryExpression                                   (* void -> Thread; value -> Future<T> *)
    ;

UnaryExpression
    = ( "!" | "~" | "-" | "+" ) UnaryExpression
    | SpawnExpr
    | "++" UnaryExpression                                     (* prefix increment *)
    | "--" UnaryExpression                                     (* prefix decrement *)
    | PostfixExpression
    ;

(* --- Postfix ---------------------------------------------------- *)
(* Function call, array indexing, and member access.                 *)

PostfixExpression
    = PrimaryExpression { PostfixOp }
    ;

PostfixOp
    = "(" [ ArgumentList ] ")"       (* function call      *)
    | "[" Expression "]"              (* array index        *)
    | "." Identifier                  (* member access      *)
    | "++"                            (* postfix increment  *)
    | "--"                            (* postfix decrement  *)
    ;

ArgumentList
    = Expression { "," Expression }
    ;

`;
//# sourceMappingURL=grammar-expressions.js.map