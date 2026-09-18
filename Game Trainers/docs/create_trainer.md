## Creating a Trainer

This is the process for building a new trainer in this repo. It is written for an agent picking up a request like *"make a trainer for &lt;game&gt;"*.

Build every new trainer on the **new structure** — a manifest in `CMakeLists.txt` and an option list in `main.cpp`, with everything else shared. [Rhythm Doctor Trainer](../trainers/Rhythm%20Doctor%20Trainer/) is the reference implementation and exercises every UI pattern the framework has.

Most folders in `trainers/` predate that structure and still carry their own hand-written `CMakeLists.txt`, `resources.rc`, `Directory.Build.props`, a checked-in `Mono.dll` and a checked-in font subset, with widget placement written out longhand in `main.cpp`. Those still build, and their `trainer.h` and payload files are the best reference you have for how to reach into a given runtime — read them freely. Do not copy their `main.cpp` or `CMakeLists.txt`, and do not convert them.

---

### Step 1 — Get the game path and the feature list from the user

**Two things must come from the user before you do anything else:**

1. **Where the game is installed.** Either an exact path (`D:\Program Files\Steam\steamapps\common\Rubinite`) or a description specific enough to resolve in one or two directory listings ("it's in my SteamLibrary on E:").
2. **What the cheats should do.** A list of options, each with whether it is a toggle or an apply, and any behaviour the user already knows about the game that constrains it.

> ### ⚠ If the game path is missing, stop and ask
>
> Do not scan the filesystem for it. Do not guess the install root from the game's name, and do not walk every drive looking for a matching folder. Ask the user, and wait.
>
> The path is not a convenience — it is load-bearing. It determines the architecture, the runtime, which tool you dump with, the game version in the manifest, and for Mono trainers it is written **literally into `Mono.csproj`** as the `HintPath` of every assembly reference. A wrong path produces a trainer that compiles against the wrong game.

The feature list matters just as much. "Make a trainer for X" without one leaves you guessing at both the option list and how much of the game you need to understand. Ask for it in the same breath as the path.

---

### Step 2 — Determine the architecture and runtime, then decide whether it is possible

With the path in hand, inspect the install directory. You are answering two questions: **x64 or x86**, and **which of the five runtimes** this game is.

Architecture comes from the PE header of the game executable — the `Machine` field, `0x8664` for x64 and `0x14c` for x86. For Unity games a `<Game>_Data/Mono/x86` or `.../x86_64` folder says the same thing.

Runtime is decided by what is on disk:

| Evidence in the install directory | `RUNTIME` | Base class |
|---|---|---|
| `<Game>_Data/Managed/Assembly-CSharp.dll`, `mono-2.0-bdwgc.dll` | `mono` | `MonoBase` |
| `GameAssembly.dll` + `<Game>_Data/il2cpp_data/Metadata/global-metadata.dat` | `il2cpp` | `Il2CppBase` |
| `Engine/`, `<Game>/Binaries/Win64/`, `.pak` files | `ue` | `UEBase` |
| Electron / NW.js / Chromium layout, `resources/app.asar` | `cdp` | `CDPBase` |
| A native executable with none of the above | `none` | `TrainerBase` |

> ### ⚠ Stop if the runtime is `none`
>
> A `none` trainer has no managed metadata and no injected payload. It reaches the game by writing to addresses found through pointer chains, AOB scans and byte patches — literals like this, from [Outland Trainer/trainer.h](../trainers/Outland%20Trainer/trainer.h):
>
> ```cpp
> std::vector<unsigned int> offsets = {0x005024F0, 0x5C, 0x14, 0x0, 0x14, 0x18, 0x3C};
> return createPointerToggle(moduleName, "SetHealth", offsets, newVal);
> ```
>
> Those numbers come from a live debugging session — running the game, changing a value, scanning memory for what changed, and walking the pointer back to a static base. None of it is derivable from the files on disk, and there is no static analysis that substitutes for it.
>
> **Report to the user that you cannot implement this one, and say why.** Do not invent offsets, do not copy a pointer chain from another game, and do not ship a trainer whose addresses you have not verified. A Cheat Engine MCP might make this tractable later; until one is actually available, `none` is out of scope.

For the other four, confirm you have the tool you need before promising anything. `tools/` is **gitignored**, so a fresh clone has none of them:

| Runtime | Tool | Produces |
|---|---|---|
| `mono` | `tools/ILSpy/ilspycmd.exe` (pin 9.1.0.7988) | C# source from `Assembly-CSharp.dll` |
| `il2cpp` | `tools/Il2CppDumper/Il2CppDumper.exe`, or `tools/Cpp2IL/` | `dump.cs`, method addresses, metadata |
| `ue` | `tools/Dumper-7/` | a C++ SDK of the game's reflected classes |
| `cdp` | the game's own DevTools endpoint | live JS objects, read at runtime |

Put the tool's output in a folder **inside the game's install directory**, not in this repo — dumps are large, game-specific and disposable.

Report the architecture, the runtime and your go/no-go before you start writing code.

---

### Step 3 — Create the trainer folder

The folder name is the trainer's identity. It becomes the product name, the executable name, the window title and that title's key in `translations.json`, so name it `<Game Name> Trainer` exactly as it should appear.

> **A colon cannot appear in a Windows path, so `_` stands in for `": "`.** A game called *MIO: Memories in Orbit* lives in `MIO_Memories in Orbit Trainer/` and ships as `MIO_Memories in Orbit Trainer.exe`, but its window title and its `translations.json` key are `MIO: Memories in Orbit Trainer`. `add_trainer()` restores the colon when it derives the display name, so the folder is the only place the underscore appears. Do not use `_` for anything else in a folder name.

The root `CMakeLists.txt` globs `trainers/*/CMakeLists.txt`, so dropping the folder in is all the registration there is.

A new-structure trainer is **seven files** (plus its payload source):

```
trainers/<Game> Trainer/
    CMakeLists.txt      the manifest - one add_trainer() call
    main.cpp            the option list, and nothing else
    trainer.h           class Trainer : public <Runtime>Base - the cheat logic
    translations.json   en_US and zh_CN, one entry per user-visible string
    logo.jpg            cover art, this exact filename
    Mono.cs             payload, for RUNTIME mono (or IL2CPP.cpp / UE.cpp; cdp and none have none)
    Mono.csproj         for RUNTIME mono only
```

Nothing else belongs in the folder. `resources.rc` is generated into the build tree, the font subset is built into the build tree, and `Mono.dll` is compiled and staged there too. If you see any of those three appear in the source folder, something is wrong.

#### `CMakeLists.txt`

```cmake
add_trainer(
    ARCH            x64
    RUNTIME         mono
    GAME_VERSION    "1.1.1"
    TRAINER_VERSION "1.0"
    WINDOW          800 600
    COLUMN_GAPS     50
    COLUMN_ALIGN    center
    ROW_GAP         10
    INFO_ICON
)
```

> **`GAME_VERSION` is the game's own version, and nothing else.** Sometimes it is easy — a Unity game usually carries it in `<Game>_Data/globalgamemanagers`, and a Steam install has a `buildid` in its `appmanifest_*.acf`. Often it is not: plenty of games only show their version on a title screen or an options menu, with nothing in the install directory that matches it. **When you cannot find it, ask the user to supply it** — they can read it off the running game in seconds. Never substitute something that merely looks like a version: the Unity or Unreal engine version, the Steam build id, or an assembly version are all the wrong number, and a trainer that claims to target "6000.3.10f1" tells the user nothing about whether it matches their copy of the game.

`WINDOW 800 600` is what almost every trainer uses; take it as the house size and change it only if the user asks. Do not tune it to how full the window looks.

Every field above is required and nothing defaults — that is deliberate, so the manifest is a complete description of the trainer. `COLUMN_GAPS` takes one value per column of options, so the number of values *is* the number of columns. The optional flags are `INFO_ICON` (needed if any option has a tooltip or lookup table), `TRANSLATION_EXTRA`, `IL2CPP_API` and `SDK_SOURCES`. The full contract, including what each runtime links and builds, is documented in [cmake/AddTrainer.cmake](../cmake/AddTrainer.cmake).

#### `main.cpp`

The option list, and genuinely nothing else — no window, no widget geometry, no resource loading:

```cpp
#include "TrainerApp.h"

void register_cheats(TrainerUI &ui)
{
    ui.toggle(
        "God Mode",
        [](Trainer &t, bool enable, const Value &, std::string &message)
        { return t.toggleGodMode(enable, message); }
    );
    ...
}
```

The canonical shape — toggles, applies, inputs, sub-rows, lookup tables, columns and separators — is documented in full at the top of [common/include/TrainerApp.h](../common/include/TrainerApp.h). Read that header before writing the list; it is the spec, and this doc will not repeat it.

Two rules that come up every time:

- **A label is the option's key everywhere** — the widget text, the `translations.json` entry and the handler's identity are one string. There is no second name-keyed list to keep in step.
- **Decisions belong in `trainer.h`, not in the option list.** If a sub-toggle means "ignore the number in the box", the handler passes both values through and `trainer.h` resolves what that combination means. `main.cpp` only reports what the widgets say.

#### `trainer.h`

`class Trainer : public MonoBase` (or `Il2CppBase` / `UEBase` / `CDPBase`), with one method per cheat. This is where the game knowledge lives.

When you trace the game's logic, **take the clean path**. If flipping a private field does the job — and it does, because you are writing directly into the object — flip it. There is no requirement to round-trip through a public method just because the game does, and doing so usually adds failure modes rather than removing them.

#### The payload

For `mono`, `Mono.cs` is compiled to `Mono.dll` and injected. `MonoBase` calls `GCMInjection.Initialize()` once, so that class is the entry point; `SendData` logs and `SendResponse` answers the trainer. Copy the dispatcher, `RunCheat` and `Tick` from [Rhythm Doctor Trainer/Mono.cs](../trainers/Rhythm%20Doctor%20Trainer/Mono.cs) — that plumbing is final and should not be redesigned per game.

`Mono.csproj` references the game's own assemblies by absolute `HintPath`. Point them at the install directory the user gave you, and include whatever the cheats actually touch — typically `UnityEngine`, `UnityEngine.CoreModule` and `Assembly-CSharp`.

---

### Step 4 — Report failures properly

**Nothing gets swallowed.** The alert the user sees always opens with the generic line — *"Failed to activate."* — and then carries whatever the failure actually said underneath it. The handler's message, any exception text, and anything written to `std::cerr` all reach the box. What you must never do is return `false` with nothing, which leaves the generic line standing alone; that is the one thing most of the older trainers get wrong.

So the job of a handler is to report the real reason and let `failure_message()` frame it. There are two kinds of reason and both are fine:

- **A refusal the trainer predicts** — "no save is loaded", "this only works in combat" — is written as a `translations.json` key and shown translated.
- **Anything else** — an exception string, a failed memory write, a runtime error from the payload — is passed through verbatim. Do not try to map it onto a key; the exact text *is* the useful part.

Payload-side exceptions should carry their type as well as their message, since `"Object reference not set to an instance of an object."` on its own says very little about where it came from.

How the reason travels back depends on the runtime — `invokeMethodReturn` for `mono` and `il2cpp`, `'ok'` / `'fail: <reason>'` through `executeJS` for `cdp`, a direct write for `none`. The per-runtime contract is documented in the [TrainerApp.h](../common/include/TrainerApp.h) header.

> `UEBase` currently has no string response channel — only `invokeMethod` and `invokeMethodReadBack<T>`. A UE trainer cannot carry a real reason back without riding it in the read-back struct. No trainer does this yet; if you write one, say so rather than quietly leaving the generic line bare.

---

### Step 5 — Translations

`translations.json` has an `en_US` and a `zh_CN` object, and **every user-visible string needs an entry in both**. The embedded font is subsetted to exactly the glyphs this file uses, so a string that is missing here renders as blanks even if it reaches the screen.

Beyond your own option labels, the framework itself needs these keys:

```
"English"  "简体中文"  "<Display Name>"  "Apply"
"Process Name:"  "Process ID:"
"Please run the game first."
"Failed to activate."  "Failed to activate / deactivate."
```

`<Display Name>` is the window title — the folder name with `_` restored to `": "`, as above. Add every predicted refusal your cheats can return alongside them. If a string lives outside this file — in a lookup table built by the payload, say — list its source in `TRANSLATION_EXTRA` so the font subset covers it.

---

### Step 6 — Build and verify

```powershell
.\build_trainers.ps1 -Trainer "<Game> Trainer" -Configuration Release
.\build_trainers.ps1 -List            # what the discovery glob found
.\build_trainers.ps1 -All             # everything, both architectures
```

Build **Debug and Release** before calling it done; they have diverged before. The executable lands in `build/bin/<Game> Trainer/`.

Then confirm the source folder is still clean — `git status` should show your seven files and nothing else. A stray `bin/`, `obj/` or `Mono.dll` in the trainer folder means an IDE design-time build leaked into it.

> ### ⚠ Do not run the trainer
>
> A clean build is where your verification ends. **Do not launch the trainer, do not screenshot it, and do not tune the window size or layout against what you see** — hand all of that to the user.
>
> Running it proves very little anyway: trainers embed `requireAdministrator`, so they run elevated, and synthetic input from a non-elevated shell is blocked by UIPI. You cannot click a widget, and nothing happening tells you nothing about your code.
>
> Report what you built and what you could not check, and let the user exercise it against the running game.

---

### Checklist

1. Game path and feature list obtained from the user — **ask and stop if either is missing**
2. Architecture and runtime determined from the install directory
3. `none` runtime → report that it cannot be implemented, and why
4. Dump tool run, artifacts left inside the game's install directory
5. Folder created with the seven files, named `<Game Name> Trainer`, `_` for any `": "`
6. Manifest complete — every required field explicit
7. `main.cpp` is the option list only; decisions live in `trainer.h`
8. Every failure path carries its real reason through to the alert
9. `translations.json` complete in both languages, framework keys included
10. Debug and Release both build; source folder clean
11. Trainer not launched; layout and behaviour handed to the user to confirm
