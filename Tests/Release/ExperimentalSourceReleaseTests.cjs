'use strict';
// Offline API fixtures only: this file never contacts GitHub or creates tags.
const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const guard = require('../../scripts/ExperimentalSourceRelease.cjs');
const notes = fs.readFileSync(path.join(__dirname, '../../docs/releases/v0.5.97.md'), 'utf8');
const SHA = 'a'.repeat(40), MAIN = 'b'.repeat(40), PREVIOUS = 'c'.repeat(40);

function fixture() {
  const plan = {tag: 'v0.5.97', previousTag: 'v0.5.96', previousSha: PREVIOUS,
    sourceSha: SHA, prNumber: 295, branch: 'feat/chaos-four-corner-suspension',
    notesPath: 'docs/releases/v0.5.97.md', name: 'LateBrake v0.5.97 SOURCE-ONLY Pre-release'};
  const context = {repo: {owner: 'sanskarIN', repo: 'latebrake'}, actor: 'sanskarIN',
    eventName: 'workflow_dispatch', ref: 'refs/heads/main', sha: MAIN};
  const state = {main: {protected: true, commit: {sha: MAIN}},
    pr: {state: 'open', draft: true, base: {ref: 'main'},
      head: {sha: SHA, ref: plan.branch, repo: {full_name: 'sanskarIN/latebrake'},
        user: {login: 'sanskarIN'}}},
    sourceNotes: {type: 'file', encoding: 'base64', content: Buffer.from(notes).toString('base64')},
    previous: {tag_name: plan.previousTag, target_commitish: PREVIOUS,
      draft: false, prerelease: true, assets: []},
    previousRef: {object: {type: 'commit', sha: PREVIOUS}},
    tag: null, release: null, writes: [], onTag: () => {}, onRelease: () => {}};
  const result = value => ({data: structuredClone(value)});
  const missing = () => { const error = new Error('fixture missing'); error.status = 404; throw error; };
  const github = {rest: {repos: {
    getBranch: async () => result(state.main),
    getContent: async input => {
      assert.equal(input.ref, SHA); assert.equal(input.path, plan.notesPath);
      return result(state.sourceNotes);
    },
    getReleaseByTag: async input => {
      if (input.tag === plan.previousTag) return result(state.previous);
      assert.equal(input.tag, plan.tag);
      return state.release ? result(state.release) : missing();
    },
    createRelease: async input => {
      assert.equal(input.make_latest, 'false');
      assert.equal(input.generate_release_notes, false);
      assert.equal(input.draft, false); assert.equal(input.prerelease, true);
      state.writes.push('release');
      state.release = {tag_name: input.tag_name, target_commitish: input.target_commitish,
        name: input.name, body: input.body, draft: false, prerelease: true, assets: []};
      state.onRelease();
      return result(state.release);
    },
  }, pulls: {get: async input => {
    assert.equal(input.pull_number, 295); return result(state.pr);
  }}, git: {
    getRef: async input => {
      if (input.ref === `tags/${plan.previousTag}`) return result(state.previousRef);
      assert.equal(input.ref, `tags/${plan.tag}`);
      return state.tag ? result(state.tag) : missing();
    },
    createRef: async input => {
      assert.equal(input.ref, `refs/tags/${plan.tag}`); assert.equal(input.sha, SHA);
      state.writes.push('tag'); state.tag = {object: {type: 'commit', sha: input.sha}};
      state.onTag(); return result(state.tag);
    },
  }}};
  return {github, context, plan, notes, state};
}

async function refusesWithoutWrite(change, pattern = /./) {
  const f = fixture(); change(f);
  await assert.rejects(guard.publish(f), pattern);
  assert.deepEqual(f.state.writes, []);
}

test('preflight authenticates exact draft source and performs no writes', async () => {
  const f = fixture(); const state = await guard.inspectState(f);
  assert.equal(state.release, null); assert.deepEqual(f.state.writes, []);
});
test('publication creates only one immutable tag and zero-asset prerelease', async () => {
  const f = fixture(); const release = await guard.publish(f);
  assert.deepEqual(f.state.writes, ['tag', 'release']);
  assert.equal(release.target_commitish, SHA); assert.equal(release.body, notes);
  assert.deepEqual(release.assets, []);
});
test('exact retry is idempotent and verifies existing bytes without edits', async () => {
  const f = fixture(); await guard.publish(f); f.state.writes = [];
  await guard.publish(f); assert.deepEqual(f.state.writes, []);
});
test('existing correct source tag is reused without rewriting it', async () => {
  const f = fixture(); f.state.tag = {object: {type: 'commit', sha: SHA}};
  await guard.publish(f); assert.deepEqual(f.state.writes, ['release']);
});
for (const [label, change] of [
  ['stale head', f => { f.state.pr.head.sha = MAIN; }],
  ['closed PR', f => { f.state.pr.state = 'closed'; }],
  ['ready PR', f => { f.state.pr.draft = false; }],
  ['fork', f => { f.state.pr.head.repo.full_name = 'other/latebrake'; }],
  ['different owner', f => { f.state.pr.head.user.login = 'other'; }],
  ['different branch', f => { f.state.pr.head.ref = 'main'; }],
  ['different base', f => { f.state.pr.base.ref = 'other'; }],
  ['moved main', f => { f.state.main.commit.sha = SHA; }],
  ['unprotected main', f => { f.state.main.protected = false; }],
]) test(`refuse ${label} before any mutation`, async () => refusesWithoutWrite(change));
for (const [label, change] of [
  ['wrong actor', f => { f.context.actor = 'other'; }],
  ['push trigger', f => { f.context.eventName = 'push'; }],
  ['feature workflow', f => { f.context.ref = 'refs/heads/feat/source'; }],
  ['other repository', f => { f.context.repo.repo = 'other'; }],
  ['abbreviated SHA', f => { f.plan.sourceSha = 'abc'; }],
  ['v0.6.0 quantity-gate bypass', f => { f.plan.tag = 'v0.6.0'; }],
  ['skipped predecessor', f => { f.plan.previousTag = 'v0.5.95'; }],
  ['noncanonical version', f => { f.plan.tag = 'v0.5.097'; }],
  ['unsafe integer patch', f => { f.plan.tag = 'v0.5.9007199254740993'; f.plan.previousTag = 'v0.5.9007199254740992'; }],
]) test(`refuse ${label}`, async () => refusesWithoutWrite(change));
for (const [label, change] of [
  ['changed source note bytes', f => { f.state.sourceNotes.content = Buffer.from(notes + '\n').toString('base64'); }],
  ['non-base64 notes', f => { f.state.sourceNotes.encoding = 'none'; }],
  ['notes symlink', f => { f.state.sourceNotes.type = 'symlink'; }],
  ['missing warnings', f => { f.notes = notes.replace('NO PLAYABLE WINDOWS EXECUTABLE', 'preview'); }],
  ['unrelated notes path', f => { f.plan.notesPath = 'README.md'; }],
  ['playable release name', f => { f.plan.name = 'LateBrake v0.5.97 playable'; }],
  ['changed previous target', f => { f.state.previous.target_commitish = SHA; }],
  ['previous executable assets', f => { f.state.previous.assets.push({name: 'fake.exe'}); }],
  ['previous stable release', f => { f.state.previous.prerelease = false; }],
  ['previous draft release', f => { f.state.previous.draft = true; }],
  ['previous tag drift', f => { f.state.previousRef.object.sha = SHA; }],
  ['annotated previous tag', f => { f.state.previousRef.object.type = 'tag'; }],
  ['new tag drift', f => { f.state.tag = {object: {type: 'commit', sha: MAIN}}; }],
  ['annotated new tag', f => { f.state.tag = {object: {type: 'tag', sha: SHA}}; }],
]) test(`refuse ${label} without touching tags/releases`, async () => refusesWithoutWrite(change));
test('bad existing release is rejected BEFORE a new tag can be created', async () => {
  await refusesWithoutWrite(f => { f.state.release = {...f.state.previous, tag_name: f.plan.tag}; },
    /release identity/);
});
test('published release without source tag is not silently repaired', async () => {
  const f = fixture(); await guard.publish(f); f.state.tag = null; f.state.writes = [];
  await assert.rejects(guard.publish(f), /no immutable source tag/);
  assert.deepEqual(f.state.writes, []);
});
test('non-404 API failure is never treated as permission to create', async () => {
  await refusesWithoutWrite(f => { f.github.rest.git.getRef = async () => {
    const error = new Error('access denied'); error.status = 403; throw error;
  }; }, /access denied/);
});
test('PR moves after tag creation: never publish the release', async () => {
  const f = fixture(); f.state.onTag = () => { f.state.pr.head.sha = MAIN; };
  await assert.rejects(guard.publish(f), /moved draft/);
  assert.deepEqual(f.state.writes, ['tag']); assert.equal(f.state.release, null);
});
test('main moves after tag creation: never publish the release', async () => {
  const f = fixture(); f.state.onTag = () => { f.state.main.commit.sha = SHA; };
  await assert.rejects(guard.publish(f), /main moved/);
  assert.deepEqual(f.state.writes, ['tag']); assert.equal(f.state.release, null);
});
test('concurrent identical publication is reauthenticated without a second release write', async () => {
  const f = fixture(); f.state.onTag = () => { f.state.release = {
    ...f.state.previous, tag_name: f.plan.tag, target_commitish: SHA,
    name: f.plan.name, body: notes,
  }; };
  await guard.publish(f); assert.deepEqual(f.state.writes, ['tag']);
});
test('post-publication tag drift fails the fresh readback', async () => {
  const f = fixture(); f.state.onRelease = () => { f.state.tag.object.sha = MAIN; };
  await assert.rejects(guard.publish(f), /immutable source commit mismatch/);
});
test('post-publication asset drift fails the fresh readback', async () => {
  const f = fixture(); f.state.onRelease = () => { f.state.release.assets.push({name: 'unexpected'}); };
  await assert.rejects(guard.publish(f), /zero-assets check failed/);
});
