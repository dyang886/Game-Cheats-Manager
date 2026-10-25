-- Field recorder for Cheat Engine, driven through scripts/native/ce_mcp.py.
--
-- Samples a memory region on a CE timer and logs, per dword, every value it changes to. Recording
-- runs until stopped, so someone can play through a sequence ("jump, land, double jump") while
-- the agent waits; the report then shows which fields moved in step with it. This replaces the
-- unknown-initial-value scan, whose next-scans have to be timed against the action.

GCM_REC = GCM_REC or {}

-- Per-dword cap, so a field that changes every sample cannot grow without bound. A small region
-- watched over a long session (one position, one counter) wants a larger cap.
function gcmRecord(name, address, size, interval, maxEvents)
  if GCM_REC[name] then gcmRecordStop(name) end
  local MAX_EVENTS = maxEvents or 400
  local rec = { address = address, size = size, prev = nil, events = {}, samples = 0, t0 = getTickCount() }
  GCM_REC[name] = rec

  rec.timer = createTimer(nil, false)
  rec.timer.Interval = interval
  rec.timer.OnTimer = function()
    local bytes = readBytes(address, size, true)
    if bytes == nil then return end
    local now = getTickCount() - rec.t0
    local first = rec.prev == nil
    if first then rec.prev = {} end
    for i = 1, size - 3, 4 do
      local v = bytes[i] | (bytes[i + 1] << 8) | (bytes[i + 2] << 16) | (bytes[i + 3] << 24)
      if first then
        rec.prev[i] = v
        rec.events[i] = { { 0, v } }
      elseif v ~= rec.prev[i] then
        rec.prev[i] = v
        local list = rec.events[i]
        if #list < MAX_EVENTS then list[#list + 1] = { now, v } end
      end
    end
    rec.samples = rec.samples + 1
  end
  rec.timer.Enabled = true
  return string.format('recording %s at %X (%d bytes every %d ms)', name, address, size, interval)
end

-- kind: 'small' = every value fits in a byte (counters, flags, enums); 'float' = plausible floats;
-- 'all' = anything that changed. Offsets that changed more than maxChanges times are skipped.
function gcmRecordReport(name, kind, minChanges, maxChanges)
  local rec = GCM_REC[name]
  if rec == nil then return 'no recording named ' .. name end
  local lines = { string.format('%s at %X: %d samples over %d ms', name, rec.address, rec.samples, getTickCount() - rec.t0) }
  for i = 1, rec.size - 3, 4 do
    local list = rec.events[i]
    local changes = list and #list - 1 or 0
    if changes >= minChanges and changes <= maxChanges then
      local keep, parts = true, {}
      for _, ev in ipairs(list) do
        local v = ev[2]
        if kind == 'small' then
          keep = keep and v >= 0 and v < 256
          parts[#parts + 1] = string.format('%d@%d', v, ev[1])
        elseif kind == 'float' then
          local f = byteTableToFloat(dwordToByteTable(v))
          keep = keep and f == f and math.abs(f) < 1e6 and (f == 0 or math.abs(f) > 1e-4)
          parts[#parts + 1] = string.format('%.3f@%d', f, ev[1])
        else
          parts[#parts + 1] = string.format('%X@%d', v, ev[1])
        end
      end
      if keep then
        table.insert(lines, string.format('+%X (%d changes): %s', i - 1, changes, table.concat(parts, ' ')))
      end
    end
  end
  return table.concat(lines, '\n')
end

function gcmRecordStop(name)
  local rec = GCM_REC[name]
  if rec == nil then return 'no recording named ' .. name end
  rec.timer.Enabled = false
  rec.timer.destroy()
  GCM_REC[name] = nil
  return 'stopped ' .. name
end
