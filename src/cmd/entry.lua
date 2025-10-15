local window = require("window")

local Actions = {}

function Actions.getCommands()
  return {
    { id = 'EntryForm.greet', menuItem = "greet", command = Actions.greet },
    { id = 'EntryForm.openWindow', menuItem = "openWindow", command = Actions.openWindow },
    { id = 'EntryForm.closeWindow', menuItem = "closeWindow", command = Actions.closeWindow },
  }
end

function Actions.greet(context)
  context.doWithDocument(function (document)
    local userName = DocumentAPI.UserInfo.name
    if userName then
      EditorAPI.messageBox("hello world, " .. userName, 'test-plugin')
    else
      EditorAPI.messageBox("hello world... " .. "oh wait I don't know your name", 'test-plugin')
    end
  end)
end

function Actions.openWindow(context)
  context.doWithDocument(function (document)
    window.open()
  end)
end

function Actions.closeWindow(context)
  context.doWithDocument(function (document)
    window.close()
  end)
end

return Actions
