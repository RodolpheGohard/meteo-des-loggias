function test() {
    var jwt = getJwt();
    var apiKey = getApiKey();
    var spreadsheetId = '1PvxQRoCh0c4rG2plXJsRunFlUngoJd-w0GSecPsbqJI';
    var range = 'A1';
  
    const row = requestBodyToRow({
      sensorName: 'test',
      temperature1: 26.72,
      temperature2: 23.12,
      humidity: 42.3,
      luminosity: 623
    });
    
    appendSheetRow(jwt, apiKey, spreadsheetId, range, row);
  }
  
  if (require.main === module) {
      test();
  }