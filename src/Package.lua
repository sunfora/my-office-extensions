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
