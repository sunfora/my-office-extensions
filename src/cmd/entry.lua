local Actions = {}

local reprlib = require("lib.repr")
local window = require("window")

function Actions.getCommands()
  return {
    { id = 'EntryForm.run', menuItem = "run", command = Actions.run },
    { id = 'EntryForm.startThread', menuItem = "start thread", command = Actions.startThread },
    { id = 'EntryForm.stopThread', menuItem = "stop thread", command = Actions.stopThread },
  }
end

function Actions.startThread(context) 
  window.startThread()
end

function Actions.stopThread(context) 
  window.stopThread()
end

function Actions.run(context) 
  EditorAPI.messageBox(reprlib.repr(window.getRuntimeInfo()))
end

return Actions
