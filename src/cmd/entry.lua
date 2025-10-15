local Actions = {}

local reprlib = require("lib.repr")

function Actions.getCommands()
  return {
    { id = 'EntryForm.take', menuItem = "take-context", command = Actions.takeContext },
  }
end

local ctx = nil

function Actions.takeContext(context) 
  if ctx == nil then 
    ctx = context
  end

  ctx.doWithDocument(function (document) 
    EditorAPI.messageBox(reprlib.repr(ctx)) 
  end)
end

return Actions
