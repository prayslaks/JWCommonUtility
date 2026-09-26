# Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT
"""저작권 표기 일괄 교체 도구.

개인 명의로 박혀 있는 저작권 문구를 팀/회사 명의로 바꾼다.
BOM, 줄바꿈(CRLF/LF), 원본 인코딩을 그대로 보존한다.

동작은 두 단계다.

1. rename    : 파일 전체에서 `Copyright (c) <연도> <옛 이름>` 을 새 이름으로 치환한다.
               .h/.cpp 뿐 아니라 .ini/.md/.py/.ps1/.cs 등 텍스트 파일 전부가 대상이다.
2. normalize : (--normalize) 코드 파일 1번 줄을 표준 헤더로 정규화한다.
               앞 공백, `// //` 중복 같은 변형을 한 줄로 통일한다.
               --add-missing 을 주면 헤더가 없는 파일에 새로 삽입한다.

기본은 dry-run 이다. 실제로 쓰려면 --apply 를 준다.

사용 예:
    python update_copyright.py --root <프로젝트 루트> --old "OldOwner" --new "TeamName"
    python update_copyright.py --root <프로젝트 루트> --old "OldOwner" --new "TeamName" --normalize --add-missing
    python update_copyright.py --root <프로젝트 루트> --old "OldOwner" --new "TeamName" --normalize --add-missing --apply
"""

from __future__ import annotations

import argparse
import os
import re
import sys
from datetime import date
from jwcu_context import emit_context, load_context, is_link, is_excluded, is_owned

# --- 설정 -------------------------------------------------------------------

# 순회에서 통째로 제외할 디렉터리 이름.
SKIP_DIRS = {
    ".git", ".vs", ".idea",
    "Intermediate", "Binaries", "Saved", "DerivedDataCache", "Build",
    "node_modules", "__pycache__",
}

# 처리 대상 텍스트 확장자. 화이트리스트라 바이너리는 애초에 열지 않는다.
TEXT_EXTS = {
    ".h", ".hpp", ".inl", ".c", ".cc", ".cpp", ".cs",
    ".py", ".ps1", ".bat", ".sh",
    ".ini", ".md", ".txt", ".json",
    ".uproject", ".uplugin",
    ".usf", ".ush",
}

# 1번 줄 헤더 정규화 대상과 그 주석 기호.
COMMENT_PREFIX = {
    ".h": "//", ".hpp": "//", ".inl": "//", ".c": "//", ".cc": "//",
    ".cpp": "//", ".cs": "//", ".usf": "//", ".ush": "//",
    ".py": "#", ".ps1": "#", ".sh": "#",
}

# 표준 헤더 본문. {year} 와 {holder} 가 치환된다.
CANONICAL_BODY = (
    "Copyright (c) {year} {holder}. All rights reserved. "
    "Unauthorized copying, modification, or distribution of this file, "
    "via any medium is strictly prohibited. Proprietary and confidential."
)

# 1번 줄이 저작권 줄인지 판정. `// //` 중복과 앞 공백을 함께 흡수한다.
COPYRIGHT_LINE_RE = re.compile(r"^\s*(?:(?://+|#+)\s*)+Copyright\b.*$")

EPIC_MARK = "Copyright Epic Games"

# 헤더가 1번 줄이 아니라 쉐뱅/param 아래에 있는 경우를 잡기 위해 훑을 줄 수.
HEADER_SCAN_LINES = 5


# --- 유틸 -------------------------------------------------------------------

def decode(raw):
    """바이트를 (텍스트, BOM 마커, 인코딩)으로 푼다."""
    if raw.startswith(b"\xef\xbb\xbf"):
        return raw[3:].decode("utf-8"), "﻿", "utf-8"
    try:
        return raw.decode("utf-8"), "", "utf-8"
    except UnicodeDecodeError:
        # 옛 파일이 cp949 로 저장돼 있을 수 있다.
        return raw.decode("cp949"), "", "cp949"


def split_first_line(body):
    """본문을 (1번 줄, 그 줄의 줄바꿈, 나머지)로 쪼갠다."""
    idx = body.find("\n")
    if idx == -1:
        return body, "", ""
    raw = body[:idx]
    rest = body[idx + 1:]
    if raw.endswith("\r"):
        return raw[:-1], "\r\n", rest
    return raw, "\n", rest


def starts_blank(text):
    """본문 첫 줄이 이미 빈 줄인지 본다. 헤더 뒤 빈 줄을 중복으로 넣지 않기 위해 쓴다."""
    return text == "" or text.startswith("\n") or text.startswith("\r\n")


def dominant_newline(body):
    crlf = body.count("\r\n")
    lf = body.count("\n") - crlf
    return "\r\n" if crlf >= lf else "\n"


# --- 본 처리 ----------------------------------------------------------------

class Rewriter:
    def __init__(self, args):
        self.args = args
        self.rename_re = re.compile(
            r"(Copyright\s*\(c\)\s*)(\d{4})(\s+)" + re.escape(args.old) + r"\b"
        )
        self.stats = {"renamed": 0, "normalized": 0, "inserted": 0}
        self.changed = []
        self.failed = []
        self.misplaced = []
        self._insert_roots = [
            os.path.normpath(os.path.abspath(os.path.join(args.root, r.strip())))
            for r in (args.add_missing_under or "").split(",") if r.strip()
        ]

    def canonical(self, year):
        return CANONICAL_BODY.format(year=self.args.year or year, holder=self.args.new)

    def rename_pass(self, text):
        def sub(m):
            year = self.args.year or m.group(2)
            return m.group(1) + year + m.group(3) + self.args.new
        return self.rename_re.sub(sub, text)

    def _insert_allowed(self, path):
        """--add-missing 삽입을 허용할 경로인지 본다. 임시/산출물 폴더를 걸러낸다."""
        if not self.args.add_missing_under:
            return True
        norm = os.path.normpath(os.path.abspath(path))
        return any(os.path.commonpath((norm, root)) == root for root in self._insert_roots)

    def normalize_pass(self, text, ext, path):
        """1번 줄 헤더를 표준형으로 맞춘다. (새 텍스트, 동작이름)을 돌려준다."""
        prefix = COMMENT_PREFIX.get(ext)
        if prefix is None:
            return text, None

        bom = "﻿" if text.startswith("﻿") else ""
        body = text[len(bom):]
        first, nl, rest = split_first_line(body)

        if COPYRIGHT_LINE_RE.match(first):
            if EPIC_MARK in first and not self.args.replace_epic:
                return text, None
            year_m = re.search(r"\(c\)\s*(\d{4})", first)
            year = year_m.group(1) if year_m else (self.args.year or str(date.today().year))
            header = prefix + " " + self.canonical(year)
            if first == header:
                return text, None
            nl = nl or dominant_newline(body) or "\n"
            return bom + header + nl + rest, "normalized"

        if not self.args.add_missing:
            return text, None
        if not self._insert_allowed(path):
            return text, None
        # 쉐뱅이나 param 블록 아래에 헤더가 이미 있으면 중복 삽입하지 않는다.
        for line in body.split("\n", HEADER_SCAN_LINES)[:HEADER_SCAN_LINES]:
            if COPYRIGHT_LINE_RE.match(line.rstrip("\r")):
                self.misplaced.append(path)
                return text, None

        header = prefix + " " + self.canonical(self.args.year or str(date.today().year))
        nl = dominant_newline(body) or "\n"
        if first.startswith("#!"):
            # 쉐뱅은 반드시 1번 줄이어야 하므로 그 아래에 넣는다.
            gap = "" if starts_blank(rest) else nl
            return bom + first + nl + header + nl + gap + rest, "inserted"
        # 헤더와 본문 사이에 빈 줄 한 줄을 둔다. 기존 파일들의 배치와 맞춘다.
        gap = "" if starts_blank(body) else nl
        return bom + header + nl + gap + body, "inserted"

    def handle(self, path):
        ext = os.path.splitext(path)[1].lower()
        try:
            with open(path, "rb") as fh:
                raw = fh.read()
            text, bom_marker, enc = decode(raw)
        except (OSError, UnicodeDecodeError) as exc:
            self.failed.append((path, str(exc)))
            return

        original = bom_marker + text
        current = original
        actions = []

        if self.args.normalize:
            current, action = self.normalize_pass(current, ext, path)
            if action:
                actions.append(action)
                self.stats[action] += 1

        after = self.rename_pass(current)
        if after != current:
            actions.append("renamed")
            self.stats["renamed"] += 1
            current = after

        if not actions or current == original:
            return

        self.changed.append((path, actions))
        if self.args.apply:
            has_bom = current.startswith("﻿")
            body = current[1:] if has_bom else current
            data = (b"\xef\xbb\xbf" if has_bom else b"") + body.encode(enc)
            with open(path, "wb") as fh:
                fh.write(data)

    def walk(self, root):
        self_path = os.path.abspath(__file__)
        for dirpath, dirnames, filenames in os.walk(root):
            dirnames[:] = sorted(d for d in dirnames if d not in SKIP_DIRS and not is_link(os.path.join(dirpath, d)))
            for name in filenames:
                if os.path.splitext(name)[1].lower() not in TEXT_EXTS:
                    continue
                path = os.path.join(dirpath, name)
                if is_link(path) or is_excluded(self.context, path):
                    continue
                # 남의 저작권을 우리 표기로 덮어쓰지 않는다.
                if not (self.args.include_external or is_owned(self.context, path)):
                    continue
                if os.path.abspath(path) == self_path and not self.args.include_self:
                    continue
                self.handle(path)


def main(argv):
    ap = argparse.ArgumentParser(description="저작권 표기를 일괄 교체한다.")
    ap.add_argument("--root", required=True, help="순회 시작 디렉터리 (명시 필수)")
    ap.add_argument("--config", help="프로젝트 설정 경로")
    ap.add_argument("--no-context", action="store_true", help="지침 안내 출력만 생략")
    ap.add_argument("--include-external", action="store_true",
                    help="owned_paths 밖의 외부 코드도 대상에 넣는다 (기본: 제외)")
    ap.add_argument("--old", required=True, help="바꿀 옛 저작권자 이름 (명시 필수)")
    ap.add_argument("--new", required=True, help="새 저작권자 이름")
    ap.add_argument("--year", default=None, help="연도도 함께 바꾼다 (미지정 시 원본 유지)")
    ap.add_argument("--normalize", action="store_true",
                    help="코드 파일 1번 줄을 표준 헤더로 정규화한다")
    ap.add_argument("--add-missing", action="store_true",
                    help="헤더가 없는 코드 파일에 헤더를 삽입한다 (--normalize 필요)")
    ap.add_argument("--add-missing-under", default="Source,Plugins",
                    help="헤더 삽입을 허용할 경로 (쉼표 구분, 빈 문자열이면 전체)")
    ap.add_argument("--replace-epic", action="store_true",
                    help="Epic Games 헤더도 교체한다 (기본: 보존)")
    ap.add_argument("--include-self", action="store_true",
                    help="이 스크립트 자신도 대상에 포함한다")
    ap.add_argument("--apply", action="store_true",
                    help="실제로 파일에 쓴다 (기본: dry-run)")
    ap.add_argument("--list", type=int, default=20,
                    help="출력할 변경 파일 수 (0 이면 전부)")
    args = ap.parse_args(argv)
    args.root = os.path.abspath(args.root)
    if not os.path.isdir(args.root):
        ap.error(f"루트 디렉터리가 없습니다: {args.root}")
    try:
        context = load_context(args.root, args.config)
    except (OSError, ValueError) as exc:
        ap.error(str(exc))
    emit_context(context, not args.no_context)
    if not args.old.strip() or not args.new.strip():
        ap.error("--old와 --new는 비어 있을 수 없습니다.")
    if args.list < 0:
        ap.error("--list는 0 이상이어야 합니다.")
    for part in args.add_missing_under.split(","):
        if part.strip():
            candidate = os.path.abspath(os.path.join(args.root, part.strip()))
            try:
                inside = os.path.commonpath((args.root, candidate)) == args.root
            except ValueError:
                inside = False
            if not inside:
                ap.error("--add-missing-under는 --root 내부 경로여야 합니다.")

    if args.add_missing and not args.normalize:
        ap.error("--add-missing 은 --normalize 와 함께 써야 한다.")

    rw = Rewriter(args)
    rw.context = context
    rw.walk(args.root)

    mode = "APPLY" if args.apply else "DRY-RUN"
    tail = (", 연도 -> " + args.year) if args.year else ""
    print("[" + mode + "] '" + args.old + "' -> '" + args.new + "'" + tail)
    print("  변경 파일: " + str(len(rw.changed)))
    print("    rename    : " + str(rw.stats["renamed"]))
    print("    normalize : " + str(rw.stats["normalized"]))
    print("    insert    : " + str(rw.stats["inserted"]))

    shown = rw.changed if args.list == 0 else rw.changed[:args.list]
    for path, actions in shown:
        print("    " + "/".join(actions).ljust(20) + " " + path)
    if args.list and len(rw.changed) > args.list:
        print("    ... 외 " + str(len(rw.changed) - args.list) + "개")

    if rw.misplaced:
        print("  헤더가 1번 줄이 아님 (삽입 건너뜀): " + str(len(rw.misplaced)))
        for path in rw.misplaced[:10]:
            print("    " + path)

    if rw.failed:
        print("  읽기 실패: " + str(len(rw.failed)))
        for path, err in rw.failed[:10]:
            print("    " + path + ": " + err)

    if not args.apply and rw.changed:
        print("\n  실제 반영하려면 같은 명령에 --apply 를 붙인다.")
    return 2 if rw.failed else 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
