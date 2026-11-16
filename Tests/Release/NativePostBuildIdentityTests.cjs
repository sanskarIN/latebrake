'use strict';
// Exercise the ACTUAL inline github-script from the manual UE workflow.
// Entirely offline: no network, no Unreal SDK, no PR code or release writes.
const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');

const text = fs.readFileSync(path.resolve(
  __dirname, '../../.github/workflows/validate-draft-unreal-pr.yml'
), 'utf8');
const jobSection = text.split('\n  postbuild-source-identity:\n');
assert.equal(jobSection.length, 2,
  'Exactly one separate source-free hosted post-build job must exist');
const marker = '\n          script: |\n';
const scriptSection = jobSection[1].split(marker);
assert.equal(scriptSection.length, 2,
  'The GitHub-hosted job must include a single github-script body');
const script = scriptSection[1].split('\n').map(line =>
  line.startsWith('            ') ? line.slice(12) : line
).join('\n');
const SHA = 'a'.repeat(40);

async function evaluate(change = () => {}) {
  const ctx = {
    repo: {owner:'sanskarIN',repo:'latebrake'},
    actor:'sanskarIN',eventName:'workflow_dispatch',ref:'refs/heads/main',
  };
  const env = {
    NATIVE_VERIFIED_SHA:SHA,REQUESTED_SHA:SHA,REQUESTED_PR:'377',
    NATIVE_VALIDATION:'Win64',
  };
  const pr = {
    state:'open',base:{ref:'main'},
    head:{sha:SHA,repo:{full_name:'sanskarIN/latebrake'},
      user:{login:'sanskarIN'}},
  };
  change(ctx,env,pr);
  const seen = {calls:0,notices:[],summary:[],queried:null};
  const github = {rest:{pulls:{get:async args=>{
    seen.calls++;
    seen.queried = args;
    return {data:pr};
  }}}};
  const summary = {
    addHeading(value){seen.summary.push(value);return this;},
    addRaw(value){seen.summary.push(value);return this;},
    async write(){seen.summary.push('written');},
  };
  const core = {summary,notice(value){seen.notices.push(value);}};
  // V8 sandbox receives only the read-only fake API and local values.
  // It never receives process tokens, filesystem, network, or require.
  await vm.runInNewContext('(async()=>{\n'+script+'\n})()',
    {github,core,context:ctx,process:{env}},
    {timeout:1500,filename:'postbuild-source-identity.workflow.js'});
  return seen;
}

test('valid current owner PR is accepted only as a source identity handoff',async()=>{
  const output = await evaluate();
  // The VM deliberately gives the API argument a separate prototype.
  // Compare the actual submitted primitives, not realm-specific prototypes.
  assert.equal(output.queried.owner, 'sanskarIN');
  assert.equal(output.queried.repo, 'latebrake');
  assert.equal(output.queried.pull_number, 377);
  assert.equal(output.calls,1);
  assert.match(output.summary.join(' '),/No installed gameplay/);
  assert.ok(output.summary.join(' ').includes(SHA));
  assert.equal(output.notices.length,1);
});

test('uppercase review SHA is normalized, built SHA must remain lowercase',async()=>{
  const seen = await evaluate((ctx,env)=>{env.REQUESTED_SHA=SHA.toUpperCase();});
  assert.equal(seen.calls,1);
});

for (const [label,change] of [
  ['current PR head changed during the native build',
    (ctx,env,pr)=>{pr.head.sha='b'.repeat(40);}],
  ['PR was closed while compiling',(ctx,env,pr)=>{pr.state='closed';}],
  ['PR base was retargeted',(ctx,env,pr)=>{pr.base.ref='other';}],
  ['owner PR replaced with a fork',
    (ctx,env,pr)=>{pr.head.repo.full_name='outside/latebrake';}],
  ['head identity no longer belongs to owner',
    (ctx,env,pr)=>{pr.head.user.login='someoneelse';}],
]) {
  test('fail closed when '+label,async()=>{
    await assert.rejects(()=>evaluate(change),
      /PR head changed, closed, retargeted or became ineligible/);
  });
}

for (const [label,change] of [
  ['manual actor is not owner',(ctx)=>{ctx.actor='another-user';}],
  ['workflow event is automatic',(ctx)=>{ctx.eventName='pull_request_target';}],
  ['workflow dispatch was not from main',(ctx)=>{ctx.ref='refs/heads/feature';}],
  ['verified SHA differs from reviewed SHA',
    (ctx,env)=>{env.NATIVE_VERIFIED_SHA='b'.repeat(40);}],
  ['built SHA is missing',(ctx,env)=>{env.NATIVE_VERIFIED_SHA='';}],
  ['input SHA is not exact',(ctx,env)=>{env.REQUESTED_SHA='1234';}],
  ['PR number zero',(ctx,env)=>{env.REQUESTED_PR='0';}],
  ['PR number is not an integer',(ctx,env)=>{env.REQUESTED_PR='1.5';}],
  ['mode is not Editor or Win64',(ctx,env)=>{env.NATIVE_VALIDATION='Release';}],
  ['repository is different',(ctx)=>{ctx.repo.repo='other';}],
]) {
  test('deny preflight identity '+label,async()=>{
    await assert.rejects(()=>evaluate(change),
      /Built SHA, input, owner, mode or dispatch identity mismatch/);
  });
}

test('hosted recheck does not checkout or call GitHub writes',()=>{
  assert.match(jobSection[1],/runs-on: ubuntu-latest/);
  assert.match(jobSection[1],/needs: \[trusted-draft-candidate\]/);
  assert.doesNotMatch(jobSection[1],/actions\/checkout@|download-artifact@/);
  assert.doesNotMatch(script,/\.createRelease|\.createRef|\.merge\(/);
});
