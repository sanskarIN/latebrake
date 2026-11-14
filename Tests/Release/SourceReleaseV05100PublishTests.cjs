'use strict';

// Offline fake API only: tests do NOT create real tags or releases.
const {test} = require('node:test');
const assert = require('node:assert/strict');
const gate = require('../../scripts/SourceReleaseV05100Readiness.cjs');
const publisher = require('../../scripts/PublishSourceV05100.cjs');

const MAIN = 'a'.repeat(40);
const notes = '# v0.5.100\nSOURCE-ONLY PRE-RELEASE\n';
const data = value => ({data:structuredClone(value)});
function fixture() {
  const state = {
    protected:true, sha:MAIN, merged:true,
    tag:false, release:false, assets:[], writes:[],
    afterTag:()=>{}, afterRelease:()=>{},
  };
  const context = {
    repo:{owner:'sanskarIN',repo:'latebrake'},
    actor:'sanskarIN', eventName:'workflow_dispatch',
    ref:'refs/heads/main', sha:MAIN,
    payload:{inputs:{
      candidate_sha:MAIN,
      approval:publisher.APPROVAL_PREFIX + MAIN,
    }},
  };
  const releaseData = () => ({
    tag_name:gate.TAG,target_commitish:MAIN,name:gate.NAME,body:notes,
    prerelease:true,draft:false,assets:state.assets,
  });
  const github = {rest:{
    git:{
      createRef:async ({owner,repo,ref,sha})=>{
        assert.equal(owner,'sanskarIN');
        assert.equal(repo,'latebrake');
        assert.equal(ref,'refs/tags/' + gate.TAG);
        assert.equal(sha,MAIN);
        state.writes.push('tag');
        state.tag=true;
        state.afterTag();
        return data({object:{type:'commit',sha}});
      },
    },
    repos:{
      createRelease:async input=>{
        assert.equal(input.tag_name,gate.TAG);
        assert.equal(input.target_commitish,MAIN);
        assert.equal(input.body,notes);
        assert.equal(input.name,gate.NAME);
        assert.equal(input.draft,false);
        assert.equal(input.prerelease,true);
        assert.equal(input.generate_release_notes,false);
        assert.equal(input.make_latest,'false');
        state.writes.push('release');
        state.release=true;
        state.afterRelease();
        return data(releaseData());
      },
      getReleaseByTag:async ({tag})=>{
        assert.equal(tag,gate.TAG);
        assert.equal(state.release,true);
        return data(releaseData());
      },
    },
  }};
  const audit = async ({candidateSha,context:ctx,notes:body})=>{
    assert.equal(candidateSha,MAIN);
    assert.equal(ctx,context);
    assert.equal(body,notes);
    if (!state.protected || state.sha !== MAIN ||
        !state.merged) {
      throw Error('Protected main or required native gameplay PR not ready');
    }
    return {
      candidateSha,
      existingTag:state.tag,existingRelease:state.release,
      nativeUnrealQualified:false,playableWindowsQualified:false,
      writesPerformed:0,
    };
  };
  return {state,context,github,notes,audit};
}
async function runCase(f) {
  const original = gate.inspectReadiness;
  gate.inspectReadiness = f.audit;
  try {return await publisher.publish(f);}
  finally {gate.inspectReadiness = original;}
}
test('accept only explicitly approved exact protected-main owner SHA',()=>{
  const f=fixture();
  assert.equal(publisher.requireExactAuthorization(f.context),MAIN);
  const invalid=[
    c=>{c.actor='intruder';},
    c=>{c.repo.repo='other';},
    c=>{c.ref='refs/heads/feature';},
    c=>{c.eventName='pull_request';},
    c=>{c.sha='b'.repeat(40);},
    c=>{c.payload.inputs.candidate_sha='b'.repeat(40);},
    c=>{c.payload.inputs.candidate_sha='main';},
    c=>{c.payload.inputs.approval='I approve';},
    c=>{c.payload.inputs.approval=publisher.APPROVAL_PREFIX+'b'.repeat(40);},
  ];
  for (const change of invalid) {
    const clone=structuredClone(f.context);
    change(clone);
    assert.throws(()=>publisher.requireExactAuthorization(clone),/approval required/);
  }
});
test('owner-approved exact source-only publish makes only direct tag and zero-asset prerelease',async()=>{
  const f=fixture();
  const result=await runCase(f);
  assert.deepEqual(f.state.writes,['tag','release']);
  assert.equal(result.prerelease,true);
  assert.deepEqual(result.assets,[]);
});
test('same already-published source release is idempotent',async()=>{
  const f=fixture();
  await runCase(f);
  f.state.writes=[];
  await runCase(f);
  assert.deepEqual(f.state.writes,[]);
});
test('recover direct tag created before an interrupted publisher',async()=>{
  const f=fixture();f.state.tag=true;
  await runCase(f);
  assert.deepEqual(f.state.writes,['release']);
});
test('deny missing merged gameplay PR before any publication',async()=>{
  const f=fixture();f.state.merged=false;
  await assert.rejects(runCase(f),/not ready/);
  assert.deepEqual(f.state.writes,[]);
});
test('deny protected main drift before any publication',async()=>{
  const f=fixture();f.state.sha='b'.repeat(40);
  await assert.rejects(runCase(f),/not ready/);
  assert.deepEqual(f.state.writes,[]);
});
test('deny a protected main change after tag but before release',async()=>{
  const f=fixture();f.state.afterTag=()=>{f.state.sha='b'.repeat(40);};
  await assert.rejects(runCase(f),/not ready/);
  assert.deepEqual(f.state.writes,['tag']);
});
test('deny added assets in the publisher readback',async()=>{
  const f=fixture();f.state.afterRelease=()=>{f.state.assets.push({name:'game.exe'});};
  await assert.rejects(runCase(f),/assets/);
  assert.deepEqual(f.state.writes,['tag','release']);
});
test('publisher has no auto-push trigger or unconfirmed standalone source release',()=>{
  const fs=require('node:fs'),path=require('node:path');
  const yaml=fs.readFileSync(path.join(__dirname,
    '../../.github/workflows/publish-v05100-mainline-source.yml'),'utf8');
  assert.match(yaml,/workflow_dispatch:/);
  assert.doesNotMatch(yaml,/^\s+push:/m);
  for (const must of [
    "github.actor == 'sanskarIN'",
    "github.ref == 'refs/heads/main'",
    "contents: write",
    "pull-requests: read",
    "checks: read",
    "candidate_sha:",
    "approval:",
    "SourceReleaseV05100ReadinessTests.cjs",
    "SourceReleaseV05100PublishTests.cjs",
    "guard.publish({github,context,notes})",
    "SlipstreamDraftBoost",
  ])assert.ok(yaml.includes(must),'Workflow invariant missing: '+must);
});
