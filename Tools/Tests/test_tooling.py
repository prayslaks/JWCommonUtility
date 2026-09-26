# Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT
"""Portable CLI integration tests; all mutations use a temporary host project."""

import codecs
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest


PLUGIN = Path(__file__).resolve().parents[2]
HEADER = """#pragma once
#include "CustomPayload.h"
/** A ship used by the portability fixture. */
UCLASS()
class ACustomShip : public AActor
{
    GENERATED_BODY()
    /** Optional payload. */
    UPROPERTY()
    FCustomPayload* Payload;
};
"""
CPP = """#include "CustomShip.h"
void ACustomShip::Tick(float DeltaTime)
{
    Advance(DeltaTime);
}
"""

# UHT 가 요구하는 배치: 전방 선언 뒤에 *.generated.h 만 온다.
GENERATED_ORDER_OK = """#pragma once
#include "CoreMinimal.h"
class UCustomPayload;
#include "CustomBeacon.generated.h"
/** A beacon used by the include order fixture. */
UCLASS()
class ACustomBeacon : public AActor
{
    GENERATED_BODY()
};
"""

# 전방 선언 뒤에 일반 include 가 이어지는 실제 순서 위반.
GENERATED_ORDER_BAD = GENERATED_ORDER_OK.replace(
    '#include "CustomBeacon.generated.h"',
    '#include "CustomPayload.h"\n#include "CustomBeacon.generated.h"',
).replace("CustomBeacon", "CustomRelay").replace("ACustomRelay", "ACustomRelay")


class PortableToolingTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="jwcu-tooling-")
        self.addCleanup(self.temp.cleanup)
        self.outside = Path(self.temp.name)
        self.host = self.outside / "Different Host"
        self.host.mkdir()
        self.project = self.host / "DifferentGame.uproject"
        self.project.write_text('{"FileVersion":3,"EngineAssociation":"5.7"}', encoding="utf-8")
        self.plugin = self.host / "Plugins" / "JWCommonUtility"
        shutil.copytree(PLUGIN, self.plugin, ignore=shutil.ignore_patterns("Binaries", "Intermediate", "__pycache__", "Tests"))
        self.header = self.write("Source/NewFlight/Public/CustomShip.h", HEADER)
        self.cpp = self.write("Source/NewFlight/Private/CustomShip.cpp", CPP)
        self.config = self.write("Config/DefaultEngine.ini", "[/Script/Engine.Engine]\n")
        self.env = dict(os.environ, PYTHONIOENCODING="utf-8", PYTHONUTF8="1", PYTHONDONTWRITEBYTECODE="1")

    def write(self, relative, content):
        path = self.host / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding="utf-8", newline="")
        return path

    def run_cli(self, argv, expected=0):
        result = subprocess.run([str(a) for a in argv], cwd=self.outside, env=self.env,
                                capture_output=True, text=True, encoding="utf-8", timeout=30)
        self.assertEqual(result.returncode, expected, result.stdout + result.stderr)
        return result.stdout + result.stderr

    def python_tool(self, relative, *args, expected=0):
        return self.run_cli([sys.executable, "-B", self.plugin / relative, *args], expected)

    def test_structure_scans_custom_prefix_from_outside_host(self):
        output = self.python_tool("Agent/Skills/unreal-code-refine/scan_structure.py",
                                  "--root", self.host, "Source")
        self.assertIn("CustomPayload.h", output)
        self.assertIn("FCustomPayload", output)
        inventory = self.python_tool("Agent/Skills/unreal-code-refine/scan_structure.py",
                                     "--root", self.host, "--inventory", "Source")
        self.assertIn("ACustomShip::Tick", inventory)

    def test_each_skill_runs_through_project_links(self):
        self.python_tool("Tools/install_agent_support.py", "install", "--project", self.host)
        for name, script in (("unreal-code-refine", "scan_structure.py"),
                             ("unreal-comment-maker", "check_comments.py")):
            linked = self.host / ".agents/skills" / name
            self.run_cli([sys.executable, "-B", linked / script, "--root", self.host, "Source"])
        self.python_tool("Tools/install_agent_support.py", "uninstall", "--project", self.host)

    def test_comment_policy_and_exact_path_exclusions(self):
        tool = "Agent/Skills/unreal-comment-maker/check_comments.py"
        output = self.python_tool(tool, "--root", self.host, "Source")
        self.assertIn("LICENSE", output)  # Explicitly reports that this check is disabled.
        self.write("Config/JWCommonUtilityTools.json", json.dumps({
            "schema_version": 1, "license_header": "// Example Studio",
            "excluded_paths": ["Source/NewFlight/Public/CustomShip.h"]}))
        self.write("Source/Other/Public/CustomShip.h", HEADER)
        output = self.python_tool(tool, "--root", self.host, "Source", expected=1)
        self.assertIn("Source/Other/Public/CustomShip.h:1", output)
        self.assertNotIn("Source/NewFlight/Public/CustomShip.h:1", output)
        self.assertIn("Source/NewFlight/Private/CustomShip.cpp:1", output)

    def test_generated_include_is_not_an_order_violation(self):
        tool = "Agent/Skills/unreal-comment-maker/check_comments.py"
        self.write("Source/NewFlight/Public/CustomBeacon.h", GENERATED_ORDER_OK)
        self.write("Source/NewFlight/Public/CustomRelay.h", GENERATED_ORDER_BAD)
        output = self.python_tool(tool, "--root", self.host, "Source")
        self.assertNotIn("CustomBeacon.h", output)
        self.assertIn("CustomRelay.h", output)
        self.assertIn("ORDER", output)

    def write_owned_policy(self):
        self.write("Config/JWCommonUtilityTools.json", json.dumps({
            "schema_version": 1, "license_header": "// Example Studio",
            "owned_paths": ["Source", "Plugins/JWCommonUtility"]}))

    def test_owned_paths_exclude_external_code_from_every_check(self):
        vendor = self.write("Plugins/OtherVendor/Source/Public/VendorShip.h",
                            HEADER.replace("    /** Optional payload. */\n", ""))
        self.write_owned_policy()
        comments = "Agent/Skills/unreal-comment-maker/check_comments.py"
        output = self.python_tool(comments, "--root", self.host, "Source", "Plugins", expected=1)
        self.assertIn("Source/NewFlight/Public/CustomShip.h:1", output)
        self.assertNotIn("VendorShip.h", output)
        # 구조 검사도 같은 범위를 따른다.
        structure = self.python_tool("Agent/Skills/unreal-code-refine/scan_structure.py",
                                     "--root", self.host, "Source", "Plugins")
        self.assertNotIn("VendorShip.h", structure)
        # 직접 손봐야 할 때만 명시적으로 켠다.
        output = self.python_tool(comments, "--root", self.host, "Plugins", "--include-external", expected=1)
        self.assertIn("VendorShip.h", output)
        self.assertEqual(vendor.read_text(encoding="utf-8").splitlines()[0], "#pragma once")

    def test_owned_paths_protect_external_copyright(self):
        vendor = self.write("Plugins/OtherVendor/Source/Public/VendorShip.h",
                            "// Copyright (c) 2024 OtherStudio. All rights reserved.\n#pragma once\n")
        self.write_owned_policy()
        tool = "Tools/update_copyright.py"
        args = ("--root", self.host, "--old", "OtherStudio", "--new", "NewOwner", "--apply")
        self.python_tool(tool, *args)
        self.assertIn("OtherStudio", vendor.read_text(encoding="utf-8"))
        self.python_tool(tool, *args, "--include-external")
        self.assertIn("NewOwner", vendor.read_text(encoding="utf-8"))

    def test_invalid_inputs_and_config_are_errors(self):
        for tool in ("Agent/Skills/unreal-code-refine/scan_structure.py",
                     "Agent/Skills/unreal-comment-maker/check_comments.py",
                     "Tools/detect_missing_comments.py"):
            self.python_tool(tool, "--root", self.host, "MissingSource", expected=2)
        self.python_tool("Tools/update_copyright.py", "--root", self.host / "Missing",
                         "--old", "Old", "--new", "New", expected=2)
        self.write("Config/JWCommonUtilityTools.json", '{"schema_version":1,"excluded_paths":"wrong"}')
        self.python_tool("Agent/Skills/unreal-comment-maker/check_comments.py",
                         "--root", self.host, "Source", expected=2)

    def test_simple_comment_scanner_accepts_host_paths(self):
        self.python_tool("Tools/detect_missing_comments.py", "--root", self.host, "Source")
        self.header.write_text(HEADER.replace("    /** Optional payload. */\n", ""), encoding="utf-8")
        output = self.python_tool("Tools/detect_missing_comments.py", "--root", self.host, "Source", expected=1)
        self.assertIn("UPROPERTY", output)

    def test_copyright_preview_apply_and_insertion_boundary(self):
        original = codecs.BOM_UTF8 + b"// Copyright (c) 2024 OldOwner. All rights reserved.\r\n#pragma once\r\n"
        self.header.write_bytes(original)
        self.write("Source/NoHeader.h", "#pragma once\n")
        sibling = self.write("SourceExtra/NoHeader.h", "#pragma once\n")
        tool = "Tools/update_copyright.py"
        args = ("--root", self.host, "--old", "OldOwner", "--new", "NewOwner")
        self.python_tool(tool, *args)
        self.assertEqual(self.header.read_bytes(), original)
        self.python_tool(tool, *args, "--apply")
        self.assertEqual(self.header.read_bytes(), original.replace(b"OldOwner", b"NewOwner"))
        self.python_tool(tool, *args, "--normalize", "--add-missing", "--add-missing-under", "Source", "--apply")
        self.assertEqual(sibling.read_bytes(), b"#pragma once\n")
        inserted = (self.host / "Source/NoHeader.h").read_bytes()
        self.assertIn(b"NewOwner", inserted)
        # 헤더와 본문 사이에 빈 줄 한 줄을 둔다.
        self.assertEqual(inserted.decode("utf-8").splitlines()[1:3], ["", "#pragma once"])
        spaced = self.write("Source/SpacedHeader.h", "\n#pragma once\n")
        self.python_tool(tool, *args, "--normalize", "--add-missing", "--add-missing-under", "Source", "--apply")
        self.assertEqual(spaced.read_text(encoding="utf-8").splitlines()[1:3], ["", "#pragma once"])
        after = self.header.read_bytes()
        self.assertTrue(after.startswith(codecs.BOM_UTF8))
        self.assertNotIn(b"\n", after.replace(b"\r\n", b""))



if __name__ == "__main__":
    unittest.main(verbosity=2)
