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
// Guard the ACTUAL job/trigger structure, not a word anywhere in the
// workflow: a second trigger or a preflight write token must fail CI even
// if the string "workflow_dispatch" still appears in a comment.
function verifyManualPublisherWorkflow(yaml) {
  const triggerMatch = /^on:\s*\n([\s\S]*?)^permissions:\s*$/m.exec(yaml);
  assert.ok(triggerMatch, 'Publisher needs top-level trigger and permission blocks');
  const directTriggers = triggerMatch[1].split('\n')
    .filter(line => /^  [a-z_]+:\s*(?:#.*)?$/.test(line))
    .map(line => line.trim().split(':')[0]);
  assert.deepEqual(directTriggers, ['workflow_dispatch'],
    'Source release must have exactly one manual dispatch trigger');
  assert.match(triggerMatch[1], /^      candidate_sha:\s*$/m);
  assert.match(triggerMatch[1], /^      approval:\s*$/m);

  const publishSplit = yaml.split(/^  publish:\s*$/m);
  assert.equal(publishSplit.length, 2,
    'Exactly one final publish job is required');
  const [prePublish, publish] = publishSplit;
  const topLevelPermissions = prePublish.match(
    /^permissions:\s*\n([\s\S]*?)^concurrency:\s*$/m);
  assert.ok(topLevelPermissions, 'Explicit top-level readonly permissions required');
  assert.match(topLevelPermissions[1], /^  contents: read\s*$/m);
  assert.doesNotMatch(prePublish, /^\s+contents: write\s*$/m,
    'Preflight or test jobs may never have a write-enabled token');
  assert.match(publish, /^    needs: \[preflight, cpp, editor, policies, docs\]\s*$/m);
  assert.match(publish, /^    permissions:\s*\n      contents: write\s*$/m);
  assert.match(publish, /github\.actor == 'sanskarIN'/);
  assert.match(publish, /github\.ref == 'refs\/heads\/main'/);
  assert.match(prePublish, /SourceReleaseV05100ReadinessTests\.cjs/);
  assert.match(prePublish, /SourceReleaseV05100PublishTests\.cjs/);
  assert.match(prePublish, /publisher\.requireExactAuthorization\(context\)/);
  assert.match(prePublish, /guard\.inspectReadiness\(/);
  assert.match(publish, /publisher\.publish\(\{github,context,notes\}\)/);
  assert.match(publish, /ref: \$\{\{ github\.sha \}\}/);
}
test('publisher is manual-only, read-only before guarded final write',()=>{
  const fs=require('node:fs'),path=require('node:path');
  const yaml=fs.readFileSync(path.join(__dirname,
    '../../.github/workflows/publish-v05100-mainline-source.yml'),'utf8');
  verifyManualPublisherWorkflow(yaml);
});
test('publisher isolation audit rejects automatic events, privilege drift and missing gates',()=>{
  const fs=require('node:fs'),path=require('node:path');
  const original=fs.readFileSync(path.join(__dirname,
    '../../.github/workflows/publish-v05100-mainline-source.yml'),'utf8');
  const modifications=[
    ['push event',v=>v.replace('  workflow_dispatch:', '  push:\n  workflow_dispatch:')],
    ['pull_request_target event',v=>v.replace('  workflow_dispatch:',
      '  pull_request_target:\n  workflow_dispatch:')],
    ['scheduled event',v=>v.replace('  workflow_dispatch:',
      '  schedule:\n  workflow_dispatch:')],
    ['repository_dispatch event',v=>v.replace('  workflow_dispatch:',
      '  repository_dispatch:\n  workflow_dispatch:')],
    ['preflight write token',v=>v.replace('  preflight:\n',
      '  preflight:\n    permissions:\n      contents: write\n')],
    ['global write token',v=>v.replace('permissions:\n  contents: read',
      'permissions:\n  contents: write')],
    ['drop preflight dependency',v=>v.replace(
      'needs: [preflight, cpp, editor, policies, docs]',
      'needs: [cpp, editor, policies, docs]')],
    ['drop macOS test dependency',v=>v.replace(
      'needs: [preflight, cpp, editor, policies, docs]',
      'needs: [preflight, editor, policies, docs]')],
    ['bypass exact-SHA authorization',v=>v.replace(
      'publisher.requireExactAuthorization(context)',
      'context.sha')],
    ['remove final owner assertion',v=>v.replace(
      "      github.actor == 'sanskarIN' &&\n      github.ref == 'refs/heads/main'\n    runs-on: ubuntu-latest\n    permissions:\n      contents: write",
      "      github.ref == 'refs/heads/main'\n    runs-on: ubuntu-latest\n    permissions:\n      contents: write")],
  ];
  for (const [name,mutate] of modifications) {
    const changed=mutate(original);
    assert.notEqual(changed,original,'Mutation did not change workflow: '+name);
    assert.throws(()=>verifyManualPublisherWorkflow(changed),
      undefined,'Publisher trust boundary should reject: '+name);
  }
});
