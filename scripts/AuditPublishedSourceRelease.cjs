'use strict';

// Read-only source-release metadata policy; intentionally no GitHub client,
// token, filesystem, tag creation, or publishing capability in this module.
// A passing metadata audit is NOT Unreal 5.8, UHT, Win64, or playable-game QA.
const SHA40 = /^[0-9a-f]{40}$/;
const OFFICIAL_SOURCE_TAG = /^v0\.5\.\d+$/;

// This is an immutable, explicitly pinned historical SOURCE-TEST snapshot.
// Its release was created against "main" and its source commit did not have
// docs/releases/v0.5.100-test.1.md. Do not generalize this exception to
// future "-test" tags or relax ordinary source-release provenance.
const LEGACY_TEST_TAG = 'v0.5.100-test.1';
const LEGACY_TEST_SHA = 'a04db1ccc15ee2e3be6771d7e4a5159dcadb2384';
const LEGACY_TEST_NAME = 'LateBrake v0.5.100-test.1 — Source Testing';
const LEGACY_TEST_BODY = [
  '## LateBrake v0.5.100-test.1 — Source Testing',
  '',
  '**Experimental SOURCE-ONLY pre-release — not a playable game build.**',
  '',
  'This development snapshot provides the current LateBrake mainline source for review, testing and development.',
  '',
  'The protected-main source has passed portable C++20 tests on Windows, Ubuntu and macOS, along with Python/editor source checks and release-policy checks.',
  '',
  '**Not included:** Unmerged gameplay work from PR #377 and PR #391, Windows game executable, installer or verified Unreal Engine 5.8 build.',
].join('\n');

const REQUIRED_SOURCE_WARNINGS = Object.freeze([
  'SOURCE-ONLY',
  'NOT a playable game build',
  'No compiled Win64 EXE',
  'Android APK/AAB',
  'not installers',
  'zero executable binaries',
]);

function requireState(condition, message) {
  if (!condition) throw new Error(message);
}

function classifySourceTag(tag) {
  if (typeof tag === 'string' && OFFICIAL_SOURCE_TAG.test(tag)) {
    return {kind: 'official-source', notesPath: 'docs/releases/' + tag + '.md'};
  }
  if (tag === LEGACY_TEST_TAG) {
    return {kind: 'legacy-source-test', notesPath: null};
  }
  throw new Error('Only v0.5.N source tags and the pinned v0.5.100-test.1 snapshot are accepted.');
}

function verifyPublishedSourceRelease({tag, reference, release, sourceNotes}) {
  const classification = classifySourceTag(tag);
  const ref = reference?.object;
  requireState(ref?.type === 'commit' && SHA40.test(ref.sha),
    'Source tag must point directly to a 40-character commit SHA.');
  requireState(release?.tag_name === tag && release.draft === false &&
    release.prerelease === true && typeof release.published_at === 'string' &&
    release.published_at.length > 0 && Array.isArray(release.assets) &&
    release.assets.length === 0 && typeof release.body === 'string',
    'Release must be published, non-draft, pre-release, and have zero attachments.');

  if (classification.kind === 'official-source') {
    requireState(release.target_commitish === ref.sha,
      'Official source release target must be the exact immutable source tag SHA.');
    requireState(typeof sourceNotes === 'string' && release.body === sourceNotes,
      'Official release notes must match exact source-tagged UTF-8 file bytes.');
    for (const warning of REQUIRED_SOURCE_WARNINGS) {
      requireState(sourceNotes.includes(warning),
        'Official source-only warning missing: ' + warning);
    }
  } else {
    requireState(ref.sha === LEGACY_TEST_SHA,
      'Historical test source tag moved away from its reviewed main SHA.');
    // Historical release metadata stores the branch name, NOT the exact
    // SHA; the separately pinned direct tag is the authoritative snapshot.
    requireState(release.target_commitish === 'main' &&
      release.name === LEGACY_TEST_NAME &&
      release.body.replace(/\r\n?/g, '\n').trimEnd() === LEGACY_TEST_BODY,
      'Historical test release identity, title or exact canonical body changed.');
    requireState(sourceNotes == null,
      'Historical source-test tag must not pretend to have tagged release notes.');
  }
  return {tag, sha: ref.sha, kind: classification.kind};
}

module.exports = {
  LEGACY_TEST_TAG,
  LEGACY_TEST_SHA,
  LEGACY_TEST_NAME,
  LEGACY_TEST_BODY,
  classifySourceTag,
  verifyPublishedSourceRelease,
};
