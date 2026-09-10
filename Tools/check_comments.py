# Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.
"""Unreal C++ 주석 스타일 감사 도구.

사용법:
    python check_comments.py [경로...]
    python check_comments.py --root <프로젝트 루트> --summary Source

경로를 주지 않으면 Source/ 와 Plugins/ 전체를 검사한다.
ERROR 가 하나라도 있으면 exit code 1, 입력/설정/읽기 실패는 2.
Config/JWCommonUtilityTools.json이 있으면 저작권 문구와 제외 경로를 읽는다.
"""

from __future__ import annotations

import argparse
import os
import re
import sys
from collections import Counter
from jwcu_context import emit_context, load_context, is_link, source_files, SKIP_DIRS

RE_TYPE_MACRO = re.compile(r"^\s*(UCLASS|USTRUCT|UENUM|UINTERFACE)\s*\(")
RE_MEMBER_MACRO = re.compile(r"^\s*(UPROPERTY|UFUNCTION)\s*\(")
RE_INCLUDE = re.compile(r'^\s*#include\s+["<]([^">]+)[">]')
RE_FWD_DECL = re.compile(r"^\s*(class|struct|enum\s+class)\s+\w+\s*;")
# UHT 는 *.generated.h 를 마지막 include 로 요구한다. 전방 선언보다 뒤에 오는 것이 정상이므로 순서 판정에서 뺀다.
RE_GENERATED_INCLUDE = re.compile(r'^\s*#include\s+"[^"]*\.generated\.h"')
RE_DOXYGEN = re.compile(r"@(param|return|returns|brief|note|see)\b")
RE_CPP_FUNC_DEF = re.compile(r"^[A-Za-z_][\w:<>,\s\*&]*\b\w+::\w+\s*\(")

SEVERITY_ERROR = "ERROR"
SEVERITY_WARN = "WARN"


class Finding:
    __slots__ = ("path", "line", "severity", "code", "message")

    def __init__(self, path: str, line: int, severity: str, code: str, message: str) -> None:
        self.path = path
        self.line = line
        self.severity = severity
        self.code = code
        self.message = message

    def __str__(self) -> str:
        return f"{self.path}:{self.line}: [{self.severity}/{self.code}] {self.message}"


def is_comment_line(line: str) -> bool:
    s = line.strip()
    return s.startswith("//") or s.startswith("/*") or s.startswith("*") or s.endswith("*/")


def find_doc_above(lines: list[str], index: int) -> tuple[int, int] | None:
    """index 바로 위에 붙은 주석 블록의 (시작, 끝) 0-based 라인 범위. 없으면 None."""
    i = index - 1
    # 매크로/전처리 라인과 빈 줄은 건너뛴다.
    while i >= 0:
        s = lines[i].strip()
        if s == "" or s.startswith("#if") or s.startswith("#endif") or s.startswith("#pragma region"):
            i -= 1
            continue
        break
    if i < 0 or not is_comment_line(lines[i]):
        return None
    end = i
    while i >= 0 and is_comment_line(lines[i]):
        i -= 1
    return i + 1, end


def check_license(path: str, lines: list[str], out: list[Finding], license_header=None) -> None:
    if license_header is not None and (not lines or lines[0].strip() != license_header):
        out.append(Finding(path, 1, SEVERITY_ERROR, "LICENSE", "첫 줄 저작권 헤더가 없거나 문구가 다르다."))


def check_doc_quality(path: str, lines: list[str], start: int, end: int, out: list[Finding]) -> None:
    block = lines[start : end + 1]
    text = " ".join(l.strip() for l in block)
    span = end - start + 1
    if span > 4:
        out.append(
            Finding(path, start + 1, SEVERITY_WARN, "VERBOSE", f"주석 블록이 {span}줄이다. 1~2줄 요약으로 줄인다.")
        )
    if RE_DOXYGEN.search(text):
        out.append(
            Finding(path, start + 1, SEVERITY_WARN, "DOXYGEN", "@param/@return 나열은 기준 스타일이 아니다. 한 줄 요약으로 대체한다.")
        )


def check_header(path: str, lines: list[str], out: list[Finding], license_header=None) -> None:
    check_license(path, lines, out, license_header)

    pragma_idx = next((i for i, l in enumerate(lines) if l.strip() == "#pragma once"), None)
    if pragma_idx is None:
        out.append(Finding(path, 1, SEVERITY_ERROR, "PRAGMA", "#pragma once 가 없다."))

    first_include = None
    last_include = None
    first_fwd = None
    for i, line in enumerate(lines):
        if RE_INCLUDE.match(line):
            if first_include is None:
                first_include = i
            if not RE_GENERATED_INCLUDE.match(line):
                last_include = i
        elif RE_FWD_DECL.match(line) and first_fwd is None:
            first_fwd = i

    if pragma_idx is not None and first_include is not None and first_include < pragma_idx:
        out.append(Finding(path, first_include + 1, SEVERITY_ERROR, "PRAGMA", "#include 가 #pragma once 보다 앞에 있다."))
    if first_fwd is not None and last_include is not None and last_include > first_fwd:
        out.append(
            Finding(path, last_include + 1, SEVERITY_WARN, "ORDER", "전방 선언 뒤에 #include 가 있다. 순서는 include → 전방 선언.")
        )

    check_reflected_comments(path, lines, out)


def check_reflected_comments(path, lines, out, quality=True):
    for i, line in enumerate(lines):
        if RE_TYPE_MACRO.match(line):
            doc = find_doc_above(lines, i)
            if doc is None:
                out.append(
                    Finding(path, i + 1, SEVERITY_ERROR, "TYPE_DOC", f"{RE_TYPE_MACRO.match(line).group(1)} 위에 타입 설명 주석이 없다.")
                )
            elif quality:
                check_doc_quality(path, lines, doc[0], doc[1], out)
        elif RE_MEMBER_MACRO.match(line):
            doc = find_doc_above(lines, i)
            if doc is None:
                kind = RE_MEMBER_MACRO.match(line).group(1)
                out.append(Finding(path, i + 1, SEVERITY_ERROR, "MEMBER_DOC", f"{kind} 위에 한 줄 설명 주석이 없다."))
            elif quality:
                check_doc_quality(path, lines, doc[0], doc[1], out)


def check_source(path: str, lines: list[str], out: list[Finding], license_header=None) -> None:
    check_license(path, lines, out, license_header)

    # 대응 헤더가 있으면 첫 include 여야 한다 (언리얼 관례).
    own_header = os.path.splitext(os.path.basename(path))[0] + ".h"
    first_include = next((RE_INCLUDE.match(l) for l in lines if RE_INCLUDE.match(l)), None)
    if first_include is not None:
        included = os.path.basename(first_include.group(1))
        if included != own_header and _header_exists(path, own_header):
            idx = next(i for i, l in enumerate(lines) if RE_INCLUDE.match(l))
            out.append(
                Finding(path, idx + 1, SEVERITY_WARN, "SELF_INCLUDE", f'첫 #include 는 "{own_header}" 여야 한다.')
            )

    # 함수 정의 위 doc 주석 금지 — cpp 주석은 본문 내부 로직에만.
    for i, line in enumerate(lines):
        if RE_CPP_FUNC_DEF.match(line):
            doc = find_doc_above(lines, i)
            if doc and lines[doc[0]].strip().startswith("/**"):
                out.append(
                    Finding(path, doc[0] + 1, SEVERITY_WARN, "CPP_DOC", "cpp 함수 정의 위 /** */ 주석. 설명은 헤더에 두고 여기선 지운다.")
                )


_HEADER_CACHE: dict[str, set[str]] = {}


def _header_exists(cpp_path: str, header_name: str) -> bool:
    root = _module_root(cpp_path)
    if root not in _HEADER_CACHE:
        found = set()
        for dirpath, dirnames, filenames in os.walk(root):
            dirnames[:] = [d for d in dirnames if d not in SKIP_DIRS and not is_link(os.path.join(dirpath, d))]
            for fn in filenames:
                if fn.endswith(".h"):
                    found.add(fn)
        _HEADER_CACHE[root] = found
    return header_name in _HEADER_CACHE[root]


def _module_root(path: str) -> str:
    parts = os.path.abspath(path).split(os.sep)
    for marker in ("Private", "Public", "Classes"):
        if marker in parts:
            return os.sep.join(parts[: parts.index(marker)])
    return os.path.dirname(os.path.abspath(path))


def main(argv=None, *, missing_only=False) -> int:
    parser = argparse.ArgumentParser(description="Unreal C++ 주석 스타일 감사")
    parser.add_argument("paths", nargs="*", help="검사할 파일/디렉터리 (기본: Source, Plugins)")
    parser.add_argument("--root", default=".", help="입력·설정·출력 경로 기준 (기본: 현재 디렉터리)")
    parser.add_argument("--config", help="루트 기준 설정 경로 (기본: Config/JWCommonUtilityTools.json, 없어도 실행 가능)")
    parser.add_argument("--no-context", action="store_true", help="지침 안내 출력만 생략 (설정은 계속 적용)")
    parser.add_argument("--include-external", action="store_true",
                        help="owned_paths 밖의 외부 코드도 검사한다 (기본: 제외)")
    parser.add_argument("--summary", action="store_true", help="파일별 위반 개수만 많은 순으로 출력")
    parser.add_argument("--errors-only", action="store_true", help="ERROR 만 출력")
    parser.add_argument("--limit", type=int, default=0, help="출력할 findings 최대 개수 (0=제한 없음)")
    parser.add_argument("--missing-only", action="store_true", default=missing_only, help="헤더의 reflected 선언 주석 누락만 검사")
    parser.add_argument("--verbose", "-v", action="store_true", help="누락 선언의 코드 줄도 출력")
    args = parser.parse_args(argv)

    root = os.path.abspath(args.root)
    if not os.path.isdir(root):
        parser.error(f"루트 디렉터리가 없습니다: {root}")
    if args.limit < 0:
        parser.error("--limit은 0 이상이어야 합니다.")
    try:
        context = load_context(root, args.config)
        license_header = context["policy"].get("license_header")
    except (OSError, ValueError) as exc:
        parser.error(str(exc))
    emit_context(context, not args.no_context)
    if license_header is None and not args.missing_only:
        print("LICENSE 검사 생략: 프로젝트 license_header가 설정되지 않았습니다.", file=sys.stderr)
    try:
        targets = source_files(root, args.paths, context, headers_only=args.missing_only,
                               defaults=("Source",) if missing_only else ("Source", "Plugins"),
                               include_external=args.include_external)
    except (OSError, ValueError) as exc:
        parser.error(str(exc))

    findings: list[Finding] = []
    declarations = {}
    file_count = 0
    failed = False
    for path in targets:
        file_count += 1
        try:
            with open(path, "r", encoding="utf-8-sig", errors="replace") as fp:
                lines = fp.read().splitlines()
        except OSError as exc:
            print(f"{path}: 읽기 실패 {exc}", file=sys.stderr)
            failed = True
            continue
        if args.verbose:
            declarations.update(((os.path.relpath(path, root).replace(os.sep, "/"), i + 1), line.strip())
                                for i, line in enumerate(lines))
        if args.missing_only:
            check_reflected_comments(path, lines, findings, quality=False)
        elif path.endswith(".h"):
            check_header(path, lines, findings, license_header)
        else:
            check_source(path, lines, findings, license_header)

    for finding in findings:
        finding.path = os.path.relpath(finding.path, root).replace(os.sep, "/")

    if args.errors_only:
        findings = [f for f in findings if f.severity == SEVERITY_ERROR]

    if args.summary:
        per_file = Counter(f.path for f in findings)
        for path, count in per_file.most_common(args.limit or 40):
            codes = Counter(f.code for f in findings if f.path == path)
            detail = " ".join(f"{c}x{n}" for c, n in codes.most_common())
            print(f"{count:4d}  {path}  ({detail})")
    else:
        shown = findings[: args.limit] if args.limit else findings
        for f in shown:
            print(f)
            if args.verbose and f.code in ("TYPE_DOC", "MEMBER_DOC"):
                print("    " + declarations[(f.path, f.line)])
        if args.limit and len(findings) > args.limit:
            print(f"... {len(findings) - args.limit} more")

    errors = sum(1 for f in findings if f.severity == SEVERITY_ERROR)
    warns = len(findings) - errors
    print(f"\n검사 파일 {file_count}개 / ERROR {errors} / WARN {warns}", file=sys.stderr)
    return 2 if failed else (1 if errors else 0)


if __name__ == "__main__":
    sys.exit(main())
