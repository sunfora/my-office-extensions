# Context 

MyOffice использует контекст для того чтобы взаимодействовать с документами.
Здесь я наверное соберу какие-то заметки на тему того как оно в данный момент работает.


## context leak

```lua
local ctx = nil

function Actions.takeContext(context) 
  if ctx == nil then 
    ctx = context
  end

  ctx.doWithDocument(function (document) 
    EditorAPI.messageBox(reprlib.repr(ctx)) 
  end)
end
```

Данный код приводит к крашу.

Steps:

1. выполните команду
2. откройте новый документ
3. закройте старый
4. выполните команду


Почему? Context не глобальная вещь для всего процесса, она имеет лайфтайм и выдаётся нам в зависимости от наших действий.
