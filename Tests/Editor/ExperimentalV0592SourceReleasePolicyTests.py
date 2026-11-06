"""Pinned source publisher: static wiring and offline mocked GitHub mutations."""
from pathlib import Path
import json
import shutil
import subprocess
import unittest

ROOT = Path(__file__).resolve().parents[2]
WORKFLOW = ROOT / ".github/workflows/publish-suspension-v0592-experimental.yml"
SOURCE_SHA = "13f5588b64462a6dcdcb7d7406c584c63837e87b"
PREVIOUS_SHA = "8edba49c0a6294ece2797c708d80b6dd52288d40"

NODE_HARNESS = r"""
const input=JSON.parse(require('fs').readFileSync(0,'utf8'));
const o=input.options, writes=[];
const missing=()=>{throw Object.assign(Error('not found'),{status:404});};
const sha=input.sha, main='b'.repeat(40);
const notes=input.notes, previousNotes=input.previousNotes;
const target={draft:false,prerelease:true,published_at:'2026-11-06T00:00:00Z',
  tag_name:'v0.5.92',target_commitish:sha,assets:[],body:notes,
  name:'LateBrake v0.5.92 — Suspension and Traction Refinement (SOURCE-ONLY Pre-release)',
  html_url:'https://github.com/sanskarIN/latebrake/releases/tag/v0.5.92'};
if(o.badRelease)target[o.badRelease]=o.badRelease==='assets'?[{name:'game.exe'}]:
  o.badRelease==='draft'?true:o.badRelease==='prerelease'?false:'wrong';
const pr={state:o.closed?'closed':'open',draft:!o.ready,base:{ref:'main'},
  head:{sha:o.stale?'c'.repeat(40):sha,ref:o.branch?'other':'feat/chaos-four-corner-suspension',
    repo:{full_name:o.fork?'other/repo':'sanskarIN/latebrake'},
    user:{login:o.owner?'other':'sanskarIN'}}};
const github={rest:{
  pulls:{get:async()=>({data:pr})},
  repos:{
    getBranch:async()=>({data:{commit:{sha:o.mainMoved?'d'.repeat(40):main}}}),
    getContent:async()=>({data:{encoding:'base64',content:Buffer.from(
      o.notesMismatch?'different notes':notes).toString('base64')}}),
    getReleaseByTag:async({tag})=>tag==='v0.5.91'?{data:{draft:false,prerelease:true,
      published_at:'2026-11-05T00:00:00Z',assets:o.previousAsset?[{}]:[],
      target_commitish:input.previousSha,body:previousNotes}}:
      o.existingRelease?{data:target}:missing(),
    createRelease:async args=>{writes.push({kind:'release',args});return{data:target};}
  },
  git:{
    getRef:async({ref})=>ref==='tags/v0.5.91'?{data:{object:{type:'commit',sha:input.previousSha}}}:
      o.existingTag?{data:{object:{type:'commit',sha:o.wrongTag?'e'.repeat(40):sha}}}:missing(),
    createRef:async args=>{writes.push({kind:'tag',args});return{data:{object:{type:'commit',sha}}};}
  }
}};
const context={repo:{owner:'sanskarIN',repo:'latebrake'},sha:main};
const mockRequire=name=>{
  if(name!=='fs')throw Error('unexpected module');
  return{readFileSync:path=>path==='docs/releases/v0.5.92.md'?notes:
    path==='docs/releases/v0.5.91.md'?previousNotes:(()=>{throw Error('unexpected path')})()};
};
const AsyncFunction=Object.getPrototypeOf(async function(){}).constructor;
(async()=>{
  let error=null;
  try{await new AsyncFunction('github','context','core','require','process',input.script)(
    github,context,{notice:()=>{}},mockRequire,{env:{RELEASE_COMMIT:sha}});
  }catch(e){error=e.message;}
  process.stdout.write(JSON.stringify({error,writes}));
})();
"""


def scripts():
    lines = WORKFLOW.read_text(encoding="utf-8").splitlines()
    result = []
    for i, line in enumerate(lines):
        if line.strip() == "script: |":
            body = []
            for following in lines[i + 1:]:
                if not following or following.startswith("            "):
                    body.append(following[12:] if following else "")
                else:
                    break
            result.append("\n".join(body))
    return result


class ExperimentalV0592SourceReleasePolicyTests(unittest.TestCase):
    def run_script(self, options=None, preflight=False):
        node = shutil.which("node")
        self.assertIsNotNone(node, "Node is required to test the actual publisher JavaScript")
        blocks = scripts()
        self.assertEqual(len(blocks), 2)
        payload = {"script": blocks[0 if preflight else 1], "options": options or {},
                   "sha": SOURCE_SHA, "previousSha": PREVIOUS_SHA,
                   "notes": (ROOT / "docs/releases/v0.5.92.md").read_text(encoding="utf-8"),
                   "previousNotes": (ROOT / "docs/releases/v0.5.91.md").read_text(encoding="utf-8")}
        run = subprocess.run([node, "-e", NODE_HARNESS], input=json.dumps(payload),
                             capture_output=True, text=True, check=False)
        self.assertEqual(run.returncode, 0, run.stderr)
        return json.loads(run.stdout)

    def test_complete_gates_and_immutable_source_wiring(self):
        text = WORKFLOW.read_text(encoding="utf-8")
        for marker in ("branches: [main]", "workflow_dispatch:",
                       "RELEASE_COMMIT: " + SOURCE_SHA,
                       "pull_number:295", "pr.head.sha !== candidate", "pr.head.sha !== sha",
                       "main !== context.sha", "!pr.draft",
                       "os: [windows-latest, ubuntu-latest, macos-latest]",
                       "needs: [preflight, source-tests, editor-tests, packaging-policy, documentation]",
                       "--parallel 4", "--no-tests=error", "SuspensionRayVelocity;",
                       "OpposingThrottleBraking", "TractionLimitedPropulsion",
                       "python3 -m unittest discover -s Tests/Editor",
                       "Test-PackagingPolicy.ps1", "Test-SourceProvenancePolicy.ps1",
                       "python -m mkdocs build --strict --clean",
                       "python scripts/verify_docs_site.py site"):
            self.assertIn(marker, text)
        self.assertEqual(text.count("          ref: " + SOURCE_SHA), 4)
        self.assertEqual(text.count("      contents: write"), 1)
        for forbidden in ("updateRef({", "deleteRef({", "deleteRelease({", "updateRelease({",
                          "uploadReleaseAsset({", "upload-artifact@", "push --force"):
            self.assertNotIn(forbidden, text)
        self.assertLess(text.index("if (published) assertRelease(published);"),
                        text.index("await github.rest.git.createRef({"))

    def test_source_only_notes(self):
        notes = (ROOT / "docs/releases/v0.5.92.md").read_text(encoding="utf-8")
        for marker in ("SOURCE-ONLY", "NOT a playable game build", "No compiled Win64 EXE",
                       "Android APK/AAB", "not installers", "zero executable binaries",
                       "unmerged development", "v0.5.91", "four-corner"):
            self.assertIn(marker, notes)

    def test_preflight_has_no_writes_and_rejects_stale_source(self):
        good = self.run_script(preflight=True)
        self.assertIsNone(good["error"])
        self.assertEqual(good["writes"], [])
        for option in ("closed", "ready", "stale", "fork", "owner", "branch", "mainMoved", "notesMismatch"):
            with self.subTest(option=option):
                bad = self.run_script({option: True}, preflight=True)
                self.assertIsNotNone(bad["error"])
                self.assertEqual(bad["writes"], [])

    def test_publication_and_idempotence(self):
        created = self.run_script()
        self.assertIsNone(created["error"])
        self.assertEqual([w["kind"] for w in created["writes"]], ["tag", "release"])
        tag, release = [w["args"] for w in created["writes"]]
        self.assertEqual(tag["ref"], "refs/tags/v0.5.92")
        self.assertEqual(tag["sha"], SOURCE_SHA)
        self.assertEqual(release["target_commitish"], SOURCE_SHA)
        self.assertTrue(release["prerelease"])
        self.assertFalse(release["draft"])
        self.assertEqual(release["make_latest"], "false")
        repeat = self.run_script({"existingTag": True, "existingRelease": True})
        self.assertIsNone(repeat["error"])
        self.assertEqual(repeat["writes"], [])
        tag_only = self.run_script({"existingTag": True})
        self.assertIsNone(tag_only["error"])
        self.assertEqual([w["kind"] for w in tag_only["writes"]], ["release"])

    def test_rejections_happen_before_any_public_mutation(self):
        variants = [{option: True} for option in
                    ("closed", "ready", "stale", "fork", "owner", "branch", "mainMoved",
                     "notesMismatch", "previousAsset")]
        variants += [{"existingTag": True, "wrongTag": True}, {"existingRelease": True}]
        variants += [{"existingTag": True, "existingRelease": True, "badRelease": field}
                     for field in ("draft", "prerelease", "target_commitish", "tag_name",
                                   "body", "name", "assets", "published_at")]
        for options in variants:
            with self.subTest(options=options):
                result = self.run_script(options)
                self.assertIsNotNone(result["error"])
                self.assertEqual(result["writes"], [])


if __name__ == "__main__":
    unittest.main()
