'use strict';
// All fixtures are offline. This file never imports a GitHub token or writes.
const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const guard = require('../../scripts/SourceReleaseV05100Readiness.cjs');
const notes = fs.readFileSync(path.join(
  __dirname, '../../docs/releases/v0.5.100.candidate.md'), 'utf8');
const SHA = 'c'.repeat(40);
const MERGE = 'b'.repeat(40);
const data = value => ({data: structuredClone(value)});
function missing() {
  const e = new Error('not found'); e.status = 404; throw e;
}
function fixture() {
  const state = {
    main: {protected:true,commit:{sha:SHA}},
    notes: {type:'file',encoding:'base64',content:Buffer.from(notes).toString('base64')},
    policy: {type:'file',encoding:'base64',content:Buffer.from(
      'FollowerVerticalSpeedCmPerSecond LeadVerticalSpeedCmPerSecond ' +
      'dz - predictedZ !std::isfinite(followerVertical) ' +
      'MaxVerticalMotionResidualCm = 75.0 ' +
      'std::abs(verticalResidual) > MaxVerticalMotionResidualCm ' +
      'std::uint64_t LeadIdentity = 0 sample.LeadIdentity == 0 ' +
      'PreviousLeadIdentity != sample.LeadIdentity').toString('base64')},
    pawn: {type:'file',encoding:'base64',content:Buffer.from(
      'static_cast<double>(Velocity.Z) static_cast<double>(RivalVelocity.Z) ' +
      'static_cast<std::uint64_t>(RivalIdentity)'
    ).toString('base64')},
    mode: {type:'file',encoding:'base64',content:Buffer.from(
      'RivalCar->GetUniqueID() OutIdentity = RivalIdentity;'
    ).toString('base64')},
    tests: {type:'file',encoding:'base64',content:Buffer.from(
      'same-lane vertical relocation identical-geometry replacement ' +
      'rapid opponent swaps cannot inherit drafting charge').toString('base64')},
    pr: {state:'closed',merged:true,merge_commit_sha:MERGE,
      base:{ref:'main'},head:{repo:{full_name:'sanskarIN/latebrake'}}},
    compare: {status:'ahead',behind_by:0,ahead_by:1},
    checks: {check_runs:guard.REQUIRED_CHECKS.map((name,i) => ({
      id:i+1,name,status:'completed',conclusion:'success',
      app:{slug:'github-actions'},head_sha:SHA,
    }))},
    oldRelease: {tag_name:guard.PREVIOUS_TAG,
      target_commitish:guard.PREVIOUS_SHA,prerelease:true,draft:false,assets:[]},
    oldTag: {object:{type:'commit',sha:guard.PREVIOUS_SHA}},
    tag:null, release:null, reads:[], writes:0, onRead:()=>{},
  };
  const context = {
    repo:{owner:'sanskarIN',repo:'latebrake'},
    actor:'sanskarIN',eventName:'workflow_dispatch',
    ref:'refs/heads/main',sha:SHA,payload:{inputs:{candidate_sha:SHA}},
  };
  const github = {rest:{
    repos:{
      getBranch: async () => {
        state.reads.push('main');
        state.onRead();
        return data(state.main);
      },
      getContent: async ({path:filename,ref}) => {
        assert.equal(ref,SHA);
        state.reads.push(filename);
        if (filename===guard.NOTES_PATH) return data(state.notes);
        if (filename==='Source/Latebrake/Public/Racing/SlipstreamDraftBoost.h')
          return data(state.policy);
        if (filename==='Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp')
          return data(state.pawn);
        if (filename==='Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp')
          return data(state.mode);
        if (filename==='Tests/Standalone/SlipstreamDraftBoostTests.cpp')
          return data(state.tests);
        throw new Error('unexpected file: '+filename);
      },
      compareCommits: async ({base,head}) => {
        assert.equal(base,MERGE);assert.equal(head,SHA);
        return data(state.compare);
      },
      getReleaseByTag: async ({tag}) => {
        state.reads.push('release:'+tag);
        if (tag===guard.PREVIOUS_TAG) return data(state.oldRelease);
        assert.equal(tag,guard.TAG);
        return state.release?data(state.release):missing();
      },
    },
    pulls:{get:async({pull_number})=>{
      assert.equal(pull_number,377);return data(state.pr);
    }},
    checks:{listForRef:async({ref})=>{
      assert.equal(ref,SHA);return data(state.checks);
    }},
    git:{getRef:async({ref})=>{
      state.reads.push(ref);
      if (ref==='tags/'+guard.PREVIOUS_TAG) return data(state.oldTag);
      assert.equal(ref,'tags/'+guard.TAG);
      return state.tag?data(state.tag):missing();
    }},
  }};
  return {github,context,candidateSha:SHA,notes,state};
}
async function reject(change,regex=/./) {
  const f=fixture();change(f);
  await assert.rejects(guard.inspectReadiness(f),regex);
  assert.equal(f.state.writes,0,'preflight must never mutate GitHub');
}
test('valid exact-main source readiness returns no writes or game claims',async()=>{
  const f=fixture();const r=await guard.inspectReadiness(f);
  assert.equal(r.writesPerformed,0);
  assert.equal(r.existingTag,false);
  assert.equal(r.existingRelease,false);
  assert.equal(r.sourceChecks,7);
  assert.equal(r.nativeUnrealQualified,false);
  assert.equal(r.playableWindowsQualified,false);
  assert.equal(f.state.reads.filter(x=>x==='main').length,2);
});
test('verified existing identical tag/release is read-only',async()=>{
  const f=fixture();
  f.state.tag={object:{type:'commit',sha:SHA}};
  f.state.release={tag_name:guard.TAG,target_commitish:SHA,
    name:guard.NAME,body:notes,draft:false,prerelease:true,assets:[]};
  const result=await guard.inspectReadiness(f);
  assert.equal(result.existingTag,true);
  assert.equal(result.existingRelease,true);
  assert.equal(result.writesPerformed,0);
});
test('matching standalone tag is observed but not modified',async()=>{
  const f=fixture();f.state.tag={object:{type:'commit',sha:SHA}};
  const r=await guard.inspectReadiness(f);
  assert.equal(r.existingTag,true);assert.equal(r.existingRelease,false);
});
for (const [label,modify,error] of [
  ['wrong repo',f=>{f.context.repo.repo='other';},/owner manual/],
  ['wrong actor',f=>{f.context.actor='intruder';},/owner manual/],
  ['automatic PR event',f=>{f.context.eventName='pull_request';},/owner manual/],
  ['wrong ref',f=>{f.context.ref='refs/heads/feature';},/owner manual/],
  ['invalid candidate sha',f=>{f.candidateSha='123';},/owner manual/],
  ['input not exact checkout',f=>{f.candidateSha='f'.repeat(40);},/owner manual/],
  ['unprotected main',f=>{f.state.main.protected=false;},/Protected main/],
  ['moved main',f=>{f.state.main.commit.sha='f'.repeat(40);},/Protected main/],
  ['tampered note bytes',f=>{f.state.notes.content=Buffer.from(notes+'!').toString('base64');},/notes do not match/],
  ['symlink note path',f=>{f.state.notes.type='symlink';},/regular source file/],
  ['missing reviewed merge',f=>{f.state.pr.merged=false;},/not merged/],
  ['closed unmerged PR',f=>{f.state.pr.state='open';},/not merged/],
  ['merged on foreign base',f=>{f.state.pr.base.ref='other';},/not merged/],
  ['merged from fork',f=>{f.state.pr.head.repo.full_name='fork/repo';},/not merged/],
  ['diverged from PR merge',f=>{f.state.compare.status='diverged';},/not a descendant/],
  ['behind PR merge',f=>{f.state.compare.behind_by=1;},/not a descendant/],
  ['missing 3D policy',f=>{f.state.policy.content=Buffer.from('no Z').toString('base64');},/physical-3D/],
  ['missing Unreal integration',f=>{f.state.pawn.content=Buffer.from('fake').toString('base64');},/physical-3D/],
  ['missing 3D regression',f=>{f.state.tests.content=Buffer.from('fake').toString('base64');},/physical-3D/],
  ['missing rival actor binding',f=>{f.state.mode.content=Buffer.from('fake').toString('base64');},/physical-3D/],
  ['missing actor identity policy',f=>{
    f.state.policy.content=Buffer.from(
      'FollowerVerticalSpeedCmPerSecond LeadVerticalSpeedCmPerSecond dz - predictedZ ' +
      '!std::isfinite(followerVertical) MaxVerticalMotionResidualCm = 75.0 ' +
      'std::abs(verticalResidual) > MaxVerticalMotionResidualCm'
    ).toString('base64');
  },/physical-3D/],
  ['missing actor identity regression',f=>{
    f.state.tests.content=Buffer.from('same-lane vertical relocation').toString('base64');
  },/physical-3D/],
  ['spoofed GitHub checks app',f=>{f.state.checks.check_runs[0].app.slug='untrusted';},/untrusted app/],
  ['check belongs to other commit',f=>{f.state.checks.check_runs[0].head_sha=MERGE;},/commit identity/],
  ['newest check spoofed by another app',f=>{
    f.state.checks.check_runs.push({id:100,name:guard.REQUIRED_CHECKS[0],
      status:'completed',conclusion:'success',
      app:{slug:'untrusted'},head_sha:SHA});
  },/untrusted app/],
  ['missing Windows check',f=>{f.state.checks.check_runs=f.state.checks.check_runs.filter(
    row=>row.name!=='Portable C++20 tests / windows-latest');},/Windows|windows/],
  ['pending macOS check',f=>{f.state.checks.check_runs[2].status='in_progress';},/macos/],
  ['failed Linux check',f=>{f.state.checks.check_runs[1].conclusion='failure';},/ubuntu/],
  ['newer failed check masks older pass',f=>{
    f.state.checks.check_runs.push({id:100,name:guard.REQUIRED_CHECKS[0],
      status:'completed',conclusion:'failure',
      app:{slug:'github-actions'},head_sha:SHA});
  },/windows/],
  ['wrong predecessor SHA',f=>{f.state.oldRelease.target_commitish=SHA;},/Pre-release identity/],
  ['old tag changed',f=>{f.state.oldTag.object.sha=SHA;},/Source tag/],
  ['old release contains binary',f=>{f.state.oldRelease.assets=[{name:'game.exe'}];},/zero-assets/],
  ['future tag moved',f=>{f.state.tag={object:{type:'commit',sha:MERGE}};},/Source tag/],
  ['future annotated tag',f=>{f.state.tag={object:{type:'tag',sha:SHA}};},/Source tag/],
  ['release without tag',f=>{f.state.release={
    tag_name:guard.TAG,target_commitish:SHA,name:guard.NAME,body:notes,
    prerelease:true,draft:false,assets:[]};},/without immutable source tag/],
  ['existing release bytes mismatch',f=>{
    f.state.tag={object:{type:'commit',sha:SHA}};
    f.state.release={tag_name:guard.TAG,target_commitish:SHA,
      name:guard.NAME,body:notes+'!',prerelease:true,draft:false,assets:[]};
  },/note bytes differ/],
  ['existing release executable',f=>{
    f.state.tag={object:{type:'commit',sha:SHA}};
    f.state.release={tag_name:guard.TAG,target_commitish:SHA,
      name:guard.NAME,body:notes,prerelease:true,draft:false,assets:[{name:'installer.exe'}]};
  },/zero-assets/],
]) test('deny '+label,async()=>reject(modify,error));
test('reject stale final protected-main read',async()=>{
  const f=fixture();let calls=0;
  f.state.onRead=()=>{calls+=1;if(calls===2)f.state.main.commit.sha=MERGE;};
  await assert.rejects(guard.inspectReadiness(f),/changed during read-only preflight/);
  assert.equal(f.state.writes,0);
});
test('rate limit is not mistaken for an absent future tag',async()=>{
  await reject(f=>{f.github.rest.git.getRef=async()=>{throw new Error('rate limit');};},
    /rate limit/);
});
test('draft-warning tokens cannot be omitted',()=>{
  assert.doesNotThrow(()=>guard.validateNotes(notes));
  assert.throws(()=>guard.validateNotes(notes.replace('NO PLAYABLE WINDOWS EXECUTABLE','')),
    /warning absent/);
  assert.throws(()=>guard.validateNotes(notes+' DRAFT NOTES PLACEHOLDER'),
    /placeholder/);
});
