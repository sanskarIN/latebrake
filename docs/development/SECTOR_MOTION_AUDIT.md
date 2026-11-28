# Local sector motion audit

Non-shipping builds emit one LATEBRAKE_SECTOR_AUDIT record after every
authoritative accepted player gate. Available rows include active time,
fees, physical sample counts, coverage, interruption counts and bounded
motion integrals. Unavailable rows deliberately omit sampled metrics.

Run `python scripts/AnalyzeSectorMotionAudit.py game.log --require-finish`.
The Windows receipt reviewer also includes this section automatically when
present; add `--require-sector-motion` to reject legacy logs without it.

The parser requires a matching chronological accepted-gate receipt for
every sector record, checks cumulative clock and fee reconciliation,
observed-duration bounds and quality classification, and never treats
the standing-start sector as a rolling reference. It refuses missing,
duplicate or malformed observations. A completed report cannot establish
native Unreal compilation, authentic gameplay, tire contact, or readiness.

Portable tests use synthetic text. For native verification, play a full
race including a restart, pause and recovery; preserve the original runtime
log and package receipt, review coverage and compare with visible results.
