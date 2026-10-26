const {google} = require('googleapis');

function requestBodyToRow(request) {
    return [
        new Date(),
        request.sensorName,
        request.temperature1,
        request.temperature2,
        request.humidity,
        request.luminosity
    ]
}

const reply = (req, res) => {
  var jwt = getJwt();
  var apiKey = getApiKey();
  var spreadsheetId = '1PvxQRoCh0c4rG2plXJsRunFlUngoJd-w0GSecPsbqJI';
  var range = 'A1';

  const requests = Array.isArray(req.body) ? req.body : [req.body];
  const rows = requests.map(requestBodyToRow);

  appendSheetRows(jwt, apiKey, spreadsheetId, range, rows);
  res.status(200).type('text/plain').end('OK');
};
exports.reply = reply;

const functions = require('@google-cloud/functions-framework');

functions.http('helloHttp', reply);

function getJwt() {
  var credentials = require("./meteo-des-loggias-b6c9776b8459.json");
  return new google.auth.JWT(
    credentials.client_email, null, credentials.private_key,
    ['https://www.googleapis.com/auth/spreadsheets']
  );
}

function getApiKey() {
  var apiKeyFile = require("../apikey.json");
  return apiKeyFile.key;
}

function appendSheetRows(jwt, apiKey, spreadsheetId, range, rows) {
  const sheets = google.sheets({version: 'v4'});
  sheets.spreadsheets.values.append({
    spreadsheetId: spreadsheetId,
    range: range,
    auth: jwt,
    key: apiKey,
    valueInputOption: 'RAW',
    resource: {values: rows}
  }, function(err, result) {
    if (err) {
      throw err;
    }
    else {
      console.log('Updated sheet: ' + result.data.updates.updatedRange);
    }
  });
}



