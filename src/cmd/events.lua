local Actions = {}
local window = require("window")

MY_GLOBAL_TEST_STRING = "A TEST STRING"

local variable = 1

function Actions.getEvents()
  return {
    { id = "Workbook.Open", command = Actions.onOpen },
    { id = "Worksheet.Change", command = Actions.onSheetChange },
  }
end

function Actions.onOpen(ctx)
  window.replaceContext(context)
  EditorAPI.messageBox("document opened")
end

function Actions.onSheetChange(context, cellRange)
  window.replaceContext(context)
  EditorAPI.messageBox("variable:" .. variable)
  variable = variable + 1
end

return Actions 
