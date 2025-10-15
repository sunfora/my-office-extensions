return {
  apiVersion = { major = 1, minor = 0 },
  applicationId = { "MyOffice Spreadsheet" },

  vendor = "sunfora",

  extensionID = "io.github.sunfora.custom-window",
  extensionName = "custom-window",
  description="custom ui demo",

  extensionVersion = { major = 0, minor = 1, patch = 0, build = "" },

  fallbackLanguage = 'EN',

  commandsProvider = 'cmd/entry.lua',
}
