"""Command-line client for the CheatEngine.Mcp gateway (see docs/native_trainer_discovery.md).

Starts tools/native/CheatEngine.Mcp/CheatEngine.Mcp.Gateway.exe over stdio, performs the MCP
handshake, runs one or more calls, and prints each result. Lets scripts - and agents without the MCP
server registered - drive the Cheat Engine that scripts/native/tools.ps1 -Start launched.

    python scripts/native/ce_mcp.py tools [substring]          list tool names (and one-line descriptions)
    python scripts/native/ce_mcp.py schema <tool>              print a tool's input schema
    python scripts/native/ce_mcp.py call <tool> [json-args]    call a tool; instanceId is filled in when
                                                               exactly one Cheat Engine instance is
                                                               running; json-args may be @file.json
    python scripts/native/ce_mcp.py batch <file.json>          run [{"tool": ..., "args": {...}}, ...] in order
    python scripts/native/ce_mcp.py resources                  list the gateway's documents
    python scripts/native/ce_mcp.py read <uri>                 print one document

Persistent watches (ce_watch.lua) - unlike debugger_start_capture they do not expire, so they
survive a round trip with someone doing the in-game action. The debugger must be attached.

    python scripts/native/ce_mcp.py watch <name> <hex-address> <size> <write|access|execute|callers>
    python scripts/native/ce_mcp.py report <name>
    python scripts/native/ce_mcp.py unwatch <name>

Field recordings (ce_record.lua) sample a region on a timer and log each dword's changes, so a
field that moves in step with an in-game action can be picked out afterwards.

    python scripts/native/ce_mcp.py record <name> <hex-address> <hex-size> [interval-ms] [max-events-per-dword]
    python scripts/native/ce_mcp.py record-report <name> [small|float|all] [min-changes] [max-changes]
    python scripts/native/ce_mcp.py record-stop <name>
"""

import json
import os
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
GATEWAY = HERE.parent.parent / "tools" / "native" / "CheatEngine.Mcp" / "CheatEngine.Mcp.Gateway.exe"
WATCH_LUA = HERE / "ce_watch.lua"
RECORD_LUA = HERE / "ce_record.lua"
PROTOCOL_VERSION = "2025-06-18"
# A first scan over a large game's memory runs ~45 s, the gateway's default call timeout.
CALL_TIMEOUT_SECONDS = "900"


class Gateway:
    def __init__(self):
        if not GATEWAY.exists():
            sys.exit(f"Gateway not found at {GATEWAY} - run .\\scripts\\native\\tools.ps1 -Setup first.")
        self.proc = subprocess.Popen(
            [str(GATEWAY)],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            encoding="utf-8",
            bufsize=1,
            env={**os.environ, "MCP_GATEWAY_CALL_TIMEOUT_SECONDS": os.environ.get("MCP_GATEWAY_CALL_TIMEOUT_SECONDS", CALL_TIMEOUT_SECONDS)},
        )
        self.next_id = 1
        self.request("initialize", {
            "protocolVersion": PROTOCOL_VERSION,
            "capabilities": {},
            "clientInfo": {"name": "gcm-ce-mcp", "version": "1.0"},
        })
        self.notify("notifications/initialized")
        self._instance_id = None

    def notify(self, method, params=None):
        message = {"jsonrpc": "2.0", "method": method}
        if params is not None:
            message["params"] = params
        self.proc.stdin.write(json.dumps(message) + "\n")
        self.proc.stdin.flush()

    def request(self, method, params=None):
        request_id = self.next_id
        self.next_id += 1
        message = {"jsonrpc": "2.0", "id": request_id, "method": method, "params": params or {}}
        self.proc.stdin.write(json.dumps(message) + "\n")
        self.proc.stdin.flush()
        while True:
            line = self.proc.stdout.readline()
            if not line:
                raise RuntimeError("gateway closed its output")
            reply = json.loads(line)
            if reply.get("id") != request_id:
                continue  # a notification or a server-initiated request; not ours
            if "error" in reply:
                raise RuntimeError(json.dumps(reply["error"]))
            return reply["result"]

    def tools(self):
        found, cursor = [], None
        while True:
            page = self.request("tools/list", {"cursor": cursor} if cursor else {})
            found.extend(page["tools"])
            cursor = page.get("nextCursor")
            if not cursor:
                return found

    def instance_id(self):
        if self._instance_id is None:
            listing = self.call("instance_list", {}, route=False)
            instances = listing.get("instances", []) if isinstance(listing, dict) else []
            if len(instances) != 1:
                raise RuntimeError(f"expected one Cheat Engine instance, found {len(instances)}: {listing}")
            self._instance_id = instances[0]["instanceId"]
        return self._instance_id

    def call(self, tool, args, route=True):
        args = dict(args)
        if route and tool != "instance_list" and "instanceId" not in args:
            args["instanceId"] = self.instance_id()
        result = self.request("tools/call", {"name": tool, "arguments": args})
        if "structuredContent" in result:
            payload = result["structuredContent"]
        else:
            text = "".join(c.get("text", "") for c in result.get("content", []) if c.get("type") == "text")
            try:
                payload = json.loads(text)
            except ValueError:
                payload = text
        if result.get("isError"):
            raise RuntimeError(json.dumps(payload, ensure_ascii=False))
        return payload

    def lua(self, source):
        result = self.call("lua_execute", {"source": source})
        values = result.get("returnValues", []) if isinstance(result, dict) else []
        return "\n".join(str(v) for v in values)

    def watch_lua(self, call, library=WATCH_LUA):
        # Defining the functions again is harmless: GCM_WATCH / GCM_REC keep the live state.
        return self.lua(library.read_text(encoding="utf-8") + "\nreturn " + call)

    def close(self):
        self.proc.stdin.close()
        self.proc.terminate()


def dump(value):
    print(json.dumps(value, indent=2, ensure_ascii=False) if not isinstance(value, str) else value)


def main(argv):
    if len(argv) < 2:
        sys.exit(__doc__)
    command = argv[1]
    gateway = Gateway()
    try:
        if command == "tools":
            needle = argv[2].lower() if len(argv) > 2 else ""
            for tool in gateway.tools():
                if needle in tool["name"].lower():
                    print(f"{tool['name']:40} {(tool.get('description') or '').splitlines()[0][:110]}")
        elif command == "schema":
            tool = next((t for t in gateway.tools() if t["name"] == argv[2]), None)
            dump(tool or f"no tool named {argv[2]}")
        elif command == "call":
            raw = argv[3] if len(argv) > 3 else "{}"
            if raw.startswith("@"):
                raw = Path(raw[1:]).read_text(encoding="utf-8")
            dump(gateway.call(argv[2], json.loads(raw)))
        elif command == "batch":
            for step in json.loads(Path(argv[2]).read_text(encoding="utf-8")):
                print(f"### {step['tool']}")
                try:
                    dump(gateway.call(step["tool"], step.get("args", {})))
                except RuntimeError as error:
                    print(f"ERROR: {error}")
                    if step.get("stopOnError", True):
                        return 1
        elif command == "watch":
            name, address, size, trigger = argv[2], argv[3], int(argv[4]), argv[5]
            print(gateway.watch_lua(f"gcmWatch({json.dumps(name)}, 0x{address}, {size}, {json.dumps(trigger)})"))
        elif command == "report":
            print(gateway.watch_lua(f"gcmWatchReport({json.dumps(argv[2])})"))
        elif command == "unwatch":
            print(gateway.watch_lua(f"gcmUnwatch({json.dumps(argv[2])})"))
        elif command == "record":
            name, address, size = argv[2], argv[3], argv[4]
            interval = int(argv[5]) if len(argv) > 5 else 33
            cap = int(argv[6]) if len(argv) > 6 else 400
            print(gateway.watch_lua(f"gcmRecord({json.dumps(name)}, 0x{address}, 0x{size}, {interval}, {cap})", RECORD_LUA))
        elif command == "record-report":
            kind = argv[3] if len(argv) > 3 else "small"
            low = int(argv[4]) if len(argv) > 4 else 1
            high = int(argv[5]) if len(argv) > 5 else 60
            print(gateway.watch_lua(f"gcmRecordReport({json.dumps(argv[2])}, {json.dumps(kind)}, {low}, {high})", RECORD_LUA))
        elif command == "record-stop":
            print(gateway.watch_lua(f"gcmRecordStop({json.dumps(argv[2])})", RECORD_LUA))
        elif command == "resources":
            for resource in gateway.request("resources/list").get("resources", []):
                print(f"{resource['uri']:70} {resource.get('name', '')}")
        elif command == "read":
            for content in gateway.request("resources/read", {"uri": argv[2]}).get("contents", []):
                print(content.get("text", ""))
        else:
            sys.exit(__doc__)
    except RuntimeError as error:
        print(f"ERROR: {error}", file=sys.stderr)
        return 1
    finally:
        gateway.close()
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
