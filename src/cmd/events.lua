local Actions = {}

function Actions.getEvents()
  return {
    { id = "Workbook.open", command = Actions.onOpen }
  }
end

function Actions.onOpen(ctx)
    EditorAPI.messageBox("document opened")
end

return Actions 
