"""Drafts dated project updates with a local model and publishes them to the profile site."""

import argparse
import json
import re
import shutil
import subprocess
import sys
import tempfile
import urllib.error
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
CONFIG = ROOT / "devlog" / "site-update.json"
STAGING = ROOT / "devlog" / "site-updates" / "staging"
PUBLISHED = ROOT / "devlog" / "site-updates" / "published"
DATE = re.compile(r"^\d{4}-\d{2}-\d{2}$")
UPDATE_NAME = re.compile(r"^(\d{4}-\d{2}-\d{2})-update(?:-\d+)?\.md$")
NO_MODEL = 3
SECTION_CAP = 40_000

SYSTEM = """You write short public progress updates for a personal engineering website. The project \
is a hobby C++ rewrite of the firmware for an STM32 self-balancing robot kit. Readers are engineers.

Rules:
- Output 2 to 4 Markdown bullets, 60 to 100 words in total. Each bullet starts with "- " and is one line.
- One change or finding per bullet, past tense, plain and factual. No adjectives, no hype, no lessons.
- Write for an engineer who has never seen the code: say what was built, found or measured and why it \
mattered. Leave out argument lists, pin modes, getter counts and test-case names. Avoid internal labels \
such as stage, step, phase, gate or test-row numbers.
- Put the most significant result first.
- Include the measured number when there is one. Copy every number exactly as the sources give it; \
never compute, round, convert or invent a number.
- Keep qualifiers. If the sources say something is suspected, provisional, not yet confirmed, or only \
built and not tested on hardware, the bullet must say so.
- Leave out anything the sources say was wrong, withdrawn, retracted or later disproved. The LATER \
SESSION section exists only for that check: never report events from it.
- Leave out people's names, AI assistants, agents and models, local file paths, serial port names, and \
documentation or workflow housekeeping (session notes, skills, status files). A newly designed stage or \
plan can be news; the act of writing notes is not.
- Do not repeat what the PREVIOUS UPDATES already said.
- Put identifiers, registers, functions and code values in backticks.
- No links of any kind.
- Write a dollar sign as \\$.
- If nothing in the session would interest a reader of the site, reply exactly: SKIP: <one-line reason>
- If the session has two clearly unrelated publishable threads you may write two updates separated by \
a line containing only ===. Usually write one.
- Reply with the bullets only: no title, heading, frontmatter or preamble."""

BANNED = [
    (re.compile(r"\b(Claude|Codex|Qwen|OpenCode|GPT|ChatGPT|Ben)\b"), "names a person, agent or model"),
    (re.compile(r"\b[A-Za-z]:\\"), "contains a drive path"),
    (re.compile(r"/Users/|/home/"), "contains a home path"),
    (re.compile(r"\bCOM\d+\b"), "names a serial port"),
    (re.compile(r"(?<!\\)\$"), "has an unescaped $"),
    (re.compile(r"\b(STATUS|AGENTS|CLAUDE|SKILL)\.md\b"), "mentions workflow files"),
]
NUMBER = re.compile(r"0x[0-9A-Fa-f]+|\d+(?:,\d{3})*(?:\.\d+)?")
LINK = re.compile(r"\]\(([^)]*)\)")


def fail(msg, code=1):
    print(f"site-update: {msg}", file=sys.stderr)
    sys.exit(code)


def read(path):
    return Path(path).read_text(encoding="utf-8", errors="replace")


def git(cwd, *args, check=True):
    r = subprocess.run(["git", *args], cwd=cwd, capture_output=True, text=True, encoding="utf-8")
    if check and r.returncode != 0:
        fail(f"git {' '.join(args)} failed in {cwd}:\n{r.stderr.strip()}")
    return r


def load_config():
    if not CONFIG.exists():
        fail(f"no {CONFIG.relative_to(ROOT)}; this project has no site configured", 2)
    cfg = json.loads(read(CONFIG))
    cfg.setdefault("ledger", {})
    site = (ROOT / cfg["site"]).resolve()
    if not (site / ".git").exists():
        fail(f"site repo not found at {site}")
    cfg["_site"] = site
    return cfg


def save_config(cfg):
    out = {k: v for k, v in cfg.items() if not k.startswith("_")}
    out["ledger"] = dict(sorted(cfg["ledger"].items()))
    CONFIG.write_text(json.dumps(out, indent=2, ensure_ascii=False) + "\n", encoding="utf-8", newline="\n")


def session_dates():
    return sorted(p.stem for p in (ROOT / "sessions").glob("*.md") if DATE.match(p.stem))


def main_ref(cfg):
    return f"origin/{cfg.get('branch', 'main')}"


def fetch_site(cfg):
    if git(cfg["_site"], "fetch", "-q", "origin", check=False).returncode != 0:
        print("site-update: fetch failed; using the last fetched site state", file=sys.stderr)


def site_files(cfg, folder):
    """Returns {name: text} for files under folder on the site's main branch."""
    site, ref = cfg["_site"], main_ref(cfg)
    names = git(site, "ls-tree", "--name-only", f"{ref}:{folder}", check=False).stdout.split()
    return {n: git(site, "show", f"{ref}:{folder}/{n}").stdout for n in names}


def frontmatter(text):
    m = re.match(r"^---\n(.*?)\n---\n?(.*)$", text.replace("\r\n", "\n"), re.S)
    if not m:
        return {}, text
    fields = dict(re.findall(r"^(\w+):\s*(.*?)\s*$", m.group(1), re.M))
    return fields, m.group(2).strip()


def published_updates(cfg):
    """Returns {name: body} for updates on main plus those staged locally."""
    out = {}
    for name, text in site_files(cfg, f"src/content/logs/{cfg['project']}").items():
        if UPDATE_NAME.match(name):
            out[name] = frontmatter(text)[1]
    for p in STAGING.glob("*.md") if STAGING.exists() else []:
        if UPDATE_NAME.match(p.name):
            out[p.name] = frontmatter(read(p))[1]
    return out


def devlog_entries(day):
    notes = ROOT / "devlog" / "notes.md"
    if not notes.exists():
        return ""
    parts = re.split(r"(?m)^(?=## )", read(notes))
    return "\n".join(p for p in parts if p.startswith("## ") and (day in p.splitlines()[0] or f"**Date:** {day}" in p))


def commits_between(after, upto):
    raw = git(ROOT, "log", "--reverse", "--date=short", "--format=%x1e%ad%x1f%h%x1f%s%x1f%b").stdout
    lines = []
    for rec in raw.split("\x1e"):
        if not rec.strip():
            continue
        day, sha, subject, body = (rec.split("\x1f") + ["", "", ""])[:4]
        if (after is None or day > after) and day <= upto:
            lines.append(f"{sha} {day} {subject}\n{body.strip()}".strip())
    return "\n\n".join(lines)


def cap(text):
    return text if len(text) <= SECTION_CAP else text[:SECTION_CAP] + "\n[truncated]"


def build_sources(cfg, day):
    dates = session_dates()
    i = dates.index(day)
    prev_day = dates[i - 1] if i > 0 else None
    next_day = dates[i + 1] if i + 1 < len(dates) else None
    issues = "\n\n".join(read(p) for p in sorted((ROOT / "issues").glob(f"{day}*.md")) if "(" not in p.name)
    updates = published_updates(cfg)
    earlier = [n for n in sorted(updates) if n[:10] < day][-2:]
    examples = [n for n in sorted(updates) if n not in earlier][:2] if len(earlier) < 2 else []
    sections = [
        ("SESSION " + day, read(ROOT / "sessions" / f"{day}.md")),
        ("ISSUES " + day, issues),
        ("DEVLOG NOTES " + day, devlog_entries(day)),
        ("COMMITS since the previous session", commits_between(prev_day, day)),
        ("LATER SESSION (only to check for withdrawn findings; do not report)",
         read(ROOT / "sessions" / f"{next_day}.md") if next_day else ""),
        ("PREVIOUS UPDATES (do not repeat)", "\n\n".join(updates[n] for n in earlier)),
        ("STYLE EXAMPLES (format only; not about this session)", "\n\n".join(updates[n] for n in examples)),
    ]
    return "\n\n".join(f"### {title}\n\n{cap(body.strip()) or '(none)'}" for title, body in sections)


def endpoint_get(url, timeout):
    with urllib.request.urlopen(url, timeout=timeout) as r:
        return json.load(r)


def model_id(cfg):
    try:
        return endpoint_get(cfg["endpoint"].rstrip("/") + "/models", 5)["data"][0]["id"]
    except (urllib.error.URLError, OSError, KeyError, IndexError) as e:
        fail(f"no model reachable at {cfg['endpoint']} ({e}); drafting deferred", NO_MODEL)


def chat(cfg, model, messages):
    body = json.dumps({"model": model, "messages": messages, "temperature": 0.2, "max_tokens": 12000}).encode()
    req = urllib.request.Request(cfg["endpoint"].rstrip("/") + "/chat/completions", body,
                                 {"Content-Type": "application/json"})
    with urllib.request.urlopen(req, timeout=1800) as r:
        text = json.load(r)["choices"][0]["message"]["content"] or ""
    return re.sub(r"(?s)<think>.*?</think>", "", text).strip()


def parse_reply(text):
    """Returns ("skip", reason) or ("updates", [body, ...])."""
    text = re.sub(r"^```\w*\n|\n```$", "", text.strip())
    if text.upper().startswith("SKIP"):
        return "skip", text.split(":", 1)[-1].strip()
    bodies = []
    for chunk in re.split(r"(?m)^===\s*$", text):
        bullets = []
        for line in chunk.splitlines():
            line = line.rstrip()
            if re.match(r"^\s*[-*]\s+", line):
                bullets.append("- " + re.sub(r"^\s*[-*]\s+", "", line))
            elif line.strip() and bullets:
                bullets[-1] += " " + line.strip()
            elif line.strip():
                bullets.append(line.strip())
        if bullets:
            bodies.append("\n".join(bullets))
    return "updates", bodies[:2]


def normalise(text):
    return re.sub(r"(?<=\d),(?=\d)", "", text.replace("−", "-"))


def grounding(sources):
    """The day's own sources: numbers from later sessions or other updates don't count."""
    return sources.split("### LATER SESSION")[0]


def check(body, sources):
    errors, warnings = [], []
    sources = grounding(sources)
    lines = [l for l in body.splitlines() if l.strip()]
    if any(not l.startswith("- ") for l in lines):
        errors.append("every line must be a '- ' bullet")
    if not 2 <= len(lines) <= 4:
        errors.append(f"{len(lines)} bullets; want 2-4")
    prose = LINK.sub("]", body)
    words = len(re.findall(r"\S+", prose))
    if not 40 <= words <= 130:
        errors.append(f"{words} words; want 60-100")
    elif not 60 <= words <= 100:
        warnings.append(f"{words} words; target 60-100")
    src = normalise(sources)
    for tok in sorted(set(NUMBER.findall(normalise(prose)))):
        if not re.search(r"(?<![\d.])" + re.escape(tok) + r"(?!\d)", src):
            errors.append(f"number {tok} is not in the sources")
    if LINK.search(body) or re.search(r"https?://", body):
        errors.append("contains a link; updates carry none")
    for pattern, why in BANNED:
        if pattern.search(body):
            errors.append(why)
    if body.count("`") % 2:
        errors.append("unbalanced backticks")
    return errors, warnings


def free_name(day, taken):
    n = 1
    while True:
        name = f"{day}-update.md" if n == 1 else f"{day}-update-{n}.md"
        if name not in taken:
            return name
        n += 1


def render(day, project, body):
    return f"---\nformat: update\npubDate: {day}\nproject: {project}\n---\n\n{body}\n"


def report(name, errors, warnings):
    status = "FAIL" if errors else "ok"
    print(f"  {name}: {status}")
    for e in errors:
        print(f"    error: {e}")
    for w in warnings:
        print(f"    warn:  {w}")


def draft_one(cfg, model, day):
    for old in STAGING.glob(f"{day}*") if STAGING.exists() else []:
        old.unlink()
    sources = build_sources(cfg, day)
    messages = [{"role": "system", "content": SYSTEM},
                {"role": "user", "content": f"Write the update for {day}.\n\n{sources}"}]
    print(f"{day}: drafting", flush=True)
    for attempt in (1, 2):
        reply = chat(cfg, model, messages)
        kind, payload = parse_reply(reply)
        if kind == "skip":
            cfg["ledger"][day] = {"state": "skipped", "note": "model: " + payload}
            print(f"  skipped: {payload}")
            return
        results = [(b, *check(b, sources)) for b in payload]
        problems = [e for _, errs, _ in results for e in errs]
        if not payload:
            problems = ["no bullets in the reply"]
        if not problems or attempt == 2:
            break
        messages += [{"role": "assistant", "content": reply},
                     {"role": "user", "content": "Fix these problems and reply with the corrected bullets only:\n- "
                      + "\n- ".join(problems)}]
    STAGING.mkdir(parents=True, exist_ok=True)
    (STAGING / f"{day}.sources.md").write_text(sources, encoding="utf-8", newline="\n")
    taken = set(published_updates(cfg))
    files = []
    for body, errors, warnings in results:
        name = free_name(day, taken)
        taken.add(name)
        (STAGING / name).write_text(render(day, cfg["project"], body), encoding="utf-8", newline="\n")
        files.append(name)
        report(name, errors, warnings)
    cfg["ledger"][day] = {"state": "staged", "files": files}


def cmd_status(cfg, _args):
    ledger = cfg["ledger"]
    pending = [d for d in session_dates() if d not in ledger]
    staged = [d for d, e in ledger.items() if e.get("state") == "staged"]
    print("pending:", " ".join(pending) or "none")
    print("staged: ", " ".join(staged) or "none")


def cmd_draft(cfg, args):
    if args.date:
        if args.date not in session_dates():
            fail(f"no sessions/{args.date}.md")
        days = [args.date]
    else:
        days = [d for d in session_dates() if d not in cfg["ledger"]][: args.limit]
    if not days:
        print("nothing to draft")
        return
    model = model_id(cfg)
    fetch_site(cfg)
    for day in days:
        draft_one(cfg, model, day)
        save_config(cfg)


def staged_files():
    return sorted(p for p in STAGING.glob("*.md") if UPDATE_NAME.match(p.name)) if STAGING.exists() else []


def check_staged(cfg):
    ok = True
    for p in staged_files():
        day = p.name[:10]
        fields, body = frontmatter(read(p))
        src = STAGING / f"{day}.sources.md"
        errors, warnings = check(body, read(src) if src.exists() else "")
        if fields.get("pubDate") != day or fields.get("format") != "update" or fields.get("project") != cfg["project"]:
            errors.append("frontmatter must be format: update, pubDate matching the filename, and this project")
        if not src.exists():
            errors.append(f"missing {src.name}")
        report(p.name, errors, warnings)
        ok = ok and not errors
    return ok


def cmd_check(cfg, _args):
    fetch_site(cfg)
    if not staged_files():
        print("nothing staged")
    elif not check_staged(cfg):
        sys.exit(1)


def cmd_publish(cfg, args):
    files = staged_files()
    if not files:
        print("nothing staged")
        return
    fetch_site(cfg)
    if not check_staged(cfg):
        fail("fix the staged files before publishing")
    site, ref, branch = cfg["_site"], main_ref(cfg), cfg.get("branch", "main")
    days = sorted({p.name[:10] for p in files})
    tree = Path(tempfile.mkdtemp(prefix="site-update-")) / "wt"
    git(site, "worktree", "add", "--detach", str(tree), ref)
    try:
        dest = tree / "src" / "content" / "logs" / cfg["project"]
        dest.mkdir(parents=True, exist_ok=True)
        for p in files:
            if (dest / p.name).exists():
                fail(f"{p.name} already exists on {branch}; rename it to the next -update-<n>")
            shutil.copyfile(p, dest / p.name)
        git(tree, "add", "--", *[str(dest / p.name) for p in files])
        label = f"update {days[0]}" if len(days) == 1 else f"updates {days[0]} to {days[-1]}"
        git(tree, "commit", "-q", "-m", f"content({cfg['project']}): {label}")
        if args.dry_run:
            print(git(tree, "show", "--stat", "--format=%s", "HEAD").stdout)
            print("dry run: not pushed")
            return
        if git(tree, "push", "-q", "origin", f"HEAD:{branch}", check=False).returncode != 0:
            git(tree, "fetch", "-q", "origin")
            git(tree, "rebase", "-q", ref)
            git(tree, "push", "-q", "origin", f"HEAD:{branch}")
        sha = git(tree, "rev-parse", "--short", "HEAD").stdout.strip()
    finally:
        git(site, "worktree", "remove", "--force", str(tree), check=False)
        shutil.rmtree(tree.parent, ignore_errors=True)
    PUBLISHED.mkdir(parents=True, exist_ok=True)
    for day in days:
        names = [p.name for p in files if p.name.startswith(day)]
        cfg["ledger"][day] = {"state": "published", "files": names, "commit": sha}
        for f in [*[STAGING / n for n in names], STAGING / f"{day}.sources.md"]:
            if f.exists():
                shutil.move(str(f), PUBLISHED / f.name)
    save_config(cfg)
    print(f"published {len(files)} update(s) to {branch} as {sha}")


def cmd_skip(cfg, args):
    if args.date not in session_dates():
        fail(f"no sessions/{args.date}.md")
    for f in STAGING.glob(f"{args.date}*") if STAGING.exists() else []:
        f.unlink()
    cfg["ledger"][args.date] = {"state": "skipped", "note": args.note}
    save_config(cfg)
    print(f"{args.date}: skipped")


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    sub = ap.add_subparsers(dest="cmd", required=True)
    sub.add_parser("status")
    d = sub.add_parser("draft")
    g = d.add_mutually_exclusive_group(required=True)
    g.add_argument("--date")
    g.add_argument("--catch-up", action="store_true")
    d.add_argument("--limit", type=int, default=5)
    sub.add_parser("check")
    p = sub.add_parser("publish")
    p.add_argument("--dry-run", action="store_true")
    s = sub.add_parser("skip")
    s.add_argument("--date", required=True)
    s.add_argument("--note", required=True)
    args = ap.parse_args()
    cfg = load_config()
    {"status": cmd_status, "draft": cmd_draft, "check": cmd_check,
     "publish": cmd_publish, "skip": cmd_skip}[args.cmd](cfg, args)


if __name__ == "__main__":
    main()
