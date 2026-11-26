# Garage chooser presentation integrity

Follow-up to the exact native Garage candidate PR #470. Do not replace
that frozen candidate or reuse its evidence for this new source SHA.

The complete chooser geometry must be valid and equal to its last
presentation before mouse, keyboard or final dispatch can select a
style. The panel background blocks underlying clicks, including padding;
corrupt active geometry blocks all pointer dispatch. The existing
GameMode ownership, currency, save and rollback policy is unchanged.

## Test plan
- Exercise all paint, stripe and rim layers at supported desktop sizes.
- Resize between draw and confirm; verify no old choice is applied.
- Confirm a newly rendered choice and verify correct vehicle identity.
- Inject overlapping choices or malformed close geometry in portable tests.
- Verify modal padding never activates a hidden control and ordinary
  navigation outside the chooser stays available.
- Verify keyboard cancel works before the first draw and after resize.
- Verify future-save and focus-loss restrictions remain intact.
- Run exact-head portable checks and separately qualify real UE5.8/UHT,
  cooked Win64 rendering and keyboard/controller behavior.

Source-only work: no native build, device result or published release claim.
