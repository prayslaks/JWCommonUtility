# Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.
"""Install/uninstall project-local links without rewriting agent instruction files."""

import argparse
from contextlib import contextmanager
import copy
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import re
import stat
import sys
import tempfile

from jwcu_context import CONFIG_RELATIVE, PLUGIN_ROOT, STATE_RELATIVE, find_project, read_json, validate_policy


OWNER = "JWCommonUtility"
AGENT_DIRS = {"codex": ".agents/skills", "claude": ".claude/skills"}
SKILL_NAME = re.compile(r"[a-z0-9]+(?:-[a-z0-9]+)*")
DEFAULT_POLICY = {"schema_version": 1, "license_header": None, "excluded_paths": [],
                  "agent": {"instructions": [], "guidance_files": []}}


def exists(path):
    return os.path.lexists(path)


def is_redirect(path):
    if not exists(path):
        return False
    info = path.lstat()
    return stat.S_ISLNK(info.st_mode) or bool(getattr(info, "st_file_attributes", 0) & 0x400)


def snapshot(path):
    if not exists(path):
        return None
    info = path.lstat()
    if stat.S_ISLNK(info.st_mode):
        kind = "symlink"
    elif getattr(info, "st_reparse_tag", 0) == 0xA0000003:
        kind = "junction"
    else:
        raise ValueError(f"Refusing to manage a real file/directory or unknown reparse point: {path}")
    return {"kind": kind, "target": os.readlink(path)}


def safe_path(host, relative, regular_leaf=False):
    parts = PurePosixPath(relative).parts
    if not parts or PurePosixPath(relative).is_absolute() or any(p in (".", "..") or ":" in p or "\\" in p for p in parts):
        raise ValueError(f"Unsafe project-relative path: {relative}")
    path = host
    for index, part in enumerate(parts):
        path = path / part
        if index < len(parts) - 1 or regular_leaf:
            if is_redirect(path):
                raise ValueError(f"Parent/config links are not managed: {path}")
            if index < len(parts) - 1 and exists(path) and not path.is_dir():
                raise ValueError(f"Parent is not a directory: {path}")
    return path


def source_for_destination(relative):
    if relative == "Tools/JWCommonUtility":
        return "Tools"
    if relative == "Docs/JWCommonUtility":
        return "Docs"
    for base in AGENT_DIRS.values():
        prefix = base + "/"
        if relative.startswith(prefix) and SKILL_NAME.fullmatch(relative[len(prefix):]):
            return "Agent/Skills/" + relative[len(prefix):]
    raise ValueError(f"Unrecognized managed destination: {relative}")


def policy_digest(policy):
    clean = {k: v for k, v in policy.items() if k != "_installation"}
    return hashlib.sha256(json.dumps(clean, sort_keys=True, ensure_ascii=True).encode()).hexdigest()


def validate_state(state):
    if state is None:
        return None
    if not isinstance(state, dict):
        raise ValueError("Installation state must be a JSON object.")
    keys = {"owner", "version", "plugin_root", "links", "created_directories", "created_config", "initial_policy_sha256"}
    if set(state) != keys or state["owner"] != OWNER or type(state["version"]) is not int or state["version"] != 1:
        raise ValueError("Unknown installation state; no files have been changed.")
    if not isinstance(state["plugin_root"], str) or not state["plugin_root"]:
        raise ValueError("Invalid recorded plugin root.")
    if type(state["created_config"]) is not bool:
        raise ValueError("Invalid created_config flag.")
    digest = state["initial_policy_sha256"]
    if digest is not None and (not isinstance(digest, str) or not re.fullmatch(r"[0-9a-f]{64}", digest)):
        raise ValueError("Invalid initial policy digest.")
    if state["created_config"] and digest is None:
        raise ValueError("Missing initial policy digest.")
    if not isinstance(state["links"], list) or not isinstance(state["created_directories"], list):
        raise ValueError("Invalid installation link/directory records.")
    seen = set()
    allowed_dirs = {"Config"}
    for entry in state["links"]:
        if not isinstance(entry, dict) or set(entry) != {"path", "source", "kind", "target"}:
            raise ValueError("Invalid link record.")
        if not isinstance(entry["path"], str) or entry["path"] in seen:
            raise ValueError("Invalid or duplicate link path.")
        if entry["source"] != source_for_destination(entry["path"]):
            raise ValueError("Recorded source does not match the destination.")
        if entry["kind"] not in ("symlink", "junction") or not isinstance(entry["target"], str) or not entry["target"]:
            raise ValueError("Invalid recorded link type/target.")
        seen.add(entry["path"])
        allowed_dirs.update(str(p) for p in PurePosixPath(entry["path"]).parents if str(p) != ".")
    if any(not isinstance(d, str) or d not in allowed_dirs for d in state["created_directories"]):
        raise ValueError("Invalid managed directory record.")
    return state


def read_policy(host):
    path = safe_path(host, CONFIG_RELATIVE, regular_leaf=True)
    if not exists(path):
        return copy.deepcopy(DEFAULT_POLICY), None
    if not path.is_file():
        raise ValueError(f"Config is not a regular file: {path}")
    before = path.read_bytes()
    return validate_policy(read_json(path)), before


def read_state(host):
    """Read the installer-owned state, adopting a pre-split record from the policy until it is migrated."""
    path = safe_path(host, STATE_RELATIVE, regular_leaf=True)
    legacy = None
    policy_path = safe_path(host, CONFIG_RELATIVE, regular_leaf=True)
    if exists(policy_path) and policy_path.is_file():
        legacy = validate_policy(read_json(policy_path)).get("_installation")
    if exists(path):
        if not path.is_file():
            raise ValueError(f"Installation state is not a regular file: {path}")
        before = path.read_bytes()
        return validate_state(read_json(path)), before, legacy is not None
    return validate_state(legacy), None, legacy is not None


def commit_state(host, state, before):
    """Write the state document, or remove the file when state is None. Never touches the project policy."""
    path = safe_path(host, STATE_RELATIVE, regular_leaf=True)
    current = path.read_bytes() if exists(path) else None
    if current != before:
        raise ValueError("Installation state changed during the operation; refusing to overwrite it.")
    if state is None:
        if current is not None:
            path.unlink()
        return
    after = (json.dumps(state, indent=2, ensure_ascii=False) + "\n").encode("utf-8")
    if current == after:
        return
    fd, temporary = tempfile.mkstemp(prefix=".jwcu-", suffix=".tmp", dir=path.parent)
    try:
        with os.fdopen(fd, "wb") as stream:
            stream.write(after)
            stream.flush()
            os.fsync(stream.fileno())
        safe_path(host, STATE_RELATIVE, regular_leaf=True)
        if (path.read_bytes() if exists(path) else None) != current:
            raise ValueError("Installation state changed during the operation.")
        os.replace(temporary, path)
    finally:
        if os.path.exists(temporary):
            os.unlink(temporary)


def restore_state(host, before):
    """Rollback helper: put the state file back exactly as the operation found it."""
    path = safe_path(host, STATE_RELATIVE, regular_leaf=True)
    if before is None:
        if exists(path):
            path.unlink()
    else:
        path.write_bytes(before)


def drop_legacy_state(host, policy, before):
    """One-way migration: the record now lives in the state file, so remove it from the shared policy."""
    policy.pop("_installation", None)
    write_policy(host, policy, before)


def check_snapshot(host, entry):
    path = safe_path(host, entry["path"])
    current = snapshot(path)
    if current is not None and current != {k: entry[k] for k in ("kind", "target")}:
        raise ValueError(f"Link target/type was changed by another owner: {path}")
    return current


def mkdir_tracked(host, path, created):
    relative = path.relative_to(host)
    current = host
    for part in relative.parts:
        current /= part
        rel = current.relative_to(host).as_posix()
        safe_path(host, rel, regular_leaf=True)
        if not exists(current):
            current.mkdir()
            created.append(rel)


def prune_empty(host, directories):
    for relative in sorted(set(directories), key=lambda p: len(PurePosixPath(p).parts), reverse=True):
        try:
            path = safe_path(host, relative, regular_leaf=True)
            path.rmdir()  # Never recurse; preserve directories containing user files.
        except (OSError, ValueError):
            pass


@contextmanager
def project_lock(host, created):
    mkdir_tracked(host, host / "Config", created)
    lock = safe_path(host, "Config/.JWCommonUtilityTools.lock", regular_leaf=True)
    try:
        stream = lock.open("x", encoding="utf-8")
    except FileExistsError:
        raise ValueError(f"Installer lock exists: {lock}. If an earlier process stopped, verify it is no longer running before removing that lock.")
    token = f"pid={os.getpid()}\n"
    try:
        with stream:
            stream.write(token)
        yield
    finally:
        try:
            if not is_redirect(lock) and lock.read_text(encoding="utf-8") == token:
                lock.unlink()
        except OSError as exc:
            print(f"Could not clear installer lock: {exc}", file=sys.stderr)


def write_policy(host, policy, before):
    path = safe_path(host, CONFIG_RELATIVE, regular_leaf=True)
    current = path.read_bytes() if exists(path) else None
    if current != before:
        raise ValueError("Config changed during the operation; refusing to overwrite it.")
    text = json.dumps(policy, indent=2, ensure_ascii=False) + "\n"
    if before and b"\r\n" in before:
        text = text.replace("\n", "\r\n")
    after = text.encode("utf-8")
    if before and before.startswith(b"\xef\xbb\xbf"):
        after = b"\xef\xbb\xbf" + after
    if before == after:
        return
    fd, temporary = tempfile.mkstemp(prefix=".jwcu-", suffix=".tmp", dir=path.parent)
    try:
        with os.fdopen(fd, "wb") as stream:
            stream.write(after)
            stream.flush()
            os.fsync(stream.fileno())
        safe_path(host, CONFIG_RELATIVE, regular_leaf=True)
        if (path.read_bytes() if exists(path) else None) != before:
            raise ValueError("Config changed during the operation.")
        os.replace(temporary, path)
    finally:
        if os.path.exists(temporary):
            os.unlink(temporary)


def create_junction(target, path):
    # CPython's Windows primitive avoids shell quoting and does not require symlink privilege.
    import _winapi
    if not hasattr(_winapi, "CreateJunction"):
        raise OSError("This Python runtime cannot create junctions. Enable Windows Developer Mode for symlinks.")
    _winapi.CreateJunction(str(target), str(path))


def create_link(path, target, mode="auto"):
    if exists(path):
        raise ValueError(f"Destination appeared during installation: {path}")
    if mode == "junction":
        if os.name != "nt":
            raise ValueError("Junctions are only available on Windows.")
        create_junction(target, path)
    else:
        try:
            relative = os.path.relpath(target, path.parent)
        except ValueError:  # Different Windows volumes.
            relative = str(target)
        try:
            os.symlink(relative, path, target_is_directory=True)
        except OSError as exc:
            if mode != "auto" or os.name != "nt" or getattr(exc, "winerror", None) != 1314:
                raise
            create_junction(target, path)
    return snapshot(path)


def remove_link(host, entry):
    current = check_snapshot(host, entry)
    if current is None:
        return
    path = safe_path(host, entry["path"])
    if current["kind"] == "junction":
        os.rmdir(path)  # Removes the junction itself, never the target tree.
    else:
        path.unlink()


def restore_link(host, entry):
    path = safe_path(host, entry["path"])
    if exists(path):
        raise ValueError(f"Cannot restore over a new entry: {path}")
    if entry["kind"] == "junction":
        create_junction(entry["target"], path)
    else:
        os.symlink(entry["target"], path, target_is_directory=True)


def desired_links(plugin, agents):
    if not (plugin / "JWCommonUtility.uplugin").is_file():
        raise ValueError(f"Plugin descriptor not found: {plugin}")
    names = []
    for skill in sorted((plugin / "Agent/Skills").iterdir()):
        if skill.is_dir() and (skill / "SKILL.md").is_file():
            if not SKILL_NAME.fullmatch(skill.name):
                raise ValueError(f"Invalid skill folder name: {skill.name}")
            names.append(skill.name)
    if agents and not names:
        raise ValueError("No skills found in the plugin.")
    links = {"Tools/JWCommonUtility": "Tools", "Docs/JWCommonUtility": "Docs"}
    for agent in agents:
        for name in names:
            links[f"{AGENT_DIRS[agent]}/{name}"] = f"Agent/Skills/{name}"
    for source in links.values():
        target = (plugin / source).resolve()
        if not target.is_relative_to(plugin) or not target.is_dir():
            raise ValueError(f"Missing or external plugin source: {source}")
    return links


def install(host, plugin, agents, dry_run=False, link_mode="auto"):
    policy, before = read_policy(host)
    state, state_before, legacy = read_state(host)
    plugin_ref = Path(os.path.relpath(plugin, host)).as_posix() if os.path.splitdrive(plugin)[0] == os.path.splitdrive(host)[0] else str(plugin)
    if state and state["plugin_root"] != plugin_ref:
        raise ValueError("This project is registered to another plugin checkout. Uninstall its links first, then install from this checkout.")
    previous = {e["path"]: e for e in state["links"]} if state else {}
    for entry in previous.values():
        check_snapshot(host, entry)
    desired = desired_links(plugin, agents)
    pending = []
    replacements = {}
    for relative, source in desired.items():
        path = safe_path(host, relative)
        if exists(path):
            if relative not in previous:
                raise ValueError(f"Unmanaged destination already exists; preserving it: {path}")
            try:
                correct_target = path.resolve() == (plugin / source).resolve()
            except (OSError, RuntimeError):
                correct_target = False
            if not correct_target:
                replacements[relative] = previous[relative]
                pending.append((relative, source))
        else:
            pending.append((relative, source))
    print(f"Project: {host}\nPlugin: {plugin}")
    for relative, source in desired.items():
        print(f"{'LINK' if any(p[0] == relative for p in pending) else 'KEEP'} {relative} -> {source}")
    print("CREATE policy" if before is None else "PRESERVE project policy")
    if legacy:
        print(f"MIGRATE installation record -> {STATE_RELATIVE}")
    if dry_run:
        return
    created_dirs = []
    created_links = []
    removed_links = []
    state_written = False
    created_policy = False
    try:
        with project_lock(host, created_dirs):
            if read_policy(host)[1] != before or read_state(host)[1] != state_before:
                raise ValueError("Config changed before the installer acquired its lock.")
            for relative, source in pending:
                path = safe_path(host, relative)
                mkdir_tracked(host, path.parent, created_dirs)
                if relative in replacements:
                    remove_link(host, replacements[relative])
                    removed_links.append(replacements[relative])
                details = create_link(path, plugin / source, link_mode)
                entry = {"path": relative, "source": source, **details}
                created_links.append(entry)
                previous[relative] = entry
            updated_state = {
                "owner": OWNER, "version": 1, "plugin_root": plugin_ref,
                "links": sorted(previous.values(), key=lambda e: e["path"]),
                "created_directories": sorted(set((state["created_directories"] if state else []) + created_dirs)),
                "created_config": state["created_config"] if state else before is None,
                "initial_policy_sha256": state["initial_policy_sha256"] if state else (policy_digest(policy) if before is None else None),
            }
            if state != updated_state or state_before is None:
                commit_state(host, updated_state, state_before)
                state_written = True
            # The project policy is written only when this run creates it, or to retire a pre-split record.
            if before is None:
                write_policy(host, policy, None)
                created_policy = True
            elif legacy:
                drop_legacy_state(host, policy, before)
    except BaseException:
        for entry in reversed(created_links):
            try:
                remove_link(host, entry)
            except (OSError, ValueError) as exc:
                print(f"Rollback could not remove {entry['path']}: {exc}", file=sys.stderr)
        for entry in reversed(removed_links):
            try:
                restore_link(host, entry)
            except (OSError, ValueError) as exc:
                print(f"Rollback could not restore {entry['path']}: {exc}", file=sys.stderr)
        if state_written:
            try:
                restore_state(host, state_before)
            except OSError as exc:
                print(f"Rollback could not restore the installation state: {exc}", file=sys.stderr)
        if created_policy:
            try:
                safe_path(host, CONFIG_RELATIVE, regular_leaf=True).unlink()
            except OSError as exc:
                print(f"Rollback could not remove the generated policy: {exc}", file=sys.stderr)
        prune_empty(host, created_dirs)
        raise
    print("Installed. Existing agent instruction files were not changed.")
    print(f"Policy: {host / CONFIG_RELATIVE}\nState: {host / STATE_RELATIVE}")
    print(f"Guide: {host / 'Docs/JWCommonUtility/AgentSupport.md'}")


def uninstall(host, dry_run=False, remove_config=False):
    policy, before = read_policy(host)
    state, state_before, legacy = read_state(host)
    if state is None:
        print("No recorded installation. Nothing was removed.")
        return
    if remove_config and (not state["created_config"] or policy_digest(policy) != state["initial_policy_sha256"]):
        raise ValueError("Config was pre-existing or edited. Preserve it; --remove-config only removes an unchanged installer-created policy.")
    for entry in state["links"]:
        check_snapshot(host, entry)
        print("UNLINK " + entry["path"])
    print("REMOVE unchanged generated policy" if remove_config else "KEEP project policy; remove installation metadata")
    if dry_run:
        return
    removed = []
    created_dirs = []
    state_written = False
    removed_policy = None
    try:
        with project_lock(host, created_dirs):
            if read_policy(host)[1] != before or read_state(host)[1] != state_before:
                raise ValueError("Config changed before the installer acquired its lock.")
            for entry in state["links"]:
                if check_snapshot(host, entry) is not None:
                    remove_link(host, entry)
                    removed.append(entry)
            commit_state(host, None, state_before)
            state_written = True
            if remove_config:
                path = safe_path(host, CONFIG_RELATIVE, regular_leaf=True)
                if path.read_bytes() != before:
                    raise ValueError("Config changed during uninstall.")
                removed_policy = before
                path.unlink()
            elif legacy:
                drop_legacy_state(host, policy, before)
    except BaseException:
        for entry in reversed(removed):
            try:
                restore_link(host, entry)
            except (OSError, ValueError) as exc:
                print(f"Rollback could not restore {entry['path']}: {exc}", file=sys.stderr)
        if state_written:
            try:
                restore_state(host, state_before)
            except OSError as exc:
                print(f"Rollback could not restore the installation state: {exc}", file=sys.stderr)
        if removed_policy is not None:
            try:
                safe_path(host, CONFIG_RELATIVE, regular_leaf=True).write_bytes(removed_policy)
            except OSError as exc:
                print(f"Rollback could not restore the policy: {exc}", file=sys.stderr)
        prune_empty(host, created_dirs)
        raise
    prune_empty(host, state["created_directories"] + created_dirs)
    print("Uninstalled. Plugin originals and user-authored files were preserved.")


def status(host):
    state, _, legacy = read_state(host)
    print(f"Project: {host}")
    if state is None:
        print("Not installed.")
        return 0
    if legacy:
        print(f"NOTE: pre-split record still in {CONFIG_RELATIVE}; the next install or uninstall moves it to {STATE_RELATIVE}.",
              file=sys.stderr)
    unhealthy = False
    for entry in state["links"]:
        try:
            current = check_snapshot(host, entry)
            path = safe_path(host, entry["path"])
            target_exists = path.is_dir()
            expected = (host / state["plugin_root"] / entry["source"]).resolve()
            label = "OK" if current and target_exists else "MISSING"
            if label == "OK" and path.resolve() != expected:
                label = "STALE"
            unhealthy |= label != "OK"
        except (ValueError, OSError, RuntimeError) as exc:
            label = f"CONFLICT ({exc})"
            unhealthy = True
        print(f"{label}: {entry['path']}")
    return 1 if unhealthy else 0



def resolve_host(value, plugin):
    if value:
        path = Path(value).resolve()
        if path.is_file() and path.suffix == ".uproject":
            project = path
        elif path.is_dir():
            projects = list(path.glob("*.uproject"))
            if len(projects) != 1:
                raise ValueError("--project directory must contain exactly one .uproject.")
            project = projects[0]
        else:
            raise ValueError("--project must be a .uproject file or its containing directory.")
    else:
        candidates = {p for p in (find_project(Path.cwd()), find_project(plugin)) if p}
        if len(candidates) != 1:
            raise ValueError("Cannot select one host project; supply --project explicitly.")
        project = candidates.pop()
    if not isinstance(read_json(project), dict):
        raise ValueError("The .uproject must contain a JSON object.")
    return project.parent.resolve()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)
    for command in ("install", "uninstall", "status"):
        child = sub.add_parser(command)
        child.add_argument("--project", help="Host .uproject or its directory; required for ambiguous/external locations")
        if command != "status":
            child.add_argument("--dry-run", action="store_true")
        if command == "install":
            child.add_argument("--agents", nargs="+", choices=["codex", "claude", "none"], default=["codex", "claude"])
            child.add_argument("--link-mode", choices=["auto", "symlink", "junction"], default="auto")
        if command == "uninstall":
            child.add_argument("--remove-config", action="store_true")
    args = parser.parse_args()
    try:
        plugin = PLUGIN_ROOT.resolve()
        host = resolve_host(args.project, plugin)
        if args.command == "install":
            if "none" in args.agents and len(args.agents) != 1:
                raise ValueError("--agents none cannot be combined with other agents.")
            agents = [] if args.agents == ["none"] else sorted(set(args.agents))
            install(host, plugin, agents, args.dry_run, args.link_mode)
        elif args.command == "uninstall":
            uninstall(host, args.dry_run, args.remove_config)
        else:
            return status(host)
        return 0
    except (OSError, ValueError) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
