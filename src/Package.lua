return {
  apiVersion = { major = 1, minor = 0 },
  applicationId = { "MyOffice Spreadsheet" },

  vendor = "sunfora",

  extensionID = "io.github.sunfora.context-leak",
  extensionName = "context-leak",
  description="crashes the program",

  extensionVersion = { major = 0, minor = 1, patch = 0, build = "" },

  fallbackLanguage = 'EN',

  commandsProvider = 'cmd/entry.lua',
}
