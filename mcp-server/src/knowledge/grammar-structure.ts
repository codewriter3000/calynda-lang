export const GRAMMAR_STRUCTURE = `
(* ===================================================================== *)
(* Calynda V2 — EBNF Grammar Specification                               *)
(* Cloned from compiler/calynda.ebnf on 2026-04-15                       *)
(*                                                                        *)
(* This file is the canonical V3 grammar.                                *)
(* ===================================================================== *)


(* ================================================================ *)
(* 1. PROGRAM STRUCTURE                                             *)
(* ================================================================ *)

Program
    = [ PackageDecl ]
      { ImportDecl }
      { TopLevelDecl }
    ;

PackageDecl
    = "package" QualifiedName ";"
    ;

ImportDecl
    = "import" QualifiedName ";"                              (* plain import *)
    | "import" QualifiedName "as" Identifier ";"              (* module alias *)
    | "import" QualifiedName "." "*" ";"                      (* wildcard — direct exports only *)
    | "import" QualifiedName "." "{" ImportNameList "}" ";"   (* selective import *)
    ;

ImportNameList
    = Identifier { "," Identifier }
    ;

QualifiedName
    = Identifier { "." Identifier }
    ;


(* ================================================================ *)
(* 2. TOP-LEVEL DECLARATIONS                                        *)
(* ================================================================ *)

TopLevelDecl
    = StartDecl
    | BootDecl
    | AsmDecl
    | BindingDecl
    | UnionDecl
    ;

(* Entry point — implicitly returns int32 (exit code).              *)
StartDecl
    = "start" "(" ParameterList ")" "->" ( Block | Expression ) ";"
    ;

(* Bare-metal entry point — bypasses the Calynda runtime and emits  *)
(* a raw _start symbol. Cannot coexist with start() in the same     *)
(* compilation unit.                                                *)
BootDecl
    = "boot" "(" ")" "->" ( Block | Expression ) ";"
    ;

(* Inline assembly declaration — the assembly body is passed        *)
(* through to the assembler unchanged. Parameters follow normal     *)
(* Calynda type syntax and map to ABI registers.                    *)
AsmDecl
    = { Modifier } Type Identifier "=" "asm" "(" [ ParameterList ] ")" "->" "{" AsmBody "}" ";"
    ;

AsmBody
    = (* raw assembly text passed through to assembler unchanged *)
    ;

(* Variable / function binding with optional modifiers.             *)
(* Examples:                                                        *)
(*   int32 x = 5;                                                   *)
(*   public int8 add = (int32 a, int32 b) -> a + b;                *)
(*   private final float64 PI = 3.14159;                            *)
(*   var y = 42;                                                    *)
BindingDecl
    = { Modifier } ( Type | "var" ) Identifier "=" Expression ";"
    ;

Modifier
    = "export"
    | "public"
    | "private"
    | "final"
    | "static"
    | "internal"
    ;

(* Tagged union declaration with optional generic parameters.       *)
(* Examples:                                                        *)
(*   union Option<T> { Some(T), None };                             *)
(*   export union Result<T, E> { Ok(T), Err(E) };                  *)
UnionDecl
    = { Modifier } "union" Identifier [ GenericParams ] "{" UnionVariantList "}" ";"
    ;

GenericParams
    = "<" GenericParamList ">"
    ;

GenericParamList
    = Identifier { "," Identifier }
    ;

UnionVariantList
    = UnionVariant { "," UnionVariant }
    ;

UnionVariant
    = Identifier [ "(" Type ")" ]
    ;


(* ================================================================ *)
(* 3. TYPES                                                         *)
(* ================================================================ *)

(* Note: GenericArgs are syntactically optional on any type, but    *)
(* semantic analysis rejects meaningless combinations such as       *)
(* int32<T>. In practice, generics apply to user-defined types      *)
(* (resolved via Identifier) and the built-in arr<T> form.          *)
Type
    = PrimitiveType [ GenericArgs ] { ArrayDimension }
    | Identifier [ GenericArgs ] { ArrayDimension }            (* named / user-defined type *)
    | "arr" GenericArgs                                        (* heterogeneous array *)
    | "void"
    ;

PrimitiveType
    = "int8"   | "int16"  | "int32"  | "int64"
    | "uint8"  | "uint16" | "uint32" | "uint64"
    | "float32" | "float64"
    | "bool"
    | "char"
    | "string"
    (* Java-style aliases *)
    | "byte" | "sbyte" | "short" | "int" | "long" | "ulong"
    | "uint" | "float" | "double"
    ;

(* Unsized [] or sized [IntegerLiteral] array dimensions.           *)
ArrayDimension
    = "[" [ IntegerLiteral ] "]"
    ;

(* Generic type arguments — e.g. <int32>, <string, ?>.             *)
GenericArgs
    = "<" GenericArgList ">"
    ;

GenericArgList
    = GenericArg { "," GenericArg }
    ;

GenericArg
    = "?"                                                      (* wildcard *)
    | Type
    ;


(* ================================================================ *)
(* 4. STATEMENTS & BLOCKS                                           *)
(* ================================================================ *)

Block
    = "{" { Statement } "}"
    ;

Statement
    = LocalBindingStatement
    | ReturnStatement
    | ExitStatement
    | ThrowStatement
    | ManualStatement
    | ExpressionStatement
    ;

(* Local variable declaration — same syntax as top-level binding    *)
(* but without visibility modifiers.  "final" and "internal" are   *)
(* allowed; "internal" restricts the local to nested-lambda use.   *)
LocalBindingStatement
    = [ "internal" ] [ "final" ] ( Type | "var" ) Identifier "=" Expression ";"
    ;

ReturnStatement
    = "return" [ Expression ] ";"
    ;

(* exit; is equivalent to return null; in void-typed lambdas.       *)
ExitStatement
    = "exit" ";"
    ;

ThrowStatement
    = "throw" Expression ";"
    ;

ExpressionStatement
    = Expression ";"
    ;

(* Manual memory boundary (experimental) — opens an unsafe scope   *)
(* for direct memory management. malloc, calloc, realloc, and free  *)
(* are available as built-in expressions inside the block.          *)
(* All arguments must be integral types. malloc/calloc/realloc      *)
(* return a raw int64 address with no type safety.                  *)
(* Example:                                                         *)
(*   manual {                                                       *)
(*       var addr = malloc(1024);                                   *)
(*       free(addr);                                                *)
(*   };                                                             *)
ManualStatement
    = "manual" "{" { Statement } "}" ";"
    ;

`;
