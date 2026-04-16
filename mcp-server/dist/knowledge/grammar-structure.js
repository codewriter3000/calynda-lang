"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.GRAMMAR_STRUCTURE = void 0;
exports.GRAMMAR_STRUCTURE = `
(* ===================================================================== *)
(* Calynda — EBNF Grammar Snapshot                                        *)
(* Snapshot version 1.0.0-alpha.2                                         *)
(* Cloned from compiler/calynda.ebnf on 2026-04-16                        *)
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
    | TypeAliasDecl
    | UnionDecl
    | LayoutDecl
    ;

(* Entry point — implicitly returns int32 (exit code).              *)
StartDecl
    = "start" "(" ParameterList ")" "->" ( Block | Expression ) ";"
    ;

(* Bare-metal entry point — emits a freestanding _start symbol and  *)
(* does not promise Linux-only exit behaviour. Cannot coexist with  *)
(* start() in the same compilation unit.                            *)
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

TypeAliasDecl
    = { Modifier } "type" Identifier "=" Type ";"
    ;

Modifier
    = "export"
    | "public"
    | "private"
    | "final"
    | "static"
    | "internal"
    | "thread_local"
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

(* Memory layout declaration — defines a named struct-like type for *)
(* use with ptr<T>, offset, deref, and store operations.            *)
(* Field types must be primitive in alpha.2.                        *)
LayoutDecl
    = "layout" Identifier "{" { LayoutField } "}" ";"
    ;

LayoutField
    = Type Identifier ";"
    ;


(* ================================================================ *)
(* 3. TYPES                                                         *)
(* ================================================================ *)

(* Note: GenericArgs are syntactically optional on any type, but    *)
(* semantic analysis rejects meaningless combinations such as       *)
(* int32<T>. In practice, generics apply to user-defined types      *)
(* (resolved via Identifier) plus the built-in arr<?> and ptr<T>    *)
(* forms.                                                           *)
Type
    = PrimitiveType [ GenericArgs ] { ArrayDimension }
    | "Thread" { ArrayDimension }                               (* semantically-resolved built-in handle *)
    | "Future" GenericArgs { ArrayDimension }                   (* future handle from non-void spawn *)
    | "Mutex" { ArrayDimension }                                (* semantically-resolved built-in handle *)
    | "Atomic" GenericArgs { ArrayDimension }                   (* alpha.2 single-word atomic cell *)
    | Identifier [ GenericArgs ] { ArrayDimension }            (* named / user-defined type *)
    | "arr" GenericArgs                                        (* heterogeneous array *)
    | "ptr" GenericArgs                                        (* typed pointer — manual only *)
    | "ptr" "<" Type "," "checked" ">"                    (* bounds-checked typed pointer *)
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

(* Manual memory boundary — opens a stable unsafe scope for direct  *)
(* for direct memory management. malloc, calloc, realloc, free,    *)
(* stackalloc, cleanup, deref, store, offset, and addr are built-in *)
(* expressions inside the block. cleanup(value, fn) defers fn(value) *)
(* until the enclosing manual scope exits. Optional checked uses    *)
(* bounds-checked runtime helpers. Variables may use ptr<T> types   *)
(* for typed pointer semantics with automatic element sizing.       *)
(* Example:                                                         *)
(*   manual {                                                       *)
(*       ptr<int32> p = malloc(40);                                 *)
(*       store(p, 7);                                               *)
(*       int32 v = deref(p);                                        *)
(*       free(p);                                                   *)
(*   };                                                             *)
ManualStatement
    = "manual" [ "checked" ] "{" { Statement } "}" ";"
    ;

`;
//# sourceMappingURL=grammar-structure.js.map