# Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT
"""Read-only starting points for agent review of guard logs and forward declarations."""

import argparse
import json
import os
from pathlib import Path
import sys

from cpp_review import guard_log_candidates, forward_declaration_candidates
from jwcu_context import load_context, emit_context, source_files


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("paths", nargs="*", help="대상 .h/.cpp 또는 디렉터리 (기본 Source, Plugins)")
    parser.add_argument("--root", default=".")
    parser.add_argument("--config")
    parser.add_argument("--no-context", action="store_true")
    parser.add_argument("--include-external", action="store_true",
                        help="owned_paths 밖의 외부 코드도 검사한다 (기본: 제외)")
    parser.add_argument("--checks", choices=("all", "guard-logs", "forward-declarations"), default="all")
    parser.add_argument("--all-guards", action="store_true", help="정상 권한 분기·긍정 조건의 반환도 검토 후보에 포함")
    parser.add_argument("--json", action="store_true", help="stdout에 구조화된 보고서 출력")
    parser.add_argument("--limit", type=int, default=100, help="후보 출력 개수 (0=전체); 전체 개수는 별도 출력")
    args = parser.parse_args(argv)
    if args.limit < 0: parser.error("--limit은 0 이상이어야 합니다.")
    try:
        root = os.path.abspath(args.root)
        context = load_context(root, args.config)
        files = source_files(root, args.paths, context, include_external=args.include_external)
    except (OSError, ValueError) as exc:
        parser.error(str(exc))
    emit_context(context, not args.no_context)
    findings = []
    failures = []
    for path in files:
        relative = os.path.relpath(path, root).replace(os.sep, "/")
        try:
            text = Path(path).read_text(encoding="utf-8-sig")
        except (OSError, UnicodeError) as exc:
            failures.append({"path": relative, "error": str(exc)})
            continue
        local = []
        if args.checks in ("all", "guard-logs"):
            local.extend(guard_log_candidates(text, context["policy"].get("guard_logs"), all_guards=args.all_guards))
        if path.endswith(".h") and args.checks in ("all", "forward-declarations"):
            local.extend(forward_declaration_candidates(text))
        findings.extend({"path": relative, **f} for f in local)
    findings.sort(key=lambda f: (f["path"], f["line"], f["rule"]))
    shown = findings[:args.limit] if args.limit else findings
    if args.json:
        print(json.dumps({"schema_version": 1, "candidate_only": True, "files": len(files),
                          "total_candidates": len(findings), "truncated": len(shown) < len(findings),
                          "findings": shown, "read_errors": failures}, ensure_ascii=True, indent=2))
    else:
        for finding in shown:
            evidence = (f"if ({finding['condition']}) -> return L{finding['return_line']}"
                        if "condition" in finding else f"{finding['include']} -> {finding['declaration']}")
            print(f"{finding['path']}:{finding['line']}: [{finding['rule']}] {evidence}")
            print("    " + finding["reason"])
            if finding.get("nearby_diagnostics"):
                print("    조건부 로그/진단 확인: " + ", ".join(finding["nearby_diagnostics"]))
    print(f"검토 후보 {len(findings)}개 / 출력 {len(shown)}개 / 검사 파일 {len(files)}개. 후보는 오류 확정이나 자동 수정 지시가 아닙니다.", file=sys.stderr)
    for failure in failures:
        print(f"{failure['path']}: 읽기 실패 {failure['error']}", file=sys.stderr)
    return 2 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
