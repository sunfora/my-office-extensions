local window = require("window")

local Actions = {}

function Actions.getCommands()
  return {
    { id = 'EntryForm.greet', menuItem = "greet", command = Actions.greet },
    { id = 'EntryForm.openWindow', menuItem = "openWindow", command = Actions.openWindow },
    { id = 'EntryForm.closeWindow', menuItem = "closeWindow", command = Actions.closeWindow },
    { id = 'EntryForm.showDlg', menuItem = "showDlg", command = Actions.showDlg},
  }
end

function Actions.greet(context)
  window.replaceContext(context)
  context.doWithDocument(function (document)
    local userName = DocumentAPI.UserInfo.name
    if userName then
      EditorAPI.messageBox("hello world, " .. userName, 'test-plugin')
    else
      EditorAPI.messageBox("hello world... " .. "oh wait I don't know your name", 'test-plugin')
    end
  end)
end

function Actions.showDlg(context) 
  window.replaceContext(context)
  context.doWithDocument(function (document)
    local dialog = ui:Dialog {
      Size = Forms.Size(600, 300),
      ui:Column {
        ui:Row {
          ui:Spacer {},
          ui:Column {},
          ui:Column {}
        }
      }
    }
    context.showDialog(dialog)
  end)
end

function select(editor, context, i, j)
  context.doWithDocument(function (document)
    local sheet = editor.getActiveWorksheet()
    local cellpos = DocumentAPI.CellPosition(j, i)
    local rngpos  = DocumentAPI.CellRangePosition(j, i, j, i)
    local range = sheet:getCellRange(rngpos)
    editor.setSelection(range)
    local cell = sheet:getCell(cellpos)
    return cell:getFormattedValue()
  end)
end

function updateNumber(editor, context, i, j)
  context.doWithDocument(function (document)
    local sheet = editor.getActiveWorksheet()
    local cellpos = DocumentAPI.CellPosition(j, i)
    local rngpos  = DocumentAPI.CellRangePosition(j, i, j, i)
    local range = sheet:getCellRange(rngpos)
    editor.setSelection(range)
    local cell = sheet:getCell(cellpos)
    if (cell:getFormattedValue() == '') then 
      cell:setNumber(0)
    else
      local prev = tonumber(cell:getFormattedValue())
      cell:setNumber(prev + 1)
    end
    return cell:getFormattedValue()
  end)
end

function Actions.openWindow(context)
  window.open(context);
end

function Actions.closeWindow(context)
  window.replaceContext(context)
  window.close()
end

return Actions
