## Native trainer discovery

How to build a `RUNTIME none` trainer — a native game with no managed metadata — by finding its cheats live with Cheat Engine driven through MCP. It picks up where [create_trainer.md](create_trainer.md) Step 2 lands on `none`; everything about the trainer folder, the manifest, translations and the build is still that document's.

[Ratchet & Clank_Rift Apart Trainer](../trainers/Ratchet%20&%20Clank_Rift%20Apart%20Trainer/) was built this way and is the worked example: every hook in its `trainer.h` says where the value lives and why that site was chosen.

---

### What the loop needs

A native cheat ends as one of three things in `trainer.h`: an AOB-located **code cave** at the instruction that touches the value, an AOB-located **byte patch**, or a **value write** through a pointer chain from a static root. Getting there needs a way to make the game change state and to see it change. The tools cover the seeing; the changing is the user, who is in the loop for anything that needs gameplay.

The scripts are in `scripts/native/` and the tools they drive in `tools/native/`. None of them is tied to a game: they take the module from their arguments, handle x86 and x64, and name addresses `module+offset` in whichever module holds them.

| Need | Tool |
|---|---|
| Is the code on disk the code that runs? What classes exist? | `recon.py` — packer hints, RTTI class → vtable map |
| Scans, memory, disassembly, signatures, AA patches | Cheat Engine 7.7 + CheatEngine.Mcp, via `ce_mcp.py` |
| What writes / reads / calls this — across a user's round trip | `ce_mcp.py watch` (`ce_watch.lua`) |
| Which field moves when the user does X | `ce_mcp.py record` (`ce_record.lua`) |
| Objects by class, fields by name, vtable diffs | `ce_inspect.py` |
| What the HUD shows, without stealing focus | `capture_window.ps1` |
| Do the trainer's signatures still match this build? | `verify_aobs.py` |

---

### Setup

```powershell
.\scripts\native\tools.ps1 -Setup   # tools/native: CheatEngine (7.7), dotnet (10.x), CheatEngine.Mcp
.\scripts\native\tools.ps1 -Start   # launch that CE; the plugin loads from its autorun and shows "MCP: Enabled"
```

`-Setup` expects Cheat Engine 7.7 in `tools/native/CheatEngine` — put it there, or pass `-CheatEngineSource <folder>` to have it copied — and downloads the rest. `-Start` sets `DOTNET_ROOT` for that CE only, which works because CE runs `asInvoker` — launched elevated it would lose the variable.

`ce_mcp.py` drives the gateway over stdio, so a session without the MCP server registered can still use it, and scripts can batch calls:

```
python scripts/native/ce_mcp.py tools [substring]     python scripts/native/ce_mcp.py call <tool> '{"json": "args"}'
python scripts/native/ce_mcp.py schema <tool>         python scripts/native/ce_mcp.py call <tool> @args.json
```

`instanceId` is filled in when one CE is running. The client raises the gateway's call timeout to 900 s — a first scan over a large game runs about 45 s, which is exactly the default.

---

### The loop

**1. Static recon.** `python -I scripts/native/recon.py <exe> --grep health,ammo,jump` — or the DLL, when the game's code lives in one. A packed `.text` means signatures cannot be checked on disk and the live dump is the only source. The RTTI map (written to `<game dir>/gcm_dump/recon.json`) is what makes the rest fast: class names for every vtable pointer, and every instance of a class findable by scanning for its vtable. The game version usually sits in the window title (`process_list` / `GetWindowText`) — the Steam build id is not it.

**2. Attach** with `process_attach`; attach the debugger (`debugger_attach`, `windows`) only when a watch needs it.

**3. Find the value.** Ask the user for the number on screen, `scan_first` in a **named** scanner (synchronous; `main` is asynchronous), have them change it, `scan_next`. Expect most survivors to be **HUD copies** — UI data models that mirror the real value. Separate them by **write-and-observe**: write a new value into one candidate and read the others; the authoritative one is the one the copies follow. A value no int scan finds may be a float — trace the HUD instead: a write watch on the HUD copy gives the code that fetches the real value (ammo in Rift Apart came back as a float through a virtual getter).

**4. Find the object, not just the address.** Read the pointer-sized slots before the value and name the vtable pointers (`ce_inspect.py dump`). From a class name, `instances` finds every live object of it; `vtables` shows which methods a class overrides. Components often carry a pointer cache that lists their siblings, and one static pointer to the player object (search the module for the object's address) is usually all the root a trainer needs.

**5. Find the code.** `ce_mcp.py watch <name> <addr> <size> write|access|execute|callers`, then `report <name>`. Each distinct instruction is logged once with its first registers and the return addresses on its stack; `callers` keys an execute watch on a function's entry by return address instead. Write and access watches trap after the instruction, so disassemble one back (`code_disassemble` with `before`). The debug registers allow four watches; `unwatch` the ones you are done with — every hit is a Lua callback, and a watch on a hot address slows the game.

**6. Timing-dependent behaviour** (a jump counter, an animation state): `record <name> <addr> <size>` samples a region on a timer while the user plays a described sequence, then `record-report` filters fields by how they changed. When the question is *which of these knobs changes X*, write a **probe**: a CE Lua timer that steps through candidates in phases while the recorder measures the outcome (speed from position samples), with a safety cut-off and a teleport filter. This is how Rift Apart's run-speed tunable was found after four failed hooks.

**7. Pick the hook site and check it is not shared.** Compare the function across classes with `vtables`; a slot two classes share is code that also runs for enemies, and the cave must check the object's vtable (`CaveBuilder::vtableCheck`). `aob_generate_signature` with `verify` gives a unique signature; check it offline too with `verify_aobs.py` once it is in `trainer.h`.

**8. Prove it live.** Write the cave as a CE Auto Assembler script and `asm_apply` it; have the user exercise it; `asm_release_patch` it. Only then translate it into `trainer.h` with `CaveBuilder`. **Release every patch before the user runs the trainer** — a trainer's AOB scan cannot find code CE has already replaced with a jump.

---

### Talking to the user

The user is the game-state oracle, and every round trip costs them. Batch the actions one message asks for ("fire twice, take one hit, run for five seconds"), say exactly what to do and for how long, and set every watch and recorder up *before* sending the message. MCP capture jobs (`debugger_start_capture`) expire after at most 300 s — too short for a round trip; use `ce_mcp.py watch`, which does not expire. Read the HUD yourself with `capture_window.ps1` when the user only needs to stand still.

---

### Pitfalls this pipeline has already paid for

- **A Lua breakpoint callback must continue the thread.** Returning 1 only keeps CE's debugger window away; without `debug_continueFromBreakpoint(co_run)` the game freezes on the first hit. `ce_watch.lua` does this.
- **Scaling a position step can compound.** If locomotion measures the speed the character actually moved and feeds it into the next step, any extra distance grows exponentially — even ×1.1. Look for the speed tunable instead, with a probe.
- **Hooking a shared setter is dangerous** even with a vtable check when other callers pass unrelated values (placement, respawn). Hook the path the feature uses.
- **Making the player immune is not god mode** if enemies read the same flag to decide whether to attack. Rejecting the damage where it arrives keeps their behaviour.
- **A game crash wedges the plugin's leases** on the dead process; restart CE (`tools.ps1 -Start`) rather than fighting `runtime_release_resources`.
- **Objects move on a checkpoint reload.** Re-resolve from the static root; never carry heap addresses across one.
- **Hyper-V is on** on this machine, so CE's DBVM is unavailable. User-mode hardware breakpoints have been enough for single-player games.

---

### Writing the trainer

Each cheat is a named modification on [TrainerBase.h](../common/include/TrainerBase.h), and `disable(name)` undoes any of them — the header's opening comment is the reference:

| What the cheat does | Call |
|---|---|
| Overwrite instructions in place | `patchCode(name, CodeSite::aob(module, "0F 85 ?? ..", offset), {0x90, 0xE9})` |
| Jump to a code cave | `hookCode(name, site, overwriteLen, [](CaveBuilder &cave) { ... })` |
| Keep a value at a pointer chain's end | `holdValue(name, {module, {rva, off, ...}}, value)` / `freezeValue<T>(name, chain)` |
| Keep tunables at original × factor | `scaleValues(name, chain, {fieldOffsets}, factor)` — originals restored on disable |
| One-off read or write | `readValue(chain, out)` / `writeValue(chain, value)` |
| Change a live cave's data | `writeCaveData(name, label, value)` |

A `CodeSite` is an AOB as Cheat Engine prints it — the only signature format, and the one `verify_aobs.py` reads — or `CodeSite::rva(module, rva)` for code no signature pins down. A null module searches all executable memory, which is where JIT-compiled code lives.

The cave body gets a [CaveBuilder](../common/include/CaveBuilder.h) that already knows the cave's address, the hooked address and the bytes the jump replaced: `original()` replays them, `jmpBack()` returns after them, `label` / `jumpTo` branch inside the cave, `ripRelative` (x64) or `address` (x86) reach data placed after the code, `originalBranchTarget` re-encodes a `call`/`jmp rel32` that was overwritten, and `vtableCheck` guards a function shared with other classes. `build()` throws on a label that is never bound, and the throw reaches the alert. An overwritten instruction that addresses relative to itself cannot be replayed with `original()` — re-encode it, or pick another site.

RVAs (a vtable, a static root, a tunable) are pinned to the build; name them in one place in `trainer.h` with the build they come from. Signatures locate code and survive more updates; check them after every game update:

```
python -I scripts/native/verify_aobs.py "trainers/<Game> Trainer" "<game dir>/<Game>.exe" [<game dir>/<code>.dll ...]
```

Pass every module the trainer's signatures are in; each has to match exactly once across them.
