# Socratic Code Review Response Template (`.agents/skills/c-expert/resources/review_response_template.md`)

This template defines the structured feedback format used during Post-Green Retrospectives and C Systems Code Reviews.

______

```markdown
# 🔍 C Systems Code Review & Socratic Retrospective

## Overview & Operational Contract
- **Target Files Audited**: `src/<file>.c`, `src/<header>.h`
- **Verification Status**: 100% test pass rate, 100.00% line coverage, ASan/UBSan clean, `assert(boot_all_freed())` satisfied.

______________________________________________________________________

### Phase 1: Pointer Ownership & Lifetime Contracts
- **Ownership State**: <Analyze whether returned/passed pointers have unambiguous borrowed vs. owned semantics.>
- **Reference Count Symmetry**: <Audit refcount_inc and refcount_dec balance across all execution branches.>
- **Inductive Step ($\mathcal{I}_k \implies \mathcal{I}_{k+1}$)**: <Confirm mutations preserve ownership balance, pointer validity, and reachability.>
- **Socratic Observation / Inquiry**: <If an improvement exists, pose a probing first-principles question.>

______________________________________________________________________

### Phase 2: Allocation Sizing & Failure Recovery
- **Base Invariant ($\mathcal{I}_0$)**: <Verify slots are zero-initialized to NULL upon allocation.>
- **Allocation Arithmetic**: <Audit sizeof calculations (e.g. sizeof(T) + N * sizeof(elem)) and overflow guards.>
- **Rollback Invariant ($\mathcal{I}_{\text{rollback}}$)**: <Verify multi-stage allocations cleanly unwind all prior steps upon failure.>
- **Socratic Observation / Inquiry**: <If an improvement exists, pose a probing first-principles question.>

______________________________________________________________________

### Phase 3: Runtime & GC Lifecycle Synchronization
- **Tracing Completeness**: <Confirm trace_blacken_object visits every pointer slot in new variants.>
- **Teardown Ordering**: <Verify object_free_payload releases heap buffers before outer header deallocation.>
- **Root Safety**: <Verify objects are tracked/rooted before any subsequent allocation that could trigger GC.>

______________________________________________________________________

### Phase 4: Standards & Portability Compliance
- **ISO C17 Adherence**: <Verify strict portability across GCC, Clang, and MSVC without GNU extensions.>
- **Conscious Post-C99 Features**: <Document and justify any anonymous structs/unions or _Static_assert usage.>
- **Prohibited Constructs**: <Confirm absence of Variable-Length Arrays (VLAs) or undefined type punning.>

______________________________________________________________________

### Phase 5: Maintainability, Idiomatic C & Verification
- **DRY & Boilerplate**: <Identify opportunities to eliminate redundant guards or simplify state machines.>
- **Const-Correctness**: <Verify inspection-only pointers are const-qualified.>
- **Doxygen Documentation**: <Audit @brief, @param, and @return tags across public APIs.>
```
