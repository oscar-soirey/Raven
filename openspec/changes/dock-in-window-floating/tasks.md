## 1. Review
- [x] 1.1 Review proposal, design and tasks with the user before implementation (approved 2026-11-13).

## 2. Implementation
- [x] 2.1 Add public mode enum/setter and retain native-window behavior by default.
- [x] 2.2 Implement floating group arrangement, painting, hit testing, z order and title dragging.
- [x] 2.3 Reuse drop zones to redock floating panels, and update panel window/shown/open queries.
- [x] 2.4 Extend layout serialization/parser compatibly and validate failed loads without partial mutation.
- [x] 2.5 Add tests for floating without extra windows, movement/clamping, redocking, close/hide/destroy and layout round trips.

## 3. Verification and contribution
- [x] 3.1 Regenerate bindings and update public API/behavior documentation.
- [x] 3.2 Run focused regressions, applicable CTest suite and binding freshness; compare failures with main.
- [x] 3.3 Review all changes against Raven architecture and contribution rules.
- [ ] 3.4 Create an independent PR referencing and closing #92, documenting verification and platform limitations.
