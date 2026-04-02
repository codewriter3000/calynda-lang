[X] Build a recursive-descent parser for the EBNF grammar, with precedence-based expression parsing and decent error reporting.
[X] Add AST pretty-printing or dumping so parser output can be inspected and tested quickly.
[X] Implement symbol tables and scope management for packages, imports, top-level bindings, parameters, and locals.
    [X] Add source-location diagnostics for unresolved identifiers and duplicate-symbol errors.
        [X] Thread source tokens or source spans from the parser into AST declarations and identifier expressions.
        [X] Store source spans on symbols and unresolved-name records so semantic errors can point at both use and declaration sites.
        [X] Standardize semantic error formatting to match parser errors with line/column context.
    [X] Build type checking on top of the resolved symbol information.
        [X] Define a semantic type model that can represent primitives, arrays, void, and any future callable/function types.
        [X] Infer expression types bottom-up and attach them to AST nodes or semantic side tables.
        [X] Check operator compatibility for unary, binary, ternary, cast, call, index, and member expressions.
        [X] Validate implicit vs explicit conversions and reject lossy or illegal assignments.
    [X] Add a semantic dump or inspection tool if you want to debug scopes the same way you already debug the AST.
        [X] Dump scopes hierarchically with declared symbols, kinds, and inferred/declared types.
        [X] Include identifier resolution edges so unresolved names and shadowing are easy to inspect.
        [X] Add focused tests so the dump remains stable enough for quick regression checks.
[X] Add semantic passes: type resolution, type checking, return/exit rules, assignment checks, and validation of start.
    [X] Type resolution.
        [X] Resolve declared types on bindings, parameters, locals, casts, and array dimensions.
        [X] Reject invalid uses of `void` and malformed array declarations.
    [X] Type checking.
        [X] Compute expression result types for literals, identifiers, lambdas, calls, indexing, members, and arrays.
        [X] Enforce operand compatibility and result typing rules for all operators.
    [X] Return and exit rules.
        [X] Validate which contexts permit `return` and `exit`.
        [X] Ensure expression-bodied and block-bodied lambdas satisfy their expected return behavior.
    [X] Assignment checks.
        [X] Ensure assignment targets are assignable l-values.
        [X] Reject writes to `final` bindings and incompatible compound assignments.
    [X] Start validation.
        [X] Require exactly one `start` entry point.
        [X] Validate the `start` parameter and return conventions that the language intends to support.
        [X] Reject top-level programs that are missing or ambiguously define the entry point.
[ ] Define the multi-level IR pipeline: HIR, MIR, and LIR.
    [X] HIR foundations.
        [X] Define a typed/desugared HIR for programs, declarations, blocks, statements, and expressions.
        [X] Normalize expression-bodied lambdas and `start` bodies into block form during lowering.
        [X] Add an HIR dump and regression tests so the first lowering contract is inspectable.
    [ ] HIR expansion.
        [ ] Decide which remaining syntax sugar is eliminated in HIR versus deferred to MIR.
            [X] Normalize `exit` into HIR `return` so control-flow sugar does not reach MIR.
            [ ] Decide whether templates, ternaries, and other high-level constructs stay in HIR or lower earlier.
        [X] Attach enough symbol/type identity to avoid re-deriving semantic facts in later passes.
            [X] Preserve resolved symbol identity on lowered references, bindings, and parameters.
            [X] Preserve owned callable signatures in HIR so later passes do not depend on AST parameter lists.
        [X] Cover more language features and edge cases in HIR lowering tests.
            [X] Add regression coverage for external callable bindings and exit normalization.
            [X] Add broader lowering coverage for templates, casts, array literals, assignments, and nested lambdas.
    [ ] MIR.
        [X] Define machine-independent control-flow graphs, values, temporaries, and callable units.
        [ ] Lower HIR blocks/statements/expressions into MIR basic blocks.
            [X] Lower the first straight-line slice: literals, arithmetic, locals, calls, casts, and returns.
            [X] Lower control-flow constructs such as ternaries and branches into multiple basic blocks.
                [X] Lower short-circuit logical operators into explicit branch/join blocks.
            [X] Lower the remaining high-level HIR forms that are still intentionally preserved.
                [X] Lower unary expressions, assignments, index/member access, array literals, and templates into explicit MIR instructions.
                [X] Design closure-aware MIR lowering for nested lambdas and callable local bindings.
            [X] Lower the remaining unsupported MIR boundaries such as throw statements and top-level value initialization.
                [X] Lower top-level non-lambda bindings into a synthetic module-init unit so globals are initialized explicitly.
                [X] Represent throw as an explicit MIR terminator instead of rejecting it during lowering.
        [X] Add an MIR printer or dumper for debugging and golden tests.
        [X] Extend the minimal slice from straight-line code to broader branch-aware MIR.
            [X] Lower ternaries into explicit branch/join blocks with synthetic merge locals.
            [X] Lower the remaining control-flow-sensitive constructs that still need explicit branching.
    [X] LIR.
        [X] Define target-aware instructions, virtual registers, stack slots, and calling convention operations.
        [X] Lower MIR into LIR after the machine-independent optimization boundary is clear.
        [X] Add LIR dumps plus backend-focused tests around register allocation and instruction selection later.
            [X] Add an LIR dump and regression coverage for the current MIR surface.
            [X] Define the first instruction-selection target as x86_64 SysV ELF with SysV integer argument registers, rax returns, and a hidden closure-environment register.
            [X] Decide which current LIR ops lower directly versus through runtime helper calls at the first backend boundary.
            [X] Add backend-focused tests around register allocation and instruction selection.
[X] Add a real machine-instruction emission layer after instruction selection and register allocation.
    [X] Reuse the existing direct-vs-runtime boundary instead of re-deciding it in the emitter.
    [X] Emit x86_64 SysV-style instruction streams for direct patterns, helper calls, and block terminators.
    [X] Preserve the currently allocated vreg registers across calls with an explicit first-pass calling-convention strategy.
    [X] Add a real x86_64 assembly writer on top of the machine layer.
    [X] Add a small emit_asm tool that lowers a source file all the way to native assembly text.
[X] Choose execution strategy beyond the current native backend: no AST/IR interpreter path; if a VM path is added later, compile IR into bytecode.
    [X] Keep native x86_64 SysV ELF as the primary execution path for now.
    [X] Treat any future bytecode compiler as a sibling backend rather than a replacement for the native backend.
    [X] Write down the backend split so later runtime work targets one concrete contract per execution path.
[ ] Add runtime pieces only when the language semantics demand them: calling convention, memory management hooks, built-in library boundary, and startup/entry handling.
    [X] Define the first helper-backed runtime ABI surface for closures, callable dispatch, member/index access, arrays, templates, cast-value, and throw.
    [X] Specify the current native calling conventions for `start`, user functions, helper calls, and lambda capture environments.
    [X] Decide the first concrete runtime object model for strings, arrays, closures, packages, extern callables, and template-part packs.
    [X] Define the first built-in package boundary around runtime package objects and extern callable member dispatch.
    [X] Implement the first runtime helper symbols behind that ABI and cover them with focused runtime tests.
    [X] Add startup/entry glue only after the entry-point rules and execution model are fixed.
[X] Add a sibling bytecode backend directly from MIR instead of introducing any interpreter stage.
    [X] Define an initial portable bytecode ISA with units, blocks, constants, and explicit opcodes.
    [X] Lower MIR into the bytecode program format as a second backend path.
    [X] Add a bytecode dump tool and backend-focused regression coverage.