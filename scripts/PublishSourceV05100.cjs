'use strict';

// Owner-only, one-version, exact-protected-main SOURCE-ONLY publisher.
// Default readiness is the existing independently tested read-only gate.
// Does not build, attach, simulate or certify playable Unreal binaries.
const gate = require('./SourceReleaseV05100Readiness.cjs');
const APPROVAL_PREFIX = 'I approve v0.5.100 SOURCE-ONLY at ';
const SHA40 = /^[0-9a-f]{40}$/;

function requireExactAuthorization(context) {
  const candidateSha = context?.payload?.inputs?.candidate_sha;
  const approval = context?.payload?.inputs?.approval;
  if (context?.repo?.owner !== 'sanskarIN' ||
      context.repo.repo !== 'latebrake' ||
      context.actor !== 'sanskarIN' ||
      context.eventName !== 'workflow_dispatch' ||
      context.ref !== 'refs/heads/main' ||
      !SHA40.test(context.sha || '') ||
      !SHA40.test(candidateSha || '') ||
      candidateSha !== context.sha ||
      approval !== APPROVAL_PREFIX + candidateSha) {
    throw new Error(
      'Explicit owner-only exact-main SHA and source-only approval required');
  }
  return candidateSha;
}

async function publish({github, context, notes}) {
  const candidateSha = requireExactAuthorization(context);
  const args = {github, context, candidateSha, notes};
  // The default gate checks: protected main identity, merged PR #377
  // ancestry, exact repo file bytes, note warnings, 3-platform checks,
  // immutable predecessor, existing tag/release conflicts. It may throw.
  let audit = await gate.inspectReadiness(args);
  if (audit.writesPerformed !== 0 ||
      audit.nativeUnrealQualified !== false ||
      audit.playableWindowsQualified !== false) {
    throw new Error('Unexpected source-only readiness semantics');
  }
  const {owner, repo} = context.repo;
  const rest = github.rest;
  if (!audit.existingTag) {
    // Never overwrite/move a preexisting v0.5.100 tag.
    const tagged = (await rest.git.createRef({
      owner, repo, ref:'refs/tags/' + gate.TAG, sha:candidateSha,
    })).data;
    if (tagged?.object?.type !== 'commit' ||
        tagged.object.sha !== candidateSha) {
      throw new Error('Created source-only tag is not exact commit identity');
    }
  }
  // Re-run the complete gate after tag creation (or resumed existing tag).
  audit = await gate.inspectReadiness(args);
  if (!audit.existingTag) {
    throw new Error('Exact source-only tag missing after creation');
  }
  if (!audit.existingRelease) {
    await rest.repos.createRelease({
      owner, repo,
      tag_name:gate.TAG, target_commitish:candidateSha,
      name:gate.NAME, body:notes,
      prerelease:true, draft:false,
      generate_release_notes:false, make_latest:'false',
    });
  }
  // Final readback checks exact source, bytes, prerelease and ZERO assets.
  audit = await gate.inspectReadiness(args);
  if (!audit.existingTag || !audit.existingRelease ||
      audit.candidateSha !== candidateSha) {
    throw new Error('Source-only post-publication readback is incomplete');
  }
  const released = (await rest.repos.getReleaseByTag({
    owner, repo, tag:gate.TAG,
  })).data;
  if (released?.tag_name !== gate.TAG ||
      released.target_commitish !== candidateSha ||
      released.name !== gate.NAME ||
      released.body !== notes ||
      released.prerelease !== true ||
      released.draft !== false ||
      !Array.isArray(released.assets) ||
      released.assets.length !== 0) {
    throw new Error('Source-only release metadata changed after readback');
  }
  return released;
}

module.exports = {APPROVAL_PREFIX, requireExactAuthorization, publish};
