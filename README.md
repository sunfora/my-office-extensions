# Создание плагина для MyOffice

Это небольшое summary данного [видео](https://www.youtube.com/watch?v=wRzM8U4P8Dk).
И пример реализации плагина по аналогии того что был создан в виде.

## Общие сведения

Любой MyOffice плагин имеет достаточно простую структуру.

- Package.lua (входная точка, конфигурация) 
- METAINF/LICENSE (файл лицензии)
- bin (для динамически линкуемых бинарников dll или so)

Package.lua является входной точкой приложения. 
И описывает детально уже что конкретно и откуда вызывать.

В нём есть часть полей которые необходимы. Есть часть полей которые не очень необходимы.
Пример минимального Package.lua

```lua
return {
  -- версия API надстроек MyOffice
  apiVersion = { major = 1, minor = 0 },
  -- приложение с которым мы будем работать
  applicationId = { "MyOffice Spreadsheet" },

  -- имя нашей компании
  vendor = "sunfora",

  -- id плагина (рекомендуется нотация принятая в Java : 
  --             обратный порядок записи некоторого доменного имени)
  extensionID = "io.github.sunfora.rmdup",
  -- название плагина
  extensionName = "rmdup",
  -- описание того что делает плагин
  description="removes duplicate rows from a given file",
  -- версия плагина
  extensionVersion = { major = 0, minor = 1, patch = 0, build = "" },


  -- параметр интернационализации
  -- язык по умолчанию
  fallbackLanguage = 'EN',

  -- файл определяющий команды плагина
  -- доступные пользователю
  commandsProvider = 'cmd/entry.lua',
}
```

В данный момент плагин и редактор взаимодействуют через два механизма.

1. Команды
2. Эвенты 

Эвенты не обязательны `eventsProvider`, команды обязательны `commandsProvider`.

Команды представляют из собой общую для всех плагинов менюшку, где пользователь выбирает конкретный плагин и вызывает конкретную команду. 
Эвенты в свою очередь представляют из себя события редактора которые инциируются во время взаимодействия пользователя с документом.
Например `Worksheet.Change`.

> Какого отдельного механизма встроиться в event loop либо вызвать команду самостоятельно в данный момент не предусмотрено.
  Подробнее в [руководстве по плагинам для MyOffice](https://support.myoffice.ru/upload/iblock/7d3/t7vxo3rlj9oqs95ho717f19ploe1140d/MyOffice_SDK_Lua_Extensions_3.5_Guide.pdf) страница 131

Из стандартных механизмов взаимодействия с пользователем предлагается использовать компоненты ui.
Которые позволяют создать по-сути дела Qt виджет-форму с которой пользователь взаимодействует пока не закроет.
Активная форма блокирует редактирование и какие либо действия.

В целом, ui достаточно простенький: есть текст, текстовые поля ввода, есть кнопки, чекбоксы, радио, списки и прочие привычные стандартные элементы.
Так же есть отдельный файловый диалог.

Можно менять какие-то цвета, можно слегка настраивать лейаут (размер вещей, выравнивание текста, конкретные сдивиги с помощью spacer).
Но в целом в пределах парадигмы того, что это просто форма ввода.
С которой пользователь взаимодействует и закрывает, чтобы команда потом исполнилась и на этом всё.

И финальное о чём стоит сказать - плагин собирают утилитой mox, просто отправляя его в папку src, которая по-сути дела пакует всё в zip архив.
И подписывает сертификатом. Без сертификата по-умолчанию плагин не ставится, но это можно обойти, подробнее в руководстве как всегда.

## Команды

`commandsProvider` представляет из себя просто файл на lua, 
который обязательно должен содержать функцию которая генерирует по сути конфиг для самих комад.

в самом каком-то простом варианте это что-то в таком стиле:

```lua
local Actions = {}

function Actions.getCommands()
  return {
    { id = 'EntryForm.remove', menuItem = "rmdup", command = Actions.rmdup },
  }
end

function Actions.rmdup(context) 
  EditorAPI.messageBox("greet")
end

return Actions
```

Что в евентах, что в командах используется контекст, контекст генерирует редактор и контекст позволяет работать либо с приложением, либо с текущим документом, либо с текущим выделением. Он на самом деле нужен еще и для того, чтобы например показать диалог. В частности пример плагина который демонстрируется в видео как раз использует контекст выделения чтобы получить информацию о текущем документе который редактирует пользователь.

## Как реализован плагин в видео?

В плагине есть единственная команда удалить дубликаты, для того чтобы это сделать плагин сначала анализирует текущую таблицу и смотрит какие вообще столбцы существуют, затем инициирует диалог где пользователь может выбрать что-то конкретное что его интересует.
Затем создаётся временная табличка где с помощью встроенной функции countif проводится пересчёт.

Из каких-то существенных нюансов можно выделить лишь то наверное, что при составлении команды для countif используется текущая локаль, потому что синтаксис немного разный на разных языках... Примерно как в эксельке. `=COUNTIF(A$1:A$7, A1)` vs `=СЧЁТЕСЛИ(A$1:A$7; A1)`. 

Но вообще говоря я не нахожу чтобы это так работало в современных версиях. Возможно потому что это зависит от системной локали.
В общем на самом деле походу можно спокойно использовать `=COUNTIF(A:A; A1)`.

Моя идея в том чтобы не делать так в любом случае, потому что мы неправильно будем работать с `a ; bc` и `ab ; c`.

## ui

UI формы достаточно straightforward вещь.
Мы по сути дела берем строим что-то вроде дерева элементов.
И затем привязывает к этим элементам эвенты так же как во многих других retained mode gui фреймворках.

В данном случае это просто обёртка над Qt.


В нашем случае это например что-то подобное:
```lua
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
```

И затем мы добавляем эвенты к кнопкам, ну например для того чтобы обновить ListBox с помощью кнопок
```lua
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
```

Само взаимодействие с диалогом происходит через `ui:DialogButtons` с определенными ролями. 
И сам диалог спавнится через `context.showDialog`. Кстати говоря, в отличие от messageBox по какой-то причине `showDialog` не блокирующая операция, несмотря на то, что сам по себе диалог модальная вещь.

```lua
dialog:setOnDone(function (code)
  if (code == Forms.DialogCode_Accepted) then
    -- ..
  end
end)

context.showDialog(dialog)
```
