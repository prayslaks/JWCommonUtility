# Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.
"""Lightweight C++ review hints, deliberately not a parser or control-flow proof."""

import re


LITERALS = re.compile(r'//[^\n]*|/\*[\s\S]*?\*/|R"(?P<delimiter>[^\s()\\]{0,16})\([\s\S]*?\)(?P=delimiter)"|"(?:\\[\s\S]|[^"\\])*"|\'(?:\\[\s\S]|[^\'\\])*\'')
INCLUDE = re.compile(r'^\s*#\s*include\s*["<]([^">]+)[">]', re.M)
DEFAULT_LOGS = ("UE_LOG", "UE_LOGFMT")


def mask_cpp(text, *, directives=True):
    """Blank comments/literals but retain offsets and lines, including raw strings."""
    blank = lambda m: re.sub(r"[^\n]", " ", m.group())
    result = LITERALS.sub(blank, text)
    if directives:
        result = re.sub(r"^[ \t]*#(?:[^\n]*\\\n)*[^\n]*", blank, result, flags=re.M)
    return result


def matching(text, start, opening="(", closing=")"):
    depth = 0
    for i in range(start, len(text)):
        if text[i] == opening:
            depth += 1
        elif text[i] == closing:
            depth -= 1
            if depth == 0:
                return i
    return None


def line_at(text, offset):
    return text.count("\n", 0, offset) + 1


def direct_statements(body):
    """Yield top-level semicolon statements; nested branches stay in their segment."""
    braces = parens = brackets = 0
    start = 0
    for i, char in enumerate(body):
        if char == "{": braces += 1
        elif char == "}":
            braces -= 1
            if braces == parens == brackets == 0 and re.match(r"\s*(?:(?:if|else|for|while|switch|try|catch)\b|\{)", body[start:i]):
                start = i + 1
        elif char == "(": parens += 1
        elif char == ")": parens -= 1
        elif char == "[": brackets += 1
        elif char == "]": brackets -= 1
        elif char == ";" and braces == parens == brackets == 0:
            yield start, body[start:i + 1]
            start = i + 1


def guard_log_candidates(text, policy=None, *, all_guards=False):
    policy = policy or {}
    code = mask_cpp(text)
    logs = set(DEFAULT_LOGS) | set(policy.get("log_functions", []))
    compact = lambda s: re.sub(r"\s+", "", s)
    excluded = {compact(c) for c in policy.get("excluded_conditions", [])}
    candidates = []
    for match in re.finditer(r"\bif\s*(?:constexpr\s*)?\(", code):
        opening = code.index("(", match.start())
        close = matching(code, opening)
        if close is None:
            continue
        condition = code[opening + 1:close].strip()
        if compact(condition) in excluded:
            continue
        start = close + 1
        while start < len(code) and code[start].isspace(): start += 1
        if start >= len(code): continue
        if code[start] == "{":
            end = matching(code, start, "{", "}")
            if end is None: continue
            start += 1
        else:
            # Only a direct braceless return; nested braceless ifs are handled on their own.
            direct = re.match(r"return\b[^;]*;", code[start:])
            if not direct: continue
            end = start + direct.end()
        body = code[start:end]
        for offset, statement in direct_statements(body):
            ret = re.match(r"\s*return\b(?P<value>[\s\S]*?);\s*$", statement)
            if not ret:
                continue
            prefix = body[:offset]
            logged = False
            for _, prior in direct_statements(prefix):
                call = re.match(r"\s*([A-Za-z_]\w*(?:::[A-Za-z_]\w*)*)\s*\(", prior)
                if call and call.group(1) in logs:
                    logged = True
            if logged:
                break
            value = ret.group("value").strip()
            normal_gate = bool(re.fullmatch(r"!?\s*(?:\w+\s*->\s*)?(?:HasAuthority|IsLocallyControlled|IsLocalController)\s*\(\s*\)", condition))
            negative = bool(re.search(r"!(?!=)|==\s*nullptr|nullptr\s*==|\b(?:Failed|Invalid)\w*\b", condition))
            failure_value = value in ("false", "nullptr")
            if not all_guards and (normal_gate or not (negative or failure_value)):
                break
            return_offset = start + offset + statement.index("return")
            diagnostics = sorted(set(re.findall(r"\b(?:UE_CLOG|ensure\w*|check\w*|" +
                                               "|".join(re.escape(n) for n in sorted(logs)) + r")\s*(?=\()", condition + "\n" + prefix)))
            candidates.append({
                "rule": "GUARD_LOG_REVIEW", "line": line_at(text, match.start()),
                "return_line": line_at(text, return_offset),
                "condition": " ".join(text[opening + 1:close].split()),
                "return_value": " ".join(text[return_offset + len("return"):start + offset + len(statement) - 1].split()),
                "priority": "normal-path-review" if normal_gate else ("failure-review" if failure_value else "guard-review"),
                "nearby_diagnostics": diagnostics,
                "reason": "이 분기의 return 앞에서 무조건 호출되는 등록 로그를 찾지 못함. 정상 복귀·호출자/헬퍼 진단 여부를 확인.",
            })
            break
    return candidates


def forward_declaration_candidates(text):
    """Conservative filename/type heuristic with explicit evidence, never an edit."""
    code = mask_cpp(text)
    candidates = []
    # Comment-masked source with original include strings restored for discovery.
    visible = mask_cpp(text, directives=False)
    for inc in INCLUDE.finditer(text):
        # Ignore includes inside comments/strings (their '#' was blanked).
        hash_offset = text.index("#", inc.start(), inc.end())
        if visible[hash_offset] != "#": continue
        header = inc.group(1).replace("\\", "/")
        leaf = header.rsplit("/", 1)[-1]
        if leaf == "CoreMinimal.h" or leaf.endswith(".generated.h") or not leaf.endswith(".h"):
            continue
        stem = leaf[:-2]
        if not re.fullmatch(r"[A-Za-z_]\w*", stem): continue
        names = sorted(set(re.findall(r"\b([AUFSI]" + re.escape(stem) + r"\w*)\b", code)))
        # 같은 헤더가 주는 심볼 중 하나라도 완전한 정의를 요구하면 include 를 지울 수 없다.
        # 그 경우 다른 심볼을 전방 선언해도 얻는 것이 없으므로 이 include 전체를 후보에서 뺀다.
        definition_required = False
        include_candidates = []
        for name in names:
            uses = list(re.finditer(r"\b" + re.escape(name) + r"\b", code))
            variables = set()
            eligible = True
            for use in uses:
                before = code[max(0, use.start() - 100):use.start()]
                after = code[use.end():]
                if re.search(r"\b(?:class|struct)\s+$", before) and re.match(r"\s*;", after):
                    continue
                if re.search(r"::\s*$|\b(?:sizeof|alignof)\s*\(\s*$", before):
                    eligible = False; break
                pointer = re.match(r"\s*(?:const\s*)?[*&]\s*(?:const\s+)?([A-Za-z_]\w*)?", after)
                wrapper = re.search(r"\b(?:TObjectPtr|TWeakObjectPtr|TSoftObjectPtr|TSubclassOf)\s*<\s*(?:const\s+)?$", before)
                if pointer:
                    if pointer.group(1): variables.add(pointer.group(1))
                elif wrapper and re.match(r"\s*>\s*([A-Za-z_]\w*)", after):
                    variables.add(re.match(r"\s*>\s*([A-Za-z_]\w*)", after).group(1))
                else:
                    eligible = False; break
            if not eligible:
                definition_required = True
                continue
            if not variables: continue
            # Inline use of the pointee requires more than a pointer declaration.
            if any(re.search(r"\b" + re.escape(var) + r"\s*(?:->|\.|\[)|\bdelete\s+" + re.escape(var) + r"\b", code)
                   for var in variables):
                definition_required = True
                continue
            kind = "struct" if name.startswith("F") else "class"
            include_candidates.append({"rule": "FORWARD_DECL_REVIEW", "line": line_at(text, hash_offset),
                                       "include": header, "type": name, "declaration": f"{kind} {name};",
                                       "priority": "include-review",
                                       "reason": "헤더 이름 기반 타입 추정; 포인터/참조·객체 래퍼 사용 후보. 실제 선언 종류·namespace·다른 include 제공 심볼·UHT를 확인."})
        if not definition_required:
            candidates.extend(include_candidates)
    return candidates
