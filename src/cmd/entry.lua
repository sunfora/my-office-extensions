local Actions = {}

local reprlib = require("lib.repr")
local window = require("window")

function Actions.getCommands()
  return {
    { id = 'EntryForm.getRuntimeInfo', 
      menuItem = "info", 
      command = Actions.getRuntimeInfo },

    { id = 'EntryForm.startThread', 
      menuItem = "start thread", 
      command = Actions.startThread },

    { id = 'EntryForm.stopThread', 
      menuItem = "stop thread", 
      command = Actions.stopThread },

    { id = 'EntryForm.openWindow', 
      menuItem = "open window", 
      command = Actions.openWindow },

    { id = 'EntryForm.closeWindow', 
      menuItem = "close window", 
      command = Actions.closeWindow },
  }
end

function Actions.startThread(context) 
  window.startThread()
end

function Actions.stopThread(context) 
  window.stopThread()
end

function Actions.openWindow(context) 
  window.openWindow()
end

function Actions.closeWindow(context) 
  window.closeWindow()
end

function Actions.getRuntimeInfo(context) 
  EditorAPI.messageBox(reprlib.repr(window.getRuntimeInfo()))
end

return Actions
