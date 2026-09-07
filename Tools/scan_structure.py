# Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.
"""Unreal C++ 파일 구조 진단 도구 — 분할 대상과 축 후보를 뽑는다.

사용법:
    python scan_structure.py <파일 또는 디렉터리>...
    python scan_structure.py --root <프로젝트 루트> --inventory Source

--inventory 는 멤버 함수 정의 목록을 정렬해 찍는다. 분할 전후로 한 번씩 돌려
함수 이름과 줄 수 비교를 돕지만 본문이 같은지는 증명하지 않는다.
"""

from __future__ import annotations

import argparse
import os
import re
import sys
from collections import defaultdict
from jwcu_context import emit_context, load_context, source_files, iter_source_files
from cpp_review import forward_declaration_candidates

# cpp 분할 권고 임계치 — 둘 중 하나만 넘어도 후보.
CPP_LINE_THRESHOLD = 800
CPP_FUNC_THRESHOLD = 60

# 구조체 이사 권고 임계치 — UCLASS 와 동거 중일 때만 적용.
STRUCT_BLOCK_LINE_THRESHOLD = 40
STRUCT_PROPERTY_THRESHOLD = 6

RE_TYPE_OPEN = re.compile(r"^\s*(USTRUCT|UCLASS|UENUM|UINTERFACE)\s*\(")
RE_STRUCT_NAME = re.compile(r"^\s*struct\s+(?:\w+_API\s+)?(\w+)")
RE_CLASS_NAME = re.compile(r"^\s*class\s+(?:\w+_API\s+)?(\w+)")
# 반환 타입 접두는 선택 — 생성자/소멸자는 줄 첫머리가 곧 클래스 이름이라 접두가 없다.
RE_MEMBER_DEF = re.compile(r"^(?:[A-Za-z_][\w:<>,\s\*&]*?\b)?(\w+)::(~?\w+)\s*\(")
RE_ANON_NS = re.compile(r"^namespace\s*$")
RE_CVAR = re.compile(r"\bTAutoConsoleVariable|\bFAutoConsoleVariable|\bIConsoleVariable")
RE_LOG_CATEGORY = re.compile(r"\bDEFINE_LOG_CATEGORY(?:_STATIC)?\s*\(")

# 함수 이름 앞머리에 붙는 동사 — 축 이름 후보를 뽑을 때 걷어낸다.
COMMON_VERBS = {
    "Set", "Get", "Update", "Handle", "Apply", "Compute", "Is", "Has", "On", "Try",
    "Refresh", "Build", "Resolve", "Make", "Ensure", "Notify", "Can", "Should", "Find",
    "Add", "Remove", "Clear", "Reset", "Start", "Stop", "Begin", "End", "Run", "Do",
    "Init", "Create", "Destroy", "Register", "Unregister", "Request", "Process", "Cache",
}
# 축 이름으로는 의미가 없는 꼬리 토큰.
NOISE_TOKENS = {"From", "To", "For", "With", "By", "Of", "And", "The", "Implementation",
                "Internal", "Impl", "Value", "State", "Data", "Info", "Source", "Target"}

# 클래스 이름에서 떼어내고 Types 를 붙일 접미사.
ENTITY_SUFFIXES = ("Component", "Manager", "Subsystem", "Actor", "Processor", "Controller",
                   "Widget", "Spec", "Profile")

# cpp 를 나눌 축 후보. 순서가 곧 우선순위 — 위에서부터 먼저 매칭한다.
ROLE_PATTERNS = [
    ("Debug",     re.compile(r"Debug|Draw|Visuali|DumpStat|OnScreen", re.I)),
    ("Net",       re.compile(r"^(Server_|Client_|Multicast_|OnRep_)|Replicat|GetLifetimeReplicated|Sync")),
    ("Input",     re.compile(r"Input|OnPressed|OnReleased|BindAction|HandleLook|HandleMove")),
    ("Lifecycle", re.compile(r"^(BeginPlay|EndPlay|PostInitial|PostLoad|OnConstruction|Initialize|Deinitialize|Setup|Construct|PostRegister|PostEditChange)")),
    ("Tick",      re.compile(r"^(Tick|Integrate|Advance|Simulate|Step)")),
]


class HeaderReport:
    def __init__(self, path):
        self.path = path
        self.lines = 0
        self.structs = []       # (name, line_count, property_count, has_method)
        self.classes = []       # (name, line_count)
        self.enums = 0
        self.include_candidates = []


class SourceReport:
    def __init__(self, path):
        self.path = path
        self.lines = 0
        self.funcs = []         # (owner, name, line_count)
        self.anon_ns_lines = []
        self.cvar_lines = []
        self.log_category_lines = []
        self.guarded_lines = 0  # #if !UE_BUILD_SHIPPING / WITH_EDITOR 안쪽 줄 수


def block_extent(lines, open_index):
    """open_index 부터 시작하는 중괄호 블록의 끝 줄 인덱스. 못 찾으면 open_index."""
    depth = 0
    started = False
    for i in range(open_index, len(lines)):
        depth += lines[i].count("{") - lines[i].count("}")
        if "{" in lines[i]:
            started = True
        if started and depth <= 0:
            return i
    return open_index


def analyze_header(path, lines):
    rep = HeaderReport(path)
    rep.lines = len(lines)

    for i, line in enumerate(lines):
        m = RE_TYPE_OPEN.match(line)
        if not m:
            continue
        kind = m.group(1)
        if kind == "UENUM":
            rep.enums += 1
            continue

        decl_index = i + 1
        while decl_index < len(lines) and not lines[decl_index].strip():
            decl_index += 1
        if decl_index >= len(lines):
            continue

        end = block_extent(lines, decl_index)
        body = lines[decl_index : end + 1]
        span = end - decl_index + 1

        if kind == "USTRUCT":
            nm = RE_STRUCT_NAME.match(lines[decl_index])
            name = nm.group(1) if nm else "?"
            props = sum(1 for l in body if l.strip().startswith("UPROPERTY"))
            # 멤버 함수가 있으면 "단순 데이터 구조체"가 아니다.
            has_method = any(re.search(r"\b\w+\s*\([^;]*\)\s*(const)?\s*[;{]", l)
                             and not l.strip().startswith(("UPROPERTY", "UFUNCTION", "GENERATED"))
                             for l in body)
            rep.structs.append((name, span, props, has_method))
        else:
            nm = RE_CLASS_NAME.match(lines[decl_index])
            rep.classes.append((nm.group(1) if nm else "?", span))

    rep.include_candidates = forward_declaration_candidates("\n".join(lines))

    return rep


def analyze_source(path, lines):
    rep = SourceReport(path)
    rep.lines = len(lines)

    guard_depth = 0
    for i, line in enumerate(lines):
        s = line.strip()

        if s.startswith("#if") and re.search(r"UE_BUILD_SHIPPING|WITH_EDITOR", s):
            guard_depth += 1
        elif guard_depth and s.startswith("#if"):
            guard_depth += 1
        elif guard_depth and s.startswith("#endif"):
            guard_depth -= 1
        elif guard_depth:
            rep.guarded_lines += 1

        if RE_ANON_NS.match(line):
            rep.anon_ns_lines.append(i + 1)
        if RE_CVAR.search(line):
            rep.cvar_lines.append(i + 1)
        if RE_LOG_CATEGORY.search(line):
            rep.log_category_lines.append(i + 1)

        m = RE_MEMBER_DEF.match(line)
        if m:
            end = block_extent(lines, i)
            rep.funcs.append((m.group(1), m.group(2), max(1, end - i + 1)))

    return rep


def role_of(func_name):
    for role, pattern in ROLE_PATTERNS:
        if pattern.search(func_name):
            return role
    return "Core"


def camel_tokens(name):
    """SetWingtipVortexColorAlpha → [Wingtip, Vortex, Color, Alpha] (앞머리 동사 제거)."""
    parts = re.findall(r"[A-Z][a-z0-9]*|[A-Z]+(?![a-z])", name)
    while parts and parts[0] in COMMON_VERBS:
        parts.pop(0)
    return [p for p in parts if p not in NOISE_TOKENS and len(p) > 2]


def suggest_domain_axes(funcs, top=6):
    """이름 토큰 빈도로 도메인 축 후보를 뽑는다. Core 버킷이 거대할 때 쓴다.

    역할(Debug/Net/…)로 안 갈리는 액터형 파일은 도메인(Vortex, Loadout, Camera…)이
    실제 분할 축이다. 정답은 아니고 사람이 고를 후보를 좁혀 주는 용도.
    """
    stats = defaultdict(lambda: [0, 0])  # token -> [함수 수, 줄 수]
    for name, span in funcs:
        seen = set()
        for token in camel_tokens(name):
            if token in seen:
                continue
            seen.add(token)
            stats[token][0] += 1
            stats[token][1] += span
    ranked = sorted(stats.items(), key=lambda kv: (-kv[1][1], -kv[1][0]))
    return [(t, c, l) for t, (c, l) in ranked if c >= 3][:top]


def types_header_name(header_path):
    """VehicleStatComponent.h → VehicleStatTypes.h"""
    stem = os.path.basename(header_path)[:-2]
    for suffix in ENTITY_SUFFIXES:
        if stem.endswith(suffix):
            stem = stem[: -len(suffix)]
            break
    return f"{stem}Types.h"


def print_header_report(rep):
    has_class = bool(rep.classes)
    heavy = [s for s in rep.structs
             if s[1] >= STRUCT_BLOCK_LINE_THRESHOLD or s[2] >= STRUCT_PROPERTY_THRESHOLD or s[3]]
    exempt = os.path.basename(rep.path).endswith("Types.h")

    print(f"\n[H] {rep.path}  ({rep.lines}줄, USTRUCT {len(rep.structs)} / UCLASS {len(rep.classes)} / UENUM {rep.enums})")

    if rep.structs:
        for name, span, props, has_method in rep.structs:
            mark = "무거움" if (span >= STRUCT_BLOCK_LINE_THRESHOLD or props >= STRUCT_PROPERTY_THRESHOLD or has_method) else "단순"
            method_note = ", 메서드 보유" if has_method else ""
            print(f"      struct {name}: {span}줄, UPROPERTY {props}{method_note}  → {mark}")

    if exempt:
        print("      판정: 타입 전용 헤더(*Types.h). 구조체가 많아도 정상 — 이사 대상 아님.")
    elif has_class and heavy:
        moved = sum(s[1] for s in heavy)
        print(f"      판정: UCLASS 와 무거운 구조체 {len(heavy)}개가 동거 중({moved}줄). "
              f"→ {types_header_name(rep.path)} 로 이사 권장")
    elif has_class and rep.structs:
        print("      판정: 동거 구조체가 전부 단순 데이터. 그대로 둔다.")
    else:
        print("      판정: 구조체 이사 대상 아님.")

    if rep.include_candidates:
        print("      include 축소 검토 후보(포인터/참조·객체 래퍼 사용 추정):")
        for candidate in rep.include_candidates:
            print(f"        - L{candidate['line']} {candidate['include']} → {candidate['declaration']} ({candidate['reason']})")


def print_source_report(rep):
    print(f"\n[C] {rep.path}  ({rep.lines}줄, 멤버 함수 {len(rep.funcs)}개)")

    buckets = defaultdict(list)
    for owner, name, span in rep.funcs:
        buckets[role_of(name)].append((name, span))

    for role in ["Debug", "Net", "Input", "Tick", "Lifecycle", "Core"]:
        if role not in buckets:
            continue
        items = buckets[role]
        total = sum(s for _, s in items)
        sample = ", ".join(n for n, _ in items[:4])
        more = f" 외 {len(items) - 4}개" if len(items) > 4 else ""
        print(f"      {role:<9} 함수 {len(items):3d}개 / {total:5d}줄  ({sample}{more})")

    # 역할 축으로 안 갈리고 Core 에 몰린 액터형 파일은 도메인 축을 봐야 한다.
    core = buckets.get("Core", [])
    if len(core) >= 20:
        axes = suggest_domain_axes(core)
        if axes:
            print("      Core 도메인 축 후보(이름 토큰 빈도):")
            for token, cnt, ln in axes:
                print(f"        - {token:<16} 함수 {cnt:3d}개 / {ln:4d}줄")

    if rep.guarded_lines:
        print(f"      #if !UE_BUILD_SHIPPING·WITH_EDITOR 안쪽 {rep.guarded_lines}줄 → _Debug.cpp 로 떼기 쉬움")

    if rep.anon_ns_lines:
        print(f"      ⚠ namespace 선언 {rep.anon_ns_lines} 행 — 익명 여부와 Unity 빌드의 이름 충돌 가능성을 확인한다")

    if rep.cvar_lines or rep.log_category_lines:
        stem = os.path.basename(rep.path)[:-4]
        bits = []
        if rep.cvar_lines:
            bits.append(f"CVar {len(rep.cvar_lines)}개")
        if rep.log_category_lines:
            bits.append(f"로그 카테고리 {len(rep.log_category_lines)}개")
        print(f"      {' / '.join(bits)} → {stem}_CVars.h 로 분리 검토")

    over_lines = rep.lines >= CPP_LINE_THRESHOLD
    over_funcs = len(rep.funcs) >= CPP_FUNC_THRESHOLD
    if over_lines or over_funcs:
        why = []
        if over_lines:
            why.append(f"{rep.lines}줄 ≥ {CPP_LINE_THRESHOLD}")
        if over_funcs:
            why.append(f"함수 {len(rep.funcs)}개 ≥ {CPP_FUNC_THRESHOLD}")
        print(f"      판정: 분할 후보 ({', '.join(why)})")
    else:
        print("      판정: 분할 임계치 미만. 그대로 둔다.")


iter_files = iter_source_files


def read_lines(path):
    with open(path, "r", encoding="utf-8-sig", errors="replace") as fp:
        return fp.read().splitlines()


def run_inventory(targets):
    """멤버 함수 이름·줄 수를 비교한다. 본문 동등성을 보장하지 않는다."""
    entries = []
    for path in iter_files(targets):
        if not path.endswith(".cpp"):
            continue
        for owner, name, span in analyze_source(path, read_lines(path)).funcs:
            entries.append(f"{owner}::{name}\t{span}")
    for line in sorted(entries):
        print(line)
    print(f"\n총 {len(entries)}개 함수", file=sys.stderr)


def main():
    parser = argparse.ArgumentParser(description="Unreal C++ 구조 진단")
    parser.add_argument("paths", nargs="+", help="파일 또는 디렉터리")
    parser.add_argument("--root", default=".", help="상대 입력 경로와 출력 경로의 기준 (기본: 현재 디렉터리)")
    parser.add_argument("--config", help="프로젝트 설정 경로")
    parser.add_argument("--no-context", action="store_true", help="지침 안내 출력만 생략")
    parser.add_argument("--inventory", action="store_true",
                        help="멤버 함수 정의 목록만 정렬 출력 (분할 전후 대조용)")
    args = parser.parse_args()
    root = os.path.abspath(args.root)
    if not os.path.isdir(root):
        parser.error(f"루트 디렉터리가 없습니다: {root}")
    try:
        context = load_context(root, args.config)
    except (OSError, ValueError) as exc:
        parser.error(str(exc))
    emit_context(context, not args.no_context)
    try:
        targets = source_files(root, args.paths, context)
    except (OSError, ValueError) as exc:
        parser.error(str(exc))

    if args.inventory:
        if targets and not any(p.endswith(".cpp") for p in targets):
            parser.error("인벤토리에 필요한 .cpp 파일이 없습니다.")
        run_inventory(targets)
        return 0

    count = 0
    for path in targets:
        lines = read_lines(path)
        rel = os.path.relpath(path, root).replace(os.sep, "/")
        count += 1
        if path.endswith(".h"):
            print_header_report(analyze_header(rel, lines))
        else:
            print_source_report(analyze_source(rel, lines))

    print(f"\n검사 파일 {count}개", file=sys.stderr)
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except OSError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        sys.exit(2)
