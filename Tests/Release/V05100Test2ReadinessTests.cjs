'use strict';

// Offline fake-GitHub contract tests. NEVER use real credentials, tags,
// binaries, workflow dispatch or mutable public release state.
const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const guard = require('../../scripts/SourcePreviewV05100Test2Readiness.cjs');
const audit = require('../../scripts/AuditPublishedSourceRelease.cjs');
const official = require('../../scripts/SourceReleaseV05100Readiness.cjs');

const SHA = 'a'.repeat(40);
const MERGED_SHA = 'b'.repeat(40);
const NOTES = fs.readFileSync(guard.NOTES_PATH, 'utf8');
function copy(x) {return structuredClone(x);}
function data(x) {return {data: copy(x)};}

function scenario() {
  const state = {
    current: SHA,
    protected: true,
    notes: NOTES,
    merged: true,
    sourceIncluded: true,
    historicAncestor: true,
    gameplayMerged: new Set(),
    checks: official.REQUIRED_CHECKS.map((name, index) => ({
      name, id:index + 100, app:{slug:'github-actions'},
      head_sha:SHA, status:'completed', conclusion:'success',
    })),
    oldTag: {object:{type:'commit',sha:guard.PREVIOUS_SHA}},
    oldRelease: {
      tag_name:guard.PREVIOUS_TAG, target_commitish:'main',
      name:audit.LEGACY_TEST_NAME,
      body:audit.LEGACY_TEST_BODY.replace(/\n/g, '\r\n'),
      assets:[], draft:false, prerelease:true,
      published_at:'2026-11-16T14:16:10Z',
    },
    officialTag:null, newTag:null, newRelease:null,
    branchCalls:0, apiCalls:[],
  };
  const context = {
    repo:{owner:'sanskarIN',repo:'latebrake'},
    actor:'sanskarIN',eventName:'workflow_dispatch',
    ref:'refs/heads/main',sha:SHA,
    payload:{inputs:{candidate_sha:SHA}},
  };
  const mark = name => {state.apiCalls.push(name);};
  const github = {rest:{
    repos:{
      getBranch: async input => {
        mark('getBranch');
        assert.equal(input.branch,'main');
        ++state.branchCalls;
        return data({protected:state.protected,commit:{sha:state.current}});
      },
      getContent: async input => {
        mark('getContent');
        assert.deepEqual({path:input.path,ref:input.ref},
          {path:guard.NOTES_PATH,ref:SHA});
        return data({type:'file',encoding:'base64',
          content:Buffer.from(state.notes,'utf8').toString('base64')});
      },
      compareCommits: async input => {
        mark('compareCommits');
        assert.equal(input.head,SHA);
        if (input.base === MERGED_SHA) {
          return data({status:state.sourceIncluded?'ahead':'diverged',
            behind_by:state.sourceIncluded?0:1});
        }
        assert.equal(input.base, guard.PREVIOUS_SHA);
        return data({status:state.historicAncestor?'ahead':'diverged',
          behind_by:state.historicAncestor?0:1});
      },
      getReleaseByTag: async input => {
        mark('getReleaseByTag');
        if(input.tag === guard.PREVIOUS_TAG) return data(state.oldRelease);
        if(input.tag === guard.TAG && state.newRelease)
          return data(state.newRelease);
        throw Object.assign(Error('not found'),{status:404});
      },
    },
    pulls:{
      get: async input => {
        mark('getPR');
        const number=input.pull_number;
        if(number === guard.REQUIRED_SOURCE_PR) {
          return data({
            state:state.merged?'closed':'open',
            merged:state.merged,
            base:{ref:'main'},
            head:{repo:{full_name:'sanskarIN/latebrake'}},
            merge_commit_sha:MERGED_SHA,
          });
        }
        assert.ok(guard.UNMERGED_GAMEPLAY_PRS.includes(number));
        return data({state:state.gameplayMerged.has(number)?'closed':'open',
          merged:state.gameplayMerged.has(number),base:{ref:'main'}});
      },
    },
    git:{
      getRef: async input => {
        mark('getRef');
        if(input.ref === 'tags/' + guard.PREVIOUS_TAG)
          return data(state.oldTag);
        if(input.ref === 'tags/v0.5.100' && state.officialTag)
          return data(state.officialTag);
        if(input.ref === 'tags/' + guard.TAG && state.newTag)
          return data(state.newTag);
        throw Object.assign(Error('not found'),{status:404});
      },
    },
    checks:{
      listForRef: async input => {
        mark('listForRef');
        assert.equal(input.ref,SHA);
        return data({check_runs:state.checks});
      },
    },
  }};
  const run = () => guard.inspectReadiness({
    github,context,candidateSha:SHA,notes:NOTES,
  });
  return {state,context,github,run};
}

test('candidate release note is honest and includes all required warnings',()=>{
  guard.checkNotes(NOTES);
  assert.equal(guard.TAG,'v0.5.100-test.2');
  assert.equal(guard.REQUIRED_SOURCE_PR,395);
  assert.equal(guard.UNMERGED_GAMEPLAY_PRS.length,3);
});

test('read-only happy path requires real old tag, merged source and 7 green checks',async()=>{
  const f=scenario();
  const result=await f.run();
  assert.equal(result.tag,guard.TAG);
  assert.equal(result.candidateSha,SHA);
  assert.equal(result.sourceChecks,official.REQUIRED_CHECKS.length);
  assert.equal(result.readyForOwnerManualPublication,true);
  assert.equal(result.alreadyPublished,false);
  assert.equal(result.writesPerformed,0);
  assert.equal(result.nativeUnrealQualified,false);
  assert.equal(result.playableWin64Qualified,false);
  assert.equal(f.state.branchCalls,2);
  assert.ok(!f.state.apiCalls.some(x=>x.startsWith('create')));
});

test('existing exact source-only publication is idempotently reported without writes',async()=>{
  const f=scenario();
  f.state.newTag={object:{type:'commit',sha:SHA}};
  f.state.newRelease={
    tag_name:guard.TAG,target_commitish:SHA,name:guard.NAME,
    body:NOTES,assets:[],draft:false,prerelease:true,
    published_at:'2026-11-17T11:30:00Z',
  };
  const out=await f.run();
  assert.equal(out.readyForOwnerManualPublication,false);
  assert.equal(out.alreadyPublished,true);
  assert.equal(out.writesPerformed,0);
});

const rejects=[
  ['owner actor mismatch',f=>{f.context.actor='other';},
    /Owner manual/],
  ['wrong repository',f=>{f.context.repo.repo='something-else';},
    /Owner manual/],
  ['non-main ref',f=>{f.context.ref='refs/heads/feature';},
    /Owner manual/],
  ['automated event',f=>{f.context.eventName='push';},
    /Owner manual/],
  ['wrong typed SHA',f=>{f.context.payload.inputs.candidate_sha='b'.repeat(40);},
    /Owner manual/],
  ['unprotected main',f=>{f.state.protected=false;},
    /current protected-main/],
  ['main moves',f=>{f.state.current='c'.repeat(40);},
    /current protected-main/],
  ['remote notes drift',f=>{f.state.notes+='extra';},
    /remote protected-main source bytes/],
  ['source PR not actually merged',f=>{f.state.merged=false;},
    /not confirmed merged/],
  ['unrelated fake source ancestry',f=>{f.state.sourceIncluded=false;},
    /does not include/],
  ['draft 3D gameplay merged without updated release scope',
    f=>{f.state.gameplayMerged.add(377);}, /scope changed/],
  ['draft reverse gameplay merged without updated release scope',
    f=>{f.state.gameplayMerged.add(391);}, /scope changed/],
  ['draft brake gameplay merged without updated release scope',
    f=>{f.state.gameplayMerged.add(394);}, /scope changed/],
  ['old tag retargeted',f=>{f.state.oldTag.object.sha=SHA;},
    /Historical test source tag moved/],
  ['old release with attached asset',f=>{f.state.oldRelease.assets=[{name:'game.exe'}];},
    /zero attachments/],
  ['old test source not an ancestor',f=>{f.state.historicAncestor=false;},
    /new descendant/],
  ['missing required cross-platform job',f=>{f.state.checks.pop();},
    /Required exact-head check/],
  ['latest C++ source job pending',f=>{f.state.checks[0].status='in_progress';},
    /not successful/],
  ['latest C++ source job failure',f=>{f.state.checks[0].conclusion='failure';},
    /not successful/],
  ['foreign forged check app',f=>{f.state.checks[0].app.slug='other';},
    /untrusted app/],
  ['official release already tagged',f=>{f.state.officialTag={object:{type:'commit',sha:SHA}};},
    /Official v0.5.100 tag exists/],
  ['half-created candidate tag',f=>{f.state.newTag={object:{type:'commit',sha:SHA}};},
    /Partial test.2/],
  ['half-created candidate release',f=>{f.state.newRelease={tag_name:guard.TAG};},
    /Partial test.2/],
  ['wrong candidate exact tag',f=>{
    f.state.newTag={object:{type:'commit',sha:'d'.repeat(40)}};
    f.state.newRelease={tag_name:guard.TAG,body:NOTES,name:guard.NAME,
      target_commitish:SHA,assets:[],draft:false,prerelease:true,
      published_at:'2026-11-17T11:30:00Z'};
  }, /exact immutable source tag SHA/],
];
for(const [label,change,expression] of rejects) {
  test('reject '+label,async()=>{
    const f=scenario();
    change(f);
    await assert.rejects(f.run(),expression);
  });
}

test('missing required note phrases is not bypassed by source CI',()=>{
  assert.throws(()=>guard.checkNotes(NOTES.replace('zero executable binaries','game available')),
    /warning missing/);
  assert.throws(()=>guard.checkNotes(NOTES.replace('PR #395','PR #999')),
    /notes omit/);
});
