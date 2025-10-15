local Actions = {}

local reprlib = require("lib.repr")

function Actions.getCommands()
  return {
    { id = 'EntryForm.run', menuItem = "run", command = Actions.run },
  }
end

function Actions.run(context) 
end

return Actions
