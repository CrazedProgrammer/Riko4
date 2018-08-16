local rif = dofile("/lib/rif.lua")
local crazedLogo = rif.createImage("crazed.rif")

local function showLogo()
  local running = true
  local t = 0
  local w, h = gpu.width, gpu.height
  local scale = 4
  local cw, ch = 16 * scale, 16 * scale

  while running do
    while true do
      local event = {coroutine.yield()}
      if not event[1] then
        break
      elseif event[1] == "key" and event[2] == "escape" then
        running = false
      end
    end

    if t == 50 then
      speaker.play({channel = 1, frequency = 523.25, time = 0.1, shift = 0, volume = 0.06, attack = 0, release = 0})
      --speaker.play({channel = 2, frequency = 128 + math.sin(t * 0.02 + 0.1) * 100, time = 0.0167, shift = 0, volume = 0.06, attack = 0, release = 0})
    end

    if t == 55 then
      speaker.play({channel = 1, frequency = 740, time = 0.5, shift = 0, volume = 0.06, attack = 0, release = 0.5})
    end

    if t == 110 then
      running = false
    end

    gpu.clear(2)

    local hs = t >= 55 and 0 or math.cos(t * 0.03 + 1.5) * 128 + 128
    crazedLogo:render((w - cw) / 2, (h - ch) / 2 + hs, 0, 0, 16, 16, scale)

    gpu.swap()
    t = t + 1
  end
end

showLogo()