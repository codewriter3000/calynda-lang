"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.DIAGNOSTIC_DOCS = void 0;
exports.findDiagnosticDocs = findDiagnosticDocs;
exports.formatDiagnosticDoc = formatDiagnosticDoc;
exports.DIAGNOSTIC_DOCS = [
    {
        key: 'spawn-mutable-capture',
        title: 'Possible Data Race On spawn Capture',
        kind: 'warning',
        aliases: ['spawn warning', 'spawn race warning', 'mutable capture warning', 'data race warning'],
        messagePatterns: ['possible data race', 'captures mutable symbol'],
        summary: 'A spawned callable captured mutable shared state without an explicitly safe sharing model.',
        when: [
            'A callable launched with `spawn` captures a mutable symbol.',
            'The captured symbol is not `final`, not `thread_local`, and not wrapped in `Atomic<T>`.',
        ],
        why: [
            'The spawned callable may run concurrently with the enclosing code.',
            'Unsynchronized shared mutation risks real races and nondeterministic behavior.',
            'Even when the bug does not reproduce immediately, the code is harder to reason about and optimize safely.',
        ],
        prefer: [
            'Use `final` for immutable shared values.',
            'Use `thread_local` when the state should not be shared between threads.',
            'Use `Atomic<T>` when cross-thread mutation is intentional.',
        ],
        relatedFlags: ['--strict-race-check'],
    },
    {
        key: 'external-call-dispatch',
        title: 'Dynamic Callable Dispatch Through external Value',
        kind: 'performance warning',
        aliases: ['dynamic callable dispatch', 'external callable dispatch', 'dispatch warning', 'external call warning'],
        messagePatterns: ['dynamic callable dispatch', 'runtime helper dispatch'],
        summary: 'A call goes through an `external`-typed value, so the compiler cannot lower it as a direct typed call.',
        when: [
            'A call target has an `external`/`var`-typed callable shape instead of a statically typed callable.',
            'The compiler must route the call through runtime helper dispatch.',
        ],
        why: [
            'Helper-based dispatch adds overhead compared with direct typed calls.',
            'It blocks straightforward inlining and other call-site optimizations.',
            'On size-sensitive native targets, helper dispatch can pull in more runtime support than a direct call.',
        ],
        prefer: [
            'Use statically typed callable parameters or bindings.',
            'Narrow `external` values to a concrete callable type before calling them.',
        ],
    },
    {
        key: 'template-literal-runtime-build',
        title: 'Template Literal Runtime String Build',
        kind: 'performance advisory',
        aliases: ['template advisory', 'template literal advisory', 'string build advisory'],
        messagePatterns: [
            'complex template literals may still build strings',
            'boot manual or size focused code build strings',
            'build strings through runtime helpers',
            'may allocate',
        ],
        summary: 'Complex hosted templates, plus templates used in boot/manual/size-focused code, still lower through runtime string-building helpers and may allocate.',
        when: [
            'A complex hosted template literal is type-checked while performance advisories are enabled.',
            'An interpolated template literal appears in boot, manual, or size-focused code.',
            'Interpolated pieces still need runtime helper code instead of the cheapest hosted fast path.',
        ],
        why: [
            'The lowering path may allocate and copy data.',
            'The cost is easy to miss because the syntax is compact and convenient.',
            'In hot code or size-sensitive builds, repeated helper-based string construction can dominate the cost of otherwise simple logic.',
        ],
        prefer: [
            'Reuse cached strings when the text is stable.',
            'Prefer already-plain string values or simple `${value}` forms on hosted paths.',
            'Use explicit casts or plain strings in boot/manual/size-focused code when size or predictability matters.',
        ],
    },
    {
        key: 'runtime-omitted-array-extent',
        title: 'Runtime-Derived Omitted Array Extent',
        kind: 'performance advisory',
        aliases: ['omitted array extent advisory', 'runtime array extent advisory', 'array length advisory'],
        messagePatterns: ['omits an array length', 'length will be determined at runtime'],
        summary: 'A declared array type omitted one or more extents, and the compiler still could not infer those extents statically.',
        when: [
            'A binding, declared lambda return, or parameter default value omits an array extent.',
            'After merging all currently known source shape information, at least one omitted extent is still unknown.',
        ],
        why: [
            'The array length remains runtime-derived instead of compile-time known.',
            'The compiler loses exact shape information that can feed later size-focused optimization decisions.',
            'The program remains valid, but it carries less information into later stages than an explicit or provably constant extent.',
        ],
        prefer: [
            'Write the extent explicitly when it is part of the API or storage contract.',
            'Feed the declaration from a statically sized source when possible.',
            'Avoid shape-erasing callable boundaries when exact extents matter downstream.',
        ],
    },
];
function normalize(text) {
    return text.toLowerCase().replace(/[^a-z0-9]+/g, ' ').trim();
}
function findDiagnosticDocs(query) {
    const normalizedQuery = normalize(query);
    if (!normalizedQuery) {
        return [];
    }
    return exports.DIAGNOSTIC_DOCS.filter(doc => {
        if (normalize(doc.title) === normalizedQuery || normalize(doc.key) === normalizedQuery) {
            return true;
        }
        return doc.aliases.some(alias => normalizedQuery.includes(normalize(alias))) ||
            doc.messagePatterns.some(pattern => normalizedQuery.includes(normalize(pattern)));
    });
}
function formatDiagnosticDoc(doc) {
    const lines = [
        `## ${doc.title}`,
        `- Kind: ${doc.kind}`,
        `- Summary: ${doc.summary}`,
        '',
        '### Message Cues',
        ...doc.messagePatterns.map(pattern => `- ${pattern}`),
        '',
        '### When It Appears',
        ...doc.when.map(item => `- ${item}`),
        '',
        '### Why It Is Discouraged',
        ...doc.why.map(item => `- ${item}`),
        '',
        '### Preferred Alternatives',
        ...doc.prefer.map(item => `- ${item}`),
    ];
    if (doc.relatedFlags && doc.relatedFlags.length > 0) {
        lines.push('', '### Related Flags', ...doc.relatedFlags.map(flag => `- ${flag}`));
    }
    return lines.join('\n');
}
//# sourceMappingURL=diagnostics.js.map