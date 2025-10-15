return {
  apiVersion = { major = 1, minor = 0 },

  extensionID = "org.example.test-plugin",
  extensionName = "Hello world!",
  extensionVersion = { major = 0, minor = 1, patch = 0, build = "test" },


  description="test",

  vendor = "lorep ipsum company",

  applicationId = {"MyOffice Spreadsheet"},
  commandsProvider = 'cmd/entry.lua',
  fallbackLanguage = 'EN',


  onLoad = "cmd/start.lua",
  onUnload = "cmd/stop.lua"
}
