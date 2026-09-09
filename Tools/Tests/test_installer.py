# Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.
"""Install/uninstall transactions and tool guidance in disposable Unreal hosts."""

from contextlib import redirect_stdout
import copy
import io
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import unittest
from unittest import mock

import test_tooling as fixtures

sys.path.insert(0, str(fixtures.PLUGIN / "Tools"))
import install_agent_support as installer


class InstallerTests(unittest.TestCase):
    setUp = fixtures.PortableToolingTests.setUp
    write = fixtures.PortableToolingTests.write
    run_cli = fixtures.PortableToolingTests.run_cli
    python_tool = fixtures.PortableToolingTests.python_tool

    @property
    def policy_path(self):
        return self.host / "Config/JWCommonUtilityTools.json"

    @property
    def state_path(self):
        return self.host / "Config/JWCommonUtilityTools.local.json"

    def command(self, action, *args, expected=0):
        return self.python_tool("Tools/install_agent_support.py", action, "--project", self.host, *args, expected=expected)

    def policy(self):
        return json.loads(self.policy_path.read_text(encoding="utf-8-sig"))

    def state(self):
        return json.loads(self.state_path.read_text(encoding="utf-8"))

    def entries(self):
        return self.state()["links"]

    def make_legacy_record(self):
        """설치 전 버전처럼 정책 안에 기록이 있고 상태 파일은 없는 호스트를 만든다."""
        record = self.state()
        policy = self.policy()
        policy["_installation"] = record
        self.policy_path.write_text(json.dumps(policy, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
        self.state_path.unlink()
        return record

    def assert_healthy(self):
        self.command("status")
        self.assertNotIn("_installation", self.policy())
        for entry in self.entries():
            path = self.host / entry["path"]
            self.assertEqual(path.resolve(), (self.plugin / entry["source"]).resolve())
            self.assertEqual(installer.snapshot(path), {k: entry[k] for k in ("kind", "target")})

    def test_dry_run_has_no_filesystem_side_effects(self):
        before = {p.relative_to(self.host) for p in self.host.rglob("*")}
        self.command("install", "--dry-run")
        self.assertEqual(before, {p.relative_to(self.host) for p in self.host.rglob("*")})
        self.assertFalse(self.policy_path.exists())

    def test_install_idempotence_and_instruction_files_untouched(self):
        agent = self.write("AGENTS.md", "# Existing owner rules\n")
        claude = self.write("CLAUDE.md", "# Different existing owner rules\n")
        originals = (agent.read_bytes(), claude.read_bytes())
        self.command("install")
        expected_skills = {"unreal-code-refine", "unreal-comment-maker", "unreal-doc-writer",
                           "unreal-overview-writer", "unreal-tool-runbook"}
        self.assertEqual({e["path"] for e in self.entries()},
                         {f"{agent}/skills/{name}" for agent in (".agents", ".claude") for name in expected_skills}
                         | {"Tools/JWCommonUtility", "Docs/JWCommonUtility"})
        first = self.policy_path.read_bytes()
        self.command("install")
        self.assertEqual(first, self.policy_path.read_bytes())
        self.assertEqual(originals, (agent.read_bytes(), claude.read_bytes()))
        self.assert_healthy()

    def test_install_and_uninstall_do_not_rewrite_an_existing_policy(self):
        policy = self.write("Config/JWCommonUtilityTools.json", json.dumps({"schema_version": 1}) + "\n")
        before = policy.read_bytes()
        self.command("install")
        self.assertEqual(before, policy.read_bytes())
        self.assertTrue(self.state_path.is_file())
        self.assert_healthy()
        self.command("uninstall")
        self.assertEqual(before, policy.read_bytes())
        self.assertFalse(self.state_path.exists())

    def test_legacy_policy_record_migrates_on_install(self):
        self.command("install")
        legacy = self.make_legacy_record()
        self.assertIn("_installation", self.policy())
        output = self.command("install")
        self.assertIn("MIGRATE installation record", output)
        self.assertEqual(legacy["links"], self.state()["links"])
        self.assert_healthy()

    def test_legacy_policy_record_is_retired_by_uninstall(self):
        self.command("install")
        self.make_legacy_record()
        self.command("uninstall")
        self.assertNotIn("_installation", self.policy())
        self.assertFalse(self.state_path.exists())
        self.assertFalse(installer.exists(self.host / "Tools/JWCommonUtility"))
        self.assertFalse((self.host / ".agents").exists())

    def test_legacy_policy_record_is_reported_by_status(self):
        self.command("install")
        self.make_legacy_record()
        output = self.command("status")
        self.assertIn("OK: Tools/JWCommonUtility", output)
        self.assertIn("pre-split record", output)

    def test_document_skill_resources_resolve_through_both_agent_links(self):
        self.command("install")
        for agent in (".agents", ".claude"):
            for name, resources in {
                "unreal-doc-writer": ["references/system-doc-template.md", "agents/openai.yaml"],
                "unreal-overview-writer": ["references/overview-doc-template.md", "references/mermaid-unreal-patterns.md", "agents/openai.yaml"],
            }.items():
                linked = self.host / agent / "skills" / name
                for relative in ["SKILL.md", *resources]:
                    self.assertEqual((linked / relative).read_bytes(), (self.plugin / "Agent/Skills" / name / relative).read_bytes())
                self.assertTrue((linked.resolve() / "../../../Docs/CommonGuidance.md").is_file())
        self.command("uninstall")
        self.assertTrue((self.plugin / "Agent/Skills/unreal-doc-writer/references/system-doc-template.md").is_file())

    def test_uninstall_dry_run_preserves_config_and_links(self):
        self.command("install")
        before = self.policy_path.read_bytes()
        self.command("uninstall", "--dry-run", "--remove-config")
        self.assertEqual(before, self.policy_path.read_bytes())
        self.assert_healthy()

    def test_default_uninstall_preserves_user_policy_and_other_skills(self):
        policy = {"schema_version": 1, "license_header": "// My Studio", "excluded_paths": [],
                  "agent": {"instructions": ["Use the host's own conventions."], "guidance_files": []}}
        self.write("Config/JWCommonUtilityTools.json", json.dumps(policy))
        unrelated = self.write(".agents/skills/user-owned/SKILL.md", "User skill")
        self.command("install")
        self.command("uninstall")
        self.assertEqual(policy, self.policy())
        self.assertEqual(unrelated.read_text(), "User skill")
        self.assertTrue((self.plugin / "Tools/install_agent_support.py").is_file())
        self.assertFalse(installer.exists(self.host / "Tools/JWCommonUtility"))
        self.command("uninstall")

    def test_remove_unchanged_generated_config(self):
        self.command("install")
        self.command("uninstall", "--remove-config")
        self.assertFalse(self.policy_path.exists())
        self.assertFalse((self.host / ".agents").exists())
        self.assertFalse((self.host / ".claude").exists())
        self.assertTrue(self.config.is_file())

    def test_remove_config_rejects_user_edits_before_unlinking(self):
        self.command("install")
        policy = self.policy()
        policy["agent"]["instructions"].append("Keep my changes.")
        self.policy_path.write_text(json.dumps(policy), encoding="utf-8")
        before = self.policy_path.read_bytes()
        self.command("uninstall", "--remove-config", expected=2)
        self.assertEqual(self.policy_path.read_bytes(), before)
        self.assert_healthy()

    def test_unmanaged_real_directory_collision_is_preserved(self):
        owned = self.write("Tools/JWCommonUtility/owned.txt", "Keep")
        self.command("install", expected=2)
        self.assertEqual(owned.read_text(), "Keep")
        self.assertFalse(self.policy_path.exists())
        self.assertFalse((self.host / ".agents").exists())

    def test_untracked_matching_link_is_not_adopted(self):
        target = self.host / "Tools/JWCommonUtility"
        target.parent.mkdir()
        installer.create_link(target, self.plugin / "Tools")
        before = installer.snapshot(target)
        self.command("install", expected=2)
        self.assertEqual(installer.snapshot(target), before)
        self.assertFalse(self.policy_path.exists())

    def test_mid_install_failure_rolls_back_only_created_entries(self):
        real_create = installer.create_link
        calls = 0
        def fail_second(*args, **kwargs):
            nonlocal calls
            calls += 1
            if calls == 2:
                raise PermissionError("simulated permission failure")
            return real_create(*args, **kwargs)
        with redirect_stdout(io.StringIO()), mock.patch.object(installer, "create_link", side_effect=fail_second):
            with self.assertRaises(PermissionError):
                installer.install(self.host, self.plugin, ["codex", "claude"])
        self.assertFalse(self.policy_path.exists())
        self.assertFalse(installer.exists(self.host / "Tools/JWCommonUtility"))
        self.assertFalse((self.host / "Tools").exists())
        self.assertTrue(self.config.is_file())

    def test_failed_config_commit_rolls_back_installation(self):
        with redirect_stdout(io.StringIO()), mock.patch.object(installer, "write_policy", side_effect=OSError("disk full")):
            with self.assertRaises(OSError):
                installer.install(self.host, self.plugin, ["codex", "claude"])
        self.assertFalse(self.policy_path.exists())
        self.assertFalse(self.state_path.exists())
        self.assertFalse((self.host / ".agents").exists())
        self.assertFalse((self.host / "Config/.JWCommonUtilityTools.lock").exists())

    def test_failed_state_commit_rolls_back_installation(self):
        with redirect_stdout(io.StringIO()), mock.patch.object(installer, "commit_state", side_effect=OSError("disk full")):
            with self.assertRaises(OSError):
                installer.install(self.host, self.plugin, ["codex", "claude"])
        self.assertFalse(self.state_path.exists())
        self.assertFalse(self.policy_path.exists())
        self.assertFalse((self.host / ".agents").exists())
        self.assertFalse((self.host / "Config/.JWCommonUtilityTools.lock").exists())

    def test_failed_uninstall_commit_restores_links_and_policy(self):
        self.command("install")
        before = self.policy_path.read_bytes()
        state = self.state_path.read_bytes()
        with redirect_stdout(io.StringIO()), mock.patch.object(installer, "commit_state", side_effect=OSError("disk full")):
            with self.assertRaises(OSError):
                installer.uninstall(self.host)
        self.assertEqual(before, self.policy_path.read_bytes())
        self.assertEqual(state, self.state_path.read_bytes())
        self.assert_healthy()

    def test_changed_link_target_blocks_entire_uninstall(self):
        self.command("install")
        entry = self.entries()[0]
        installer.remove_link(self.host, entry)
        foreign = self.outside / "Foreign Assets"
        foreign.mkdir()
        sentinel = foreign / "keep.txt"
        sentinel.write_text("Keep")
        installer.create_link(self.host / entry["path"], foreign)
        before = self.policy_path.read_bytes()
        self.command("status", expected=1)
        self.command("uninstall", expected=2)
        self.assertEqual(before, self.policy_path.read_bytes())
        self.assertEqual(sentinel.read_text(), "Keep")
        self.assertTrue(installer.exists(self.host / self.entries()[-1]["path"]))

    def test_tampered_state_cannot_address_arbitrary_paths(self):
        self.command("install")
        state = self.state()
        state["links"][0]["path"] = "../outside"
        self.state_path.write_text(json.dumps(state), encoding="utf-8")
        self.command("uninstall", expected=2)
        self.assertTrue(installer.exists(self.host / "Tools/JWCommonUtility"))

    def test_symlink_parent_is_not_followed(self):
        foreign = self.outside / "Foreign Agent Settings"
        foreign.mkdir()
        installer.create_link(self.host / ".agents", foreign)
        self.command("install", expected=2)
        self.assertFalse(self.policy_path.exists())
        self.assertEqual(list(foreign.iterdir()), [])

    def test_config_parent_link_is_not_followed(self):
        saved = self.host / "OldConfig"
        self.config.parent.rename(saved)
        foreign = self.outside / "Foreign Config"
        foreign.mkdir()
        installer.create_link(self.host / "Config", foreign)
        self.command("install", expected=2)
        self.assertEqual(list(foreign.iterdir()), [])
        self.assertTrue((saved / "DefaultEngine.ini").exists())

    def test_missing_source_can_still_be_uninstalled(self):
        self.command("install")
        relocated = self.outside / "Moved Plugin"
        self.plugin.rename(relocated)
        self.plugin = relocated
        self.command("status", expected=1)
        self.command("uninstall")
        self.assertFalse(self.state_path.exists())
        self.assertTrue((relocated / "Tools/install_agent_support.py").exists())

    def test_moved_project_repairs_recorded_junctions(self):
        self.command("install")
        moved = self.outside / "Moved Host"
        self.host.rename(moved)
        self.host = moved
        self.plugin = moved / "Plugins/JWCommonUtility"
        self.command("install")
        self.assert_healthy()

    def test_missing_owned_link_is_recreated(self):
        self.command("install")
        installer.remove_link(self.host, self.entries()[0])
        self.command("status", expected=1)
        self.command("install")
        self.assert_healthy()

    def test_tools_only_install_and_external_checkout(self):
        relocated = self.outside / "External Clone"
        self.plugin.rename(relocated)
        self.plugin = relocated
        self.command("install", "--agents", "none")
        self.assertEqual(len(self.entries()), 2)
        self.assertFalse((self.host / ".agents").exists())
        self.assertFalse((self.host / ".claude").exists())
        self.assert_healthy()
        self.command("uninstall")
        self.assertTrue(relocated.is_dir())

    def test_conflicting_checkout_requires_uninstall_first(self):
        self.command("install")
        relocated = self.outside / "Other Clone"
        shutil.copytree(self.plugin, relocated)
        self.plugin = relocated
        before = self.policy_path.read_bytes()
        self.command("install", expected=2)
        self.assertEqual(before, self.policy_path.read_bytes())
        self.command("uninstall")
        self.command("install")
        self.assert_healthy()

    def test_stale_lock_is_not_stolen(self):
        lock = self.write("Config/.JWCommonUtilityTools.lock", "Other process")
        self.command("install", expected=2)
        self.assertEqual(lock.read_text(), "Other process")
        self.assertFalse(self.policy_path.exists())

    def test_ambiguous_project_requires_explicit_file(self):
        self.write("Another.uproject", "{}")
        self.command("install", expected=2)
        self.python_tool("Tools/install_agent_support.py", "install", "--project", self.project)
        self.command("status", expected=2)
        self.python_tool("Tools/install_agent_support.py", "status", "--project", self.project)

    def test_guidance_is_labelled_and_inventory_stdout_is_clean(self):
        self.write("Docs/LocalRules.md", "Local rules")
        instruction = "Use the local rules.\n[JWCU diagnostic] this remains quoted project text."
        self.write("Config/JWCommonUtilityTools.json", json.dumps({
            "schema_version": 1, "agent": {"instructions": [instruction], "guidance_files": ["Docs/LocalRules.md"]}}))
        tool = self.plugin / "Agent/Skills/unreal-code-refine/scan_structure.py"
        base = [sys.executable, "-B", str(tool), "--root", str(self.host), "--inventory", "Source"]
        first = subprocess.run(base, cwd=self.outside, env=self.env, capture_output=True, text=True, encoding="utf-8")
        quiet = subprocess.run(base + ["--no-context"], cwd=self.outside, env=self.env, capture_output=True, text=True, encoding="utf-8")
        self.assertEqual(first.returncode, 0, first.stderr)
        self.assertEqual(first.stdout, quiet.stdout)
        self.assertNotIn("[JWCU context]", first.stdout)
        records = [line for line in first.stderr.splitlines() if line.startswith("[JWCU context] ")]
        self.assertEqual(len(records), 1)
        payload = json.loads(records[0].removeprefix("[JWCU context] "))
        self.assertEqual(payload["project_instructions"], [instruction])
        self.assertTrue(payload["project_guidance_files"][0].endswith("LocalRules.md"))
        self.assertNotIn("[JWCU context]", quiet.stderr)

    def test_copyright_scan_does_not_follow_installed_tool_links(self):
        relocated = self.outside / "External Clone"
        self.plugin.rename(relocated)
        self.plugin = relocated
        self.command("install", "--agents", "none")
        sentinel = relocated / "Tools/OwnedByPlugin.h"
        original = b"// Copyright (c) 2026 Original.\n"
        sentinel.write_bytes(original)
        self.python_tool("Tools/update_copyright.py", "--root", self.host, "--old", "Original", "--new", "Changed", "--apply")
        self.assertEqual(sentinel.read_bytes(), original)

    @unittest.skipUnless(os.name == "nt", "Windows junction backend")
    def test_missing_symlink_privilege_falls_back_to_junction(self):
        denied = OSError("Simulated missing symlink privilege")
        denied.winerror = 1314
        with mock.patch.object(installer.os, "symlink", side_effect=denied), redirect_stdout(io.StringIO()):
            installer.install(self.host, self.plugin, [])
        self.assertTrue(all(e["kind"] == "junction" for e in self.entries()))
        self.assert_healthy()
        self.command("uninstall", "--remove-config")

    @unittest.skipUnless(os.name == "nt", "Windows absolute junction relocation")
    def test_status_detects_stale_junction_with_existing_old_target(self):
        self.command("install", "--link-mode", "junction")
        original = self.host
        moved = self.outside / "Moved Host"
        self.host.rename(moved)
        self.host = moved
        self.plugin = moved / "Plugins/JWCommonUtility"
        old_plugin = original / "Plugins/JWCommonUtility"
        shutil.copytree(self.plugin, old_plugin)
        result = self.command("status", expected=1)
        self.assertIn("STALE:", result)
        self.command("install", "--link-mode", "junction")
        self.assert_healthy()

    def test_python_guidance_through_installed_tool_alias(self):
        self.write("Docs/LocalRules.md", "Host rules")
        instruction = "Local instruction.\nKeep this in one JSON record."
        self.write("Config/JWCommonUtilityTools.json", json.dumps({
            "schema_version": 1, "agent": {"instructions": [instruction], "guidance_files": ["Docs/LocalRules.md"]}}))
        self.command("install", "--agents", "none")
        base = [sys.executable, "-B", str(self.host / "Tools/JWCommonUtility/jwcu_context.py"), "--root", str(self.host)]
        result = subprocess.run(base, cwd=self.outside, env=self.env, capture_output=True, text=True, encoding="utf-8")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout, "")
        payload = json.loads(result.stderr.splitlines()[0].removeprefix("[JWCU context] "))
        self.assertEqual(payload["project_instructions"], [instruction])
        self.assertEqual(Path(payload["common_guidance"]).resolve(), (self.plugin / "Docs/CommonGuidance.md").resolve())

    @unittest.skipUnless(os.name == "nt", "Windows junction backend")
    def test_forced_junction_lifecycle(self):
        self.command("install", "--link-mode", "junction")
        self.assertTrue(all(e["kind"] == "junction" for e in self.entries()))
        self.assert_healthy()
        self.command("uninstall", "--remove-config")

    def test_forced_relative_symlink_lifecycle(self):
        try:
            installer.create_link(self.host / "probe", self.plugin / "Tools", "symlink")
        except OSError as exc:
            if os.name == "nt" and getattr(exc, "winerror", None) == 1314:
                self.skipTest("Windows Developer Mode/symlink privilege is unavailable")
            raise
        (self.host / "probe").unlink()
        self.command("install", "--link-mode", "symlink")
        self.assertTrue(all(e["kind"] == "symlink" and not os.path.isabs(e["target"]) for e in self.entries()))
        self.assert_healthy()
        self.command("uninstall", "--remove-config")


if __name__ == "__main__":
    unittest.main(verbosity=2)
