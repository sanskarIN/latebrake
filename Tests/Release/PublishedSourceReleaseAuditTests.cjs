'use strict';

// All fixtures are offline. No GitHub token, API access, publishing or writes.
const test = require('node:test');
const assert = require('node:assert/strict');
const {
  LEGACY_TEST_TAG, LEGACY_TEST_SHA, LEGACY_TEST_NAME,
  LEGACY_TEST_BODY, REVIEWED_TEST_TAG, REVIEWED_TEST_NAME,
  REVIEWED_TEST_NOTES_PATH, classifySourceTag, verifyPublishedSourceRelease,
} = require('../../scripts/AuditPublishedSourceRelease.cjs');

const MAIN = 'a'.repeat(40);
const NOTES = [
  'v0.5.99 SOURCE-ONLY',
  'NOT a playable game build',
  'No compiled Win64 EXE',
  'Android APK/AAB',
  'not installers',
  'zero executable binaries',
].join('\n');
function legacy() {
  return {
    tag: LEGACY_TEST_TAG,
    reference: {object: {type: 'commit', sha: LEGACY_TEST_SHA}},
    release: {
      tag_name: LEGACY_TEST_TAG,
      target_commitish: 'main',
      draft: false,
      prerelease: true,
      published_at: '2026-11-16T14:16:10Z',
      assets: [],
      name: LEGACY_TEST_NAME,
      body: LEGACY_TEST_BODY.replace(/\n/g, '\r\n'),
    },
  };
}
function official() {
  return {
    tag: 'v0.5.99',
    reference: {object: {type: 'commit', sha: MAIN}},
    sourceNotes: NOTES,
    release: {
      tag_name: 'v0.5.99',
      target_commitish: MAIN,
      draft: false,
      prerelease: true,
      published_at: '2026-11-13T11:03:28Z',
      assets: [],
      name: 'LateBrake source-only v0.5.99',
      body: NOTES,
    },
  };
}
const REVIEWED_NOTES = [
  'LateBrake v0.5.100-test.2 SOURCE-ONLY Pre-release',
  'NOT a playable game build',
  'No compiled Win64 EXE',
  'Android APK/AAB',
  'not installers',
  'zero executable binaries',
  'PR #395',
  'PR #377',
  'PR #391',
  'PR #394',
  'not integrated into real Unreal gameplay',
  '87,792',
].join('\n');
function reviewed() {
  return {
    tag: REVIEWED_TEST_TAG,
    reference: {object: {type:'commit', sha:MAIN}},
    sourceNotes: REVIEWED_NOTES,
    release: {
      tag_name: REVIEWED_TEST_TAG, target_commitish: MAIN,
      draft:false, prerelease:true,
      published_at:'2026-11-17T11:30:00Z',
      name: REVIEWED_TEST_NAME,
      body: REVIEWED_NOTES, assets:[],
    },
  };
}

function denied(name, edit, expression) {
  test(name, () => {
    const fixture = edit.kind === 'official' ? official() :
      edit.kind === 'reviewed' ? reviewed() : legacy();
    edit.apply(fixture);
    assert.throws(() => verifyPublishedSourceRelease(fixture), expression);
  });
}

test('ordinary tags still demand immutable tagged release notes', () => {
  assert.deepEqual(classifySourceTag('v0.5.100'),
    {kind: 'official-source', notesPath: 'docs/releases/v0.5.100.md'});
  const result = verifyPublishedSourceRelease(official());
  assert.equal(result.kind, 'official-source');
  assert.equal(result.sha, MAIN);
});

test('pinned historical test tag verifies its real published branch target', () => {
  assert.deepEqual(classifySourceTag(LEGACY_TEST_TAG),
    {kind: 'legacy-source-test', notesPath: null});
  const result = verifyPublishedSourceRelease(legacy());
  assert.equal(result.kind, 'legacy-source-test');
  assert.equal(result.sha, LEGACY_TEST_SHA);
});

test('historical body supports actual GitHub CRLF normalization only', () => {
  const fixture = legacy();
  fixture.release.body = LEGACY_TEST_BODY;
  assert.equal(verifyPublishedSourceRelease(fixture).sha, LEGACY_TEST_SHA);
});

for (const tag of [
  'v0.5.100-test.3', 'v0.5.101-test.1', 'v0.5.100-rc.1',
  'v0.6.0', 'v0.5.100-test.1+dirty', 'v0.5.100/tag', '',
]) {
  test('unapproved source-preview tag is rejected: ' + tag, () => {
    assert.throws(() => classifySourceTag(tag), /Only v0.5.N/);
  });
}

test('reviewed v0.5.100-test.2 must have tagged exact notes and exact release target', () => {
  assert.deepEqual(classifySourceTag(REVIEWED_TEST_TAG),
    {kind:'reviewed-source-test', notesPath:REVIEWED_TEST_NOTES_PATH});
  const receipt = verifyPublishedSourceRelease(reviewed());
  assert.equal(receipt.kind, 'reviewed-source-test');
  assert.equal(receipt.sha, MAIN);
});

const reviewedChanges = [
  ['moved reviewed source tag', f=>{f.reference.object.sha='b'.repeat(40);},
    /exact immutable source tag SHA/],
  ['annotated reviewed source tag', f=>{f.reference.object.type='tag';},
    /point directly/],
  ['release points at mutable main branch', f=>{f.release.target_commitish='main';},
    /exact immutable source tag SHA/],
  ['stale copied notes', f=>{f.release.body+='\\n';},
    /exact source-tagged UTF-8/],
  ['forged review title', f=>{f.release.name+=' Windows EXE';},
    /Reviewed test snapshot title/],
  ['missing provenance note', f=>{
    f.sourceNotes=f.sourceNotes.replace('PR #395','PR #999');
    f.release.body=f.sourceNotes;
  }, /Reviewed test snapshot title/],
  ['incorrect unmerged PR note', f=>{
    f.sourceNotes=f.sourceNotes.replace('PR #394','merged feature');
    f.release.body=f.sourceNotes;
  }, /Reviewed test snapshot title/],
  ['missing actual nonplayable warning', f=>{
    f.sourceNotes=f.sourceNotes.replace('No compiled Win64 EXE','Included compiled Win64 EXE');
    f.release.body=f.sourceNotes;
  }, /warning missing/],
  ['fake 99-thousand source count', f=>{
    f.sourceNotes=f.sourceNotes.replace('87,792','99,999');
    f.release.body=f.sourceNotes;
  }, /Reviewed test snapshot title/],
  ['attached binary', f=>{f.release.assets=[{name:'LateBrake.exe'}];},
    /zero attachments/],
  ['published as stable', f=>{f.release.prerelease=false;},
    /Release must be published/],
  ['new release without exact source notes', f=>{delete f.sourceNotes;},
    /exact source-tagged UTF-8/],
];
for (const [name, apply, expression] of reviewedChanges) {
  denied('deny test.2 '+name, {kind:'reviewed',apply}, expression);
}

const legacyChanges = [
  ['moved historical tag', f => { f.reference.object.sha = MAIN; },
    /Historical test source tag moved/],
  ['annotated historical tag', f => { f.reference.object.type = 'tag'; },
    /point directly/],
  ['unpublished draft', f => { f.release.draft = true; },
    /Release must be published/],
  ['stable rather than pre-release', f => { f.release.prerelease = false; },
    /Release must be published/],
  ['attached Windows game binary', f => { f.release.assets = [{name:'game.exe'}]; },
    /zero attachments/],
  ['missing published timestamp', f => { f.release.published_at = null; },
    /Release must be published/],
  ['wrong stored test target branch', f => { f.release.target_commitish = MAIN; },
    /Historical test release identity/],
  ['renamed test release', f => { f.release.name += ' Verified Windows'; },
    /Historical test release identity/],
  ['mutated release warning', f => { f.release.body = f.release.body.replace('not a playable game build', 'playable game build'); },
    /Historical test release identity/],
  ['mutated unmerged-work warning', f => { f.release.body = f.release.body.replace('Unmerged gameplay work', 'Merged gameplay work'); },
    /Historical test release identity/],
  ['forged alternate tag', f => { f.release.tag_name = 'v0.5.99'; },
    /Release must be published/],
  ['invented source notes for old tag', f => { f.sourceNotes = 'not present in tag'; },
    /must not pretend/],
];
for (const [name, apply, expression] of legacyChanges) {
  denied('deny ' + name, {kind:'legacy', apply}, expression);
}

const officialChanges = [
  ['moved source tag', f => { f.reference.object.sha = 'b'.repeat(40); },
    /exact immutable source tag SHA/],
  ['release body not equal to committed note bytes', f => { f.release.body += '\n'; },
    /exact source-tagged UTF-8/],
  ['missing tagged source notes', f => { delete f.sourceNotes; },
    /exact source-tagged UTF-8/],
  ['missing mandatory nonplayable warning', f => {
    f.sourceNotes = f.sourceNotes.replace('No compiled Win64 EXE', 'Windows executable');
    f.release.body = f.sourceNotes;
  }, /warning missing/],
  ['attached executable on official release', f => { f.release.assets = [{name:'installer.exe'}]; },
    /zero attachments/],
];
for (const [name, apply, expression] of officialChanges) {
  denied('deny official ' + name, {kind:'official', apply}, expression);
}
