local Actions = {}

local reprlib = require("lib.repr")

function Actions.getCommands()
  return {
    { id = 'EntryForm.remove', menuItem = "rmdup", command = Actions.rmdup },
  }
end

--[[
-- Check whether the cell is empty or not
--]]
function isEmptyCell(cell) 
  return cell:getFormattedValue() == ""
end

--[[
-- Find the bounding box of the table and retrieve it as range.
-- If table is empty then the return value is `nil`.
--]]
function findActiveRegion(myOfficeTable) 
  local xMax = myOfficeTable:getColumnsCount()
  local yMax = myOfficeTable:getRowsCount()
  
  local ayMin = yMax
  local ayMax = 0

  local axMin = xMax
  local axMax = 0
  
  local empty = true

  for x=0,xMax-1 do
    for y=0,yMax-1 do

      local pos = DocumentAPI.CellPosition(y, x)
      local cell = myOfficeTable:getCell(pos)

      if isEmptyCell(cell) then
        goto resume
      end
      
      ayMin = math.min(ayMin, y)
      ayMax = math.max(ayMax, y)
      axMin = math.min(axMin, x)
      axMax = math.max(axMax, x)
      empty = false

      ::resume::
    end
  end
  
  if empty then
    return nil
  else
    local range = DocumentAPI.CellRangePosition(ayMin, axMin, ayMax, axMax)
    return myOfficeTable:getCellRange(range)
  end
end

function reverse(t) 
  local i = 1
  local j = #t
  while i <= j do 
    t[i], t[j] = t[j], t[i]
    i = i + 1
    j = j - 1
  end
end

--[[
-- Using integer calculates spreadsheet like 
--]]
function columnIndexToName(idx) 
  local result = {}
    
  -- divide once
  local remainder = idx % 26
  table.insert(result, string.char(65 + remainder))
  idx = idx // 26

  -- and do it more times if necessary
  while idx > 0 do
    idx = idx - 1
    local remainder = idx % 26
    table.insert(result, string.char(65 + remainder))
    idx = idx // 26 
  end
  reverse(result)
  return table.concat(result)
end

--[[
-- Retrieves headers and column names from a region of the table
--]]
function selectionHeaders(region) 
  local myOfficeTable = region:getTable()
  local xMin = region:getBeginColumn()
  local xMax = region:getLastColumn()
  local y = region:getBeginRow()

  -- now iterate the row and get everything
  local result = {}
  for x=xMin,xMax do
    table.insert(
      result,
      {
        index  = x,
        header = myOfficeTable:getCell(DocumentAPI.CellPosition(y, x)):getFormattedValue(),
        column = columnIndexToName(x),
        checked = false
      }
    )
  end
  return result
end

--[[
-- utility function to create ui:ListItems 
-- with different styles
--]]
function generateListItems(headers, style)
  local checked = Forms.CheckState_Checked
  local unchecked = Forms.CheckState_Unchecked

  local result = {}

  for i, v in ipairs(headers) do
    local state = nil
    if v.checked then
      state = checked
    else 
      state = unchecked
    end

    local header = nil
    if style == "column" then
      header = v.column
    elseif v.header == "" then
      header = v.column
    else
      header = v.header
    end
    
    table.insert(result, 
    {
      text = header,
      id = i,
      checkState = state
    })
  end

  return ui:ListItems(result)
end


--[[
-- removes duplicates inside a region
-- where keys is list of columns
--
--  keys = { 
--    index = <number>,
--    ..
--  }
--]]
function removeDuplicates(region, keys)
  local data = {}
  
  local rowStart = region:getBeginRow() 
  local rowEnd = region:getLastRow()
  local sheet = region:getTable()

  for row=rowStart,rowEnd do

    local compound = {}
    for i, key in ipairs(keys) do
      local column = key.index
      local position = DocumentAPI.CellPosition(row, column)
      local cell = sheet:getCell(position)
      table.insert(compound, cell:getFormattedValue())
    end

    table.insert(
      data,
      {
        row = row,
        key = compound
      }
    )
  end

  local order = function (x, y) 
    local n = #x.key

    for i=1,n do
      if x.key[i] > y.key[i] then
        return true
      elseif x.key[i] < y.key[i] then
        return false
      end
    end

    return x.row > y.row
  end

  local isEqual = function (x, y)
    if type(x) ~= type(y) then
      return false
    end

    local n = #x.key
    for i=1,n do
      if x.key[i] ~= y.key[i] then
        return false
      end
    end

    return true
  end

  table.sort(data, order)
  
  local rowsToRemove = {}
  local prev = nil
  for i, current in ipairs(data) do
    if isEqual(prev, current) then
      table.insert(rowsToRemove, prev.row)
    end
    prev = current
  end

  table.sort(rowsToRemove)
  reverse(rowsToRemove)
  
  for i, v in ipairs(rowsToRemove) do
    region:getTable():removeRow(v, 1)
  end
end

function Actions.rmdup(context) 
  context.doWithDocument(function (document)
    local myOfficeTable = EditorAPI.getActiveWorksheet()
    local oldSelection = EditorAPI.getSelection()
    local region = findActiveRegion(myOfficeTable)

    if region == nil then
      EditorAPI.messageBox("table is empty")
      return
    end

    -- now when we have a range to work with we can 
    -- extract the data : columns and headers
    local headers = selectionHeaders(region)
    EditorAPI.setSelection(region)

    --------------------
    -- form layout
    --
    local buttonSize = Forms.Size(120, 28)

    local selectButton = ui:Button {
      Name  = "select",
      Title = "select all",
      Size  = buttonSize
    }

    local clearButton = ui:Button {
      Name  = "clear",
      Title = "clear all",
      Size  = buttonSize
    }

    local toggleHeaders = ui:CheckBox {
      Name  = "toggle-headers",
      Title = "use headers"
    }

    local list = ui:ListBox({
      Name = "columns",
      Items = generateListItems(headers, "column")
    })


    local actions = ui:DialogButtons {}
    actions:addButton("cancel",  Forms.DialogButtonRole_Reject)
    actions:addButton("remove",  Forms.DialogButtonRole_Accept)

    local dialog = ui:Dialog {
      Name = "rmdup-dialog",
      Size = Forms.Size(700, 500),
      Buttons = actions,
      ui:Column {
        ui:Row {
          ui:Label {
            Name = "description",
            Text = "found a region, select keys to delete duplicates"
          }
        },
        ui:Row { selectButton, clearButton, ui:Spacer{}, toggleHeaders },
        ui:Row { list }
      }
    }

    --------------------
    -- wire up events
    --
    toggleHeaders:setOnStateChanged(function (state)
      if (state == Forms.CheckState_Checked) then 
        list:setItems(generateListItems(headers, "headers"))
      else
        list:setItems(generateListItems(headers, "column"))
      end
    end)

    list:setOnItemStateChanged(function (id, state)
      headers[id].checked = state == Forms.CheckState_Checked
    end)

    selectButton:setOnClick(function ()
      for id=1,#headers do
        list:setItemCheckState(id, Forms.CheckState_Checked) 
        headers[id].checked = true
      end
    end)

    clearButton:setOnClick(function ()
      for id=1,#headers do
        list:setItemCheckState(id, Forms.CheckState_Unchecked) 
        headers[id].checked = false
      end
    end)

    dialog:setOnDone(function (code)
      if (code == Forms.DialogCode_Accepted) then
        local keys = {}
        for i, v in ipairs(headers) do
          if v.checked then
            table.insert(keys, v)
          end
        end

        if #keys == 0 then
          EditorAPI.messageBox("no key columns were selected")
        else  
          context.doWithSelection(function (region)
            removeDuplicates(region, keys)
          end)
        end
      end
      --
      -- clean up
      -- restore old selection 
      --
      -- NOTE(ivan): technically we bring oldSelection from
      -- a closure above... aka from the same context of the dialog 
      -- which we use slightly above 
      --
      -- so I guess it works... but does it work all the time? idk
      -- what is the lifetime of context in the end?
      EditorAPI.setSelection(oldSelection)
    end)

    context.showDialog(dialog)
  end)
end

return Actions
