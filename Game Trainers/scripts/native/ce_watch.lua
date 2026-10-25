-- Persistent hardware-breakpoint watches for Cheat Engine, driven through scripts/native/ce_mcp.py.
--
-- CheatEngine.Mcp's own capture jobs expire after at most 300 s, which is shorter than a round trip
-- with a human doing something in the game. These watches last until gcmUnwatch() and log each
-- distinct instruction once, with its first register set and the return addresses on its stack.
--
-- For write and access watches the CPU traps after the instruction, so the instruction pointer is
-- the instruction that follows the one that touched the address; disassemble one instruction back
-- to see it. Addresses are named module+offset in whichever module holds them, and the stack scan
-- keeps the values that point into a game module (not a system DLL) - its likely return addresses.

GCM_WATCH = GCM_WATCH or {}

-- 'callers' is an execute watch on a function's first instruction that logs one entry per
-- return address instead of per instruction pointer, which answers "who calls this?".
local triggers = { write = bptWrite, access = bptAccess, execute = bptExecute, callers = bptExecute }

local function moduleOffset(address)
  return getNameFromAddress(address, true, false)
end

local function registers(is64)
  if is64 then
    return string.format(
      'RAX=%X RBX=%X RCX=%X RDX=%X RSI=%X RDI=%X RBP=%X RSP=%X R8=%X R9=%X R10=%X R11=%X R12=%X R13=%X R14=%X R15=%X',
      RAX, RBX, RCX, RDX, RSI, RDI, RBP, RSP, R8, R9, R10, R11, R12, R13, R14, R15)
  end
  return string.format('EAX=%X EBX=%X ECX=%X EDX=%X ESI=%X EDI=%X EBP=%X ESP=%X', EAX, EBX, ECX, EDX, ESI, EDI, EBP, ESP)
end

local function returnAddresses(sp, pointerSize)
  local found = {}
  for i = 0, 63 do
    local value = readPointer(sp + i * pointerSize)
    if value and inModule(value) and not inSystemModule(value) then
      table.insert(found, string.format('+%X:%s', i * pointerSize, moduleOffset(value)))
      if #found >= 8 then break end
    end
  end
  return table.concat(found, ' ')
end

function gcmWatch(name, address, size, trigger)
  if GCM_WATCH[name] then gcmUnwatch(name) end
  local is64 = targetIs64Bit()
  local pointerSize = is64 and 8 or 4
  local watch = { address = address, trigger = trigger, hits = {}, order = {}, total = 0 }
  GCM_WATCH[name] = watch

  debug_setBreakpoint(address, size, triggers[trigger], bpmDebugRegister, function()
    watch.total = watch.total + 1
    local ip, sp = is64 and RIP or EIP, is64 and RSP or ESP
    local key = trigger == 'callers' and readPointer(sp) or ip
    local hit = watch.hits[key]
    if hit == nil then
      hit = { count = 0, regs = registers(is64), stack = returnAddresses(sp, pointerSize) }
      watch.hits[key] = hit
      table.insert(watch.order, key)
    end
    hit.count = hit.count + 1
    -- Returning 1 only keeps the debugger UI away; the thread stays suspended until continued.
    debug_continueFromBreakpoint(co_run)
    return 1
  end)
  return string.format('watching %s at %X (%d bytes, %s)', name, address, size, trigger)
end

function gcmWatchReport(name)
  local watch = GCM_WATCH[name]
  if watch == nil then return 'no watch named ' .. name end
  local label = watch.trigger == 'callers' and 'caller' or 'IP'
  local lines = { string.format('%s at %X: %d hits, %d instructions', name, watch.address, watch.total, #watch.order) }
  for _, key in ipairs(watch.order) do
    local hit = watch.hits[key]
    table.insert(lines, string.format('%s=%X (%s) x%d\n  %s\n  stack %s', label, key, moduleOffset(key), hit.count, hit.regs, hit.stack))
  end
  return table.concat(lines, '\n')
end

function gcmUnwatch(name)
  local watch = GCM_WATCH[name]
  if watch == nil then return 'no watch named ' .. name end
  debug_removeBreakpoint(watch.address)
  GCM_WATCH[name] = nil
  return 'removed ' .. name
end
