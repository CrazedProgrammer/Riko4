--[[ /gitget
GitHub downloading utility for CC.
Developed by apemanzilla.
Modified to work on Riko4 by CrazedProgrammer.

This requires ElvishJerricco's JSON parsing API.
Direct link: http://pastebin.com/raw.php?i=4nRg9CHU
]]--

-- Whether to write to the terminal as files are downloaded
-- Note that unless checked for this will not affect pre-set start/done code below
local silent = false

-- How many requests are being sent at once
local threadCount = 4

local preset = {
  -- The GitHub account name
  user = nil,
  -- The GitHub repository true
  repo = nil,

  -- The branch or commit tree to download (defaults to 'master')
  branch = nil,

  -- The local folder to save all the files to (defaults to '/')
  path = nil,

  -- Function to run before starting the download
  start = function()
    if not silent then print("Downloading files from GitHub...") end
  end,

  -- Function to run when the download completes
  done = function()
    if not silent then print("Done") end
  end
}

-- Leave the rest of the program alone.
local args = {...}

args[1] = preset.user or args[1]
args[2] = preset.repo or args[2]
args[3] = preset.branch or args[3] or "master"
args[4] = preset.path or args[4] or ""

if #args < 2 then
    print("Usage:\n"..((shell and shell.getRunningProgram()) or "gitget").." <user> <repo> [branch/tree] [path]") error()
end

local function fsGetName(path)
  local dir = fs.getBaseDir(path)
  if dir == "." then
    return path
  else
    return path:sub(#dir + 2)
  end
end

local function save(data,file)
  local file = file:gsub("%%20"," ")
  if not (fs.exists(string.sub(file,1,#file - #fsGetName(file))) and fs.isDir(string.sub(file,1,#file - #fsGetName(file)))) then
    if fs.exists(string.sub(file,1,#file - #fsGetName(file))) then fs.delete(string.sub(file,1,#file - #fsGetName(file))) end
    fs.mkdir(string.sub(file,1,#file - #fsGetName(file)))
  end
  local f = fs.open(file,"w")
  f:write(data)
  f:close()
end

local function download(url, file)
  save(net.get(url):readAll(),file)
end

if not json then
  download("http://pastebin.com/raw.php?i=4nRg9CHU","/tmp/json.lua")
  local func = loadfile("/tmp/json.lua")
  local json = setmetatable({}, { __index = _G })
  setfenv(func, json)
  func()
  getfenv().json = json
end

preset.start()
if args[4] ~= "" then fs.mkdir(args[4]) end
local rawdata = net.get("https://api.github.com/repos/"..args[1].."/"..args[2].."/git/trees/"..args[3].."?recursive=1"):readAll()
local data = json.decode(rawdata)
if data.message and data.message:find("API rate limit exceeded") then error("Out of API calls, try again later") end
if data.message and data.message == "Not found" then error("Invalid repository",2) else
  for k,v in pairs(data.tree) do
    -- Make directories
    if v.type == "tree" then
      fs.mkdir(fs.combine(args[4],v.path))
      if not hide_progress then
      end
    end
  end
  local filecount = 0
  local downloaded = 0
  local nextfile = 1
  local threads = 0
  local urls = {}
  local paths = {}
  local failed = {}
  for k,v in pairs(data.tree) do
    -- Send all HTTP requests (async)
    if v.type == "blob" then
      v.path = v.path:gsub("%s","%%20")
      local url = "https://raw.github.com/"..args[1].."/"..args[2].."/"..args[3].."/"..v.path,fs.combine(args[4],v.path)
      urls[#urls + 1] = url
      paths[url] = fs.combine(args[4],v.path)
      filecount = filecount + 1
    end
  end
  while downloaded < filecount do
    for i = threads + 1, threadCount do
      if nextfile <= #urls then
        net.request(urls[nextfile])
        nextfile = nextfile + 1
        threads = threads + 1
      else
        break
      end
    end
    local e, a, b = coroutine.yield()
    if e == "netSuccess" then
      save(b:readAll(),paths[a])
      downloaded = downloaded + 1
      threads = threads - 1
      print(paths[a], 16)
      shell.draw()
    elseif e == "netFailure" then
      print("Failed to download " .. a .. ": " .. b .. ". Trying again...", 8)
      net.request(a)
    end
  end
end
preset.done()
