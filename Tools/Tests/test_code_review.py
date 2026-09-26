# Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT
"""Observable review hints and consistent policy across public CLI entry points."""

import json
from pathlib import Path
import subprocess
import sys
import unittest

import test_tooling as fixtures
sys.path.insert(0, str(fixtures.PLUGIN / "Tools"))
from cpp_review import guard_log_candidates, forward_declaration_candidates, mask_cpp


class ReviewHintsTests(unittest.TestCase):
    def test_direct_guard_and_braceless_return(self):
        text = 'void Run() {\nif (!Object) { return; }\nif (!Other) return false;\n}'
        found = guard_log_candidates(text)
        self.assertEqual([f["line"] for f in found], [2, 3])
        self.assertEqual(found[1]["return_value"], "false")

    def test_unconditional_logs_and_registered_host_macro(self):
        text = 'if (!Object) {\nSTUDIO_LOG(LogTest, Warning, TEXT("No object"));\nreturn false;\n}'
        self.assertEqual(len(guard_log_candidates(text)), 1)
        self.assertEqual(guard_log_candidates(text, {"log_functions": ["STUDIO_LOG"]}), [])
        self.assertEqual(guard_log_candidates(text.replace("STUDIO_LOG", "UE_LOG")), [])

    def test_other_branch_and_conditional_logs_do_not_hide_candidate(self):
        for prefix in ('if (bDebug) { UE_LOG(LogTest, Warning, TEXT("No")); }',
                       'if (bDebug) UE_LOG(LogTest, Warning, TEXT("No"));',
                       'UE_CLOG(bDebug, LogTest, Warning, TEXT("No"));'):
            with self.subTest(prefix=prefix):
                found = guard_log_candidates('if (!Object) { ' + prefix + ' return; }')
                self.assertEqual(len(found), 1)
                self.assertTrue(found[0]["nearby_diagnostics"])
        self.assertEqual(len(guard_log_candidates('UE_LOG(LogTest, Log, TEXT("Entry")); if (!Object) return;')), 1)

    def test_nested_branch_then_unconditional_log(self):
        self.assertEqual(guard_log_candidates('if (!Object) { if (bDebug) { Debug(); } UE_LOG(LogTest, Warning, TEXT("No")); return; }'), [])

    def test_comments_literals_raw_strings_and_macro_definitions_are_ignored(self):
        text = '''// if (!Fake) return;
const char* Fake = "if (!Text) return;";
const char* Raw = R"tag(if (!RawText) { return; })tag";
#define GUARD(X) \\
    if (!(X)) return;
/* if (!Block) return; */
if (!Real) { /* UE_LOG(Log, Warning, TEXT("No")); */ return; }
'''
        found = guard_log_candidates(text)
        self.assertEqual([f["condition"] for f in found], ["!Real"])
        self.assertEqual(len(mask_cpp(text)), len(text))

    def test_normal_gates_and_exact_config_exclusions(self):
        text = 'if (!HasAuthority()) return; if (Cached) return Cached; if (!HasAuthority() || !Object) return;'
        self.assertEqual(len(guard_log_candidates(text)), 1)
        self.assertEqual(len(guard_log_candidates(text, all_guards=True)), 3)
        self.assertEqual(guard_log_candidates('if (!Object) return;', {"excluded_conditions": ["! Object"]}), [])

    def test_ensure_is_reported_as_diagnostic_not_unconditional_log(self):
        found = guard_log_candidates('if (!ensure(IsValid(Object))) return;')
        self.assertEqual(len(found), 1)
        self.assertIn("ensure", found[0]["nearby_diagnostics"])

    def test_aggregate_return_and_nested_lambda(self):
        found = guard_log_candidates('if (!Object) { auto Action = [] { return false; }; return FResult{}; }')
        self.assertEqual(len(found), 1)
        self.assertEqual(found[0]["return_value"], "FResult{}")

    def test_pointer_reference_and_object_wrapper_candidates(self):
        for decl in ('UWidget* Widget;', 'const UWidget& Widget;', 'TObjectPtr<UWidget> Widget;'):
            with self.subTest(decl=decl):
                found = forward_declaration_candidates('#include "Widget.h"\n' + decl)
                self.assertEqual([f["type"] for f in found], ["UWidget"])
        found = forward_declaration_candidates('#include "Payload.h"\nFPayload* Payload;')
        self.assertEqual(found[0]["declaration"], "struct FPayload;")

    def test_definition_required_uses_are_not_candidates(self):
        for body in ('UWidget Widget;', 'class UHost : public UWidget {};',
                     'UWidget* Widget; void Reset() { Widget->Reset(); }',
                     'UWidget* Widget; void Reset() { delete Widget; }',
                     'UWidget* Widget; auto Value = UWidget::StaticClass();',
                     'Namespace::UWidget* Widget;'):
            with self.subTest(body=body):
                self.assertEqual(forward_declaration_candidates('#include "Widget.h"\n' + body), [])

    def test_commented_include_and_comment_only_types_ignored(self):
        self.assertEqual(forward_declaration_candidates('// #include "Widget.h"\nUWidget* Widget;'), [])
        self.assertEqual(forward_declaration_candidates('#include "Widget.h"\n// UWidget* Widget;'), [])


class ReviewCliTests(unittest.TestCase):
    setUp = fixtures.PortableToolingTests.setUp
    write = fixtures.PortableToolingTests.write
    run_cli = fixtures.PortableToolingTests.run_cli
    python_tool = fixtures.PortableToolingTests.python_tool

    def test_missing_mode_and_compatibility_command_use_same_rules(self):
        self.header.write_text('// Type documentation\nUCLASS()\nclass UHost {};\nUENUM()\nenum class EMode { One };\n', encoding="utf-8")
        args = ('--root', self.host, '--no-context', '--verbose', 'Source')
        shared = self.python_tool('Tools/check_comments.py', '--missing-only', *args, expected=1)
        old = self.python_tool('Tools/detect_missing_comments.py', *args, expected=1)
        self.assertEqual(shared, old)
        self.assertIn('UENUM', shared)
        self.assertNotIn('UCLASS 위에', shared)
        self.assertNotIn('PRAGMA', shared)
        self.assertNotIn('LICENSE', shared)

    def test_excluded_paths_apply_to_all_python_source_tools_from_subdirectory(self):
        self.write('Config/JWCommonUtilityTools.json', json.dumps({
            'schema_version': 1, 'excluded_paths': ['Source/NewFlight/*']}))
        self.cpp.write_text('void ACustomShip::Run() { if (!Object) return; }', encoding='utf-8')
        for tool in ('Tools/check_comments.py', 'Tools/detect_missing_comments.py',
                     'Tools/scan_structure.py', 'Agent/Skills/unreal-code-refine/scan_structure.py', 'Tools/check_code.py'):
            with self.subTest(tool=tool):
                output = self.python_tool(tool, '--root', self.host / 'Source', '--no-context', 'NewFlight')
                self.assertNotIn('CustomShip', output)
        self.header.write_text('// Copyright (c) 2026 Original.\n', encoding='utf-8')
        before = self.header.read_bytes()
        self.python_tool('Tools/update_copyright.py', '--root', self.host / 'Source', '--old', 'Original', '--new', 'Changed', '--apply')
        self.assertEqual(before, self.header.read_bytes())

    def test_code_report_json_exit_zero_limit_and_read_only(self):
        self.cpp.write_text('void ACustomShip::Run() { if (!One) return; if (!Two) return; }', encoding='utf-8')
        before = self.cpp.read_bytes()
        result = subprocess.run([sys.executable, '-B', str(self.plugin / 'Tools/check_code.py'),
                                 '--root', str(self.host), '--json', '--checks', 'guard-logs', '--limit', '1', 'Source'],
                                cwd=self.outside, env=self.env, capture_output=True, text=True, encoding='utf-8', timeout=30)
        self.assertEqual(result.returncode, 0, result.stderr)
        report = json.loads(result.stdout)
        self.assertTrue(report['candidate_only'])
        self.assertEqual(report['total_candidates'], 2)
        self.assertEqual(len(report['findings']), 1)
        self.assertTrue(report['truncated'])
        self.assertIn('[JWCU context]', result.stderr)
        self.assertEqual(before, self.cpp.read_bytes())

    def test_invalid_guard_config_fails_even_when_context_hidden(self):
        self.write('Config/JWCommonUtilityTools.json', json.dumps({'schema_version': 1, 'guard_logs': {'log_functions': ['bad.*']}}))
        for tool in ('Tools/check_code.py', 'Tools/check_comments.py', 'Tools/scan_structure.py'):
            self.python_tool(tool, '--root', self.host, '--no-context', 'Source', expected=2)

    def test_structure_inventory_compatibility_and_cli(self):
        args = ('--root', self.host, '--no-context', '--inventory', 'Source')
        self.assertEqual(self.python_tool('Tools/scan_structure.py', *args),
                         self.python_tool('Agent/Skills/unreal-code-refine/scan_structure.py', *args))


if __name__ == '__main__':
    unittest.main(verbosity=2)
