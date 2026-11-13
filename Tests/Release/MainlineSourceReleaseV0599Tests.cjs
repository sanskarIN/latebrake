'use strict';
// Entirely offline: synthetic GitHub API, no tokens, no tags/releases written.
const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const guard = require('../../scripts/MainlineSourceReleaseV0599.cjs');
const notes = fs.readFileSync(
  path.join(__dirname, '../../docs/releases/v0.5.99.md'), 'utf8');
const MAIN = 'a'.repeat(40);
const data = x => ({data: structuredClone(x)});
function missing() {
  const error = new Error('not found'); error.status = 404; throw error;
}
function fixture() {
  const state = {
    main: {protected:true, commit:{sha:MAIN}},
    note: {type:'file', encoding:'base64', content:Buffer.from(notes).toString('base64')},
    policy: {type:'file', encoding:'base64', content:Buffer.from(
      'ClearPhysicalDraftPath MaxRelativeMotionResidualCm').toString('base64')},
    mode: {type:'file', encoding:'base64', content:Buffer.from(
      'LatebrakeRivalDraftSightline LineTraceSingleByChannel').toString('base64')},
    oldRelease: {tag_name:guard.PREVIOUS_TAG, target_commitish:guard.PREVIOUS_SHA,
      draft:false, prerelease:true, assets:[]},
    oldTag: {object:{type:'commit',sha:guard.PREVIOUS_SHA}},
    tag:null, release:null, writes:[], onTag:()=>{}, onRelease:()=>{},
  };
  const context = {
    repo:{owner:'sanskarIN',repo:'latebrake'},
    actor:'sanskarIN', eventName:'push', ref:'refs/heads/main', sha:MAIN,
    payload:{head_commit:{message:guard.APPROVED_SUBJECT}},
  };
  const github = {rest:{
    repos:{
      getBranch: async () => data(state.main),
      getContent: async ({ref, path:filename}) => {
        assert.equal(ref, MAIN);
        if (filename === guard.NOTES_PATH) return data(state.note);
        if (filename ===
          'Source/Latebrake/Public/Racing/SlipstreamDraftBoost.h')
          return data(state.policy);
        if (filename ===
          'Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp')
          return data(state.mode);
        throw new Error('unexpected source lookup: ' + filename);
      },
      getReleaseByTag: async ({tag}) => {
        if (tag === guard.PREVIOUS_TAG) return data(state.oldRelease);
        assert.equal(tag, guard.TAG);
        return state.release ? data(state.release) : missing();
      },
      createRelease: async input => {
        assert.equal(input.tag_name, guard.TAG);
        assert.equal(input.target_commitish, MAIN);
        assert.equal(input.body, notes);
        assert.equal(input.draft, false);
        assert.equal(input.prerelease, true);
        assert.equal(input.make_latest, 'false');
        state.writes.push('release');
        state.release = {
          tag_name:input.tag_name, target_commitish:input.target_commitish,
          draft:input.draft, prerelease:input.prerelease,
          name:input.name, body:input.body, assets:[],
        };
        state.onRelease();
        return data(state.release);
      },
    },
    git:{
      getRef: async ({ref}) => {
        if (ref === 'tags/' + guard.PREVIOUS_TAG) return data(state.oldTag);
        assert.equal(ref, 'tags/' + guard.TAG);
        return state.tag ? data(state.tag) : missing();
      },
      createRef: async ({ref,sha}) => {
        assert.equal(ref, 'refs/tags/' + guard.TAG); assert.equal(sha, MAIN);
        state.writes.push('tag');
        state.tag = {object:{type:'commit',sha}};
        state.onTag();
        return data(state.tag);
      },
    },
  }};
  return {github,context,notes,state};
}
async function reject(change, pattern=/./) {
  const f=fixture(); change(f);
  await assert.rejects(guard.publish(f),pattern);
  assert.deepEqual(f.state.writes,[]);
}
test('approved main push is an exact one-version authorization',async()=>{
  const f=fixture(); await guard.inspectState(f);
  assert.deepEqual(f.state.writes,[]);
});
test('manual owner dispatch can retry exactly without changing source',async()=>{
  const f=fixture(); f.context.eventName='workflow_dispatch';
  f.context.payload={}; await guard.inspectState(f);
});
test('one approved run creates only one tag and asset-free prerelease',async()=>{
  const f=fixture(); const r=await guard.publish(f);
  assert.deepEqual(f.state.writes,['tag','release']);
  assert.equal(r.body,notes); assert.deepEqual(r.assets,[]);
});
test('already-published identical release is idempotent',async()=>{
  const f=fixture(); await guard.publish(f); f.state.writes=[];
  await guard.publish(f); assert.deepEqual(f.state.writes,[]);
});
test('interrupted exact tag can finish without rewriting a tag',async()=>{
  const f=fixture(); f.state.tag={object:{type:'commit',sha:MAIN}};
  await guard.publish(f); assert.deepEqual(f.state.writes,['release']);
});
for(const [label,change] of [
  ['not owner',f=>{f.context.actor='other';}],
  ['not exact commit subject',f=>{f.context.payload.head_commit.message='run release';}],
  ['unsanctioned PR event',f=>{f.context.eventName='pull_request';}],
  ['wrong branch',f=>{f.context.ref='refs/heads/feature';}],
  ['wrong repo',f=>{f.context.repo.repo='untrusted';}],
  ['unprotected main',f=>{f.state.main.protected=false;}],
  ['moved main',f=>{f.state.main.commit.sha='b'.repeat(40);}],
  ['changed release notes',f=>{f.state.note.content=
    Buffer.from(notes+'\n').toString('base64');}],
  ['fake nonregular notes',f=>{f.state.note.type='symlink';}],
  ['missing source feature',f=>{f.state.policy.content=
    Buffer.from('other feature').toString('base64');}],
  ['missing collision integration',f=>{f.state.mode.content=
    Buffer.from('no query').toString('base64');}],
  ['wrong predecessor commit',f=>{f.state.oldRelease.target_commitish=MAIN;}],
  ['previous tag moved',f=>{f.state.oldTag.object.sha=MAIN;}],
  ['old release has assets',f=>{f.state.oldRelease.assets.push({name:'fake.exe'});}],
  ['prior release is stable',f=>{f.state.oldRelease.prerelease=false;}],
  ['new tag conflicts',f=>{f.state.tag={object:{type:'commit',sha:'b'.repeat(40)}};}],
  ['annotated new tag forbidden',f=>{f.state.tag={object:{type:'tag',sha:MAIN}};}],
  ['existing release lacks tag',f=>{f.state.release={
    tag_name:guard.TAG,target_commitish:MAIN, draft:false,
    prerelease:true,body:notes,name:guard.NAME,assets:[]};}],
  ['new release with executable',f=>{f.state.tag={object:{type:'commit',sha:MAIN}};
    f.state.release={tag_name:guard.TAG,target_commitish:MAIN,draft:false,
      prerelease:true,body:notes,name:guard.NAME,assets:[{name:'game.exe'}]};}],
])test('refuse '+label+' before any write',async()=>reject(change));
test('unexpected network error never becomes authorization to create',async()=>{
  await reject(f=>{f.github.rest.git.getRef=async()=>{throw Error('rate limited');};},/rate limited/);
});
test('source moves just after creating tag: refuse publication',async()=>{
  const f=fixture();f.state.onTag=()=>{f.state.main.commit.sha='b'.repeat(40);};
  await assert.rejects(guard.publish(f),/Protected main moved/);
  assert.deepEqual(f.state.writes,['tag']);
});
test('tag changes after release: fail read-back verification',async()=>{
  const f=fixture();f.state.onRelease=()=>{f.state.tag.object.sha='b'.repeat(40);};
  await assert.rejects(guard.publish(f),/Immutable source tag mismatch/);
});
test('attached binary after release: fail read-back verification',async()=>{
  const f=fixture();f.state.onRelease=()=>{f.state.release.assets.push({name:'unexpected.exe'});};
  await assert.rejects(guard.publish(f),/zero-assets verification failed/);
});
