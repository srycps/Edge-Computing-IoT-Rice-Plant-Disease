function doGet(e) {
  Logger.log(JSON.stringify(e));
  var result = 'Ok';

  if (!e || !e.parameter || Object.keys(e.parameter).length === 0) {
    return ContentService.createTextOutput("No param can be sent.");
  }

  var sheet_id = 'xxx'; // Your Spreadsheet ID
  var sheet = SpreadsheetApp.openById(sheet_id).getActiveSheet();
  var newRow = sheet.getLastRow() + 1;
  var rowData = [];
  var CurrDate = new Date();

  rowData[0] = CurrDate;
  var CurrTime = Utilities.formatDate(CurrDate, "Asia/Jakarta", 'HH:mm:ss');
  rowData[1] = CurrTime;

  var classification = "";
  var confidence = "";
  var classCode = "";
  var recommendation = "";
  var reference = "";

  if ('Result' in e.parameter) {
    classification = stripQuotes(e.parameter['Result'].toString());
    rowData[2] = classification;
  } else {
    rowData[2] = "N/A";
  }

  if ('Confidence' in e.parameter) {
    confidence = stripQuotes(e.parameter['Confidence']);
    rowData[3] = confidence;
  } else {
    rowData[3] = "N/A";
  }

  // classification code and detection result
  switch (classification) {
    case '0':
      classCode = 0;
      classification = "Healthy";
      recommendation = "Maintain healthy conditions: maintain irrigation and apply balanced fertilizer. Pay attention to micronutrients: Ca, Zn, chitosan, ammonium phosphate, silica, and microfertilizer.";
      reference = "https://mitrabertani.com/artikel/detail/Budidaya-Tanaman-Padi-Strategi-Padi-Tumbuh-Pesat-dengan-Pemupukan-Tepat";
      break;
    case '1':
      classCode = 1;
      classification = "Brown Spot";
      recommendation = "Remove weeds and diseased plants. Dry the rice fields when the seedling phase reaches its maximum.";
      reference = "https://repository.pertanian.go.id/items/55b34212-9f04-4e2b-ac0f-ee59ea693555";
      break;
    case '2':
      classCode = 2;
      classification = "Tungro";
      recommendation = "Remove and destroy infected plants. Use insecticides (buprofezin/pimetrozin) and balanced fertilizers as recommended by the authorities.";
      reference = "https://distan.bulelengkab.go.id/informasi/detail/artikel/34_mengenal-penyakit-tungro-pada-padi-dan-cara-mengatasinya";
      break;
    case '3':
      classCode = 3;
      classification = "Blight";
      recommendation = "Reduce planting density, dry the land, then open the canopy of the plants. Rotate crops after harvest with legumes (optional).";
      reference = "https://repository.pertanian.go.id/items/55b34212-9f04-4e2b-ac0f-ee59ea693555";
      break;
    default:
      classCode = "-";
      recommendation = "No recommendations because the classification is not recognized.";
      reference = "-";
  }


  // Save only important data to the spreadsheet
  rowData[4] = classCode;

  var newRange = sheet.getRange(newRow, 1, 1, rowData.length);
  newRange.setValues([rowData]);

  if (classification !== "" && confidence !== "") {
    sendToTelegram(classification, confidence, classCode, recommendation, reference);
    result = "Data received and sent to Telegram!";
  } else {
    result = "The data is saved, but it is incomplete to send to Telegram.";
  }

  return ContentService.createTextOutput(result);
}

function stripQuotes(value) {
  return value.replace(/^["']|['"]$/g, "");
}

function sendToTelegram(result, confidence, classCode, recommendation, reference) {
  var token = "xxx"; // Replace w your bot token
  var chat_id = "123"; // Change your chat ID

  var message = "📢 *Rice Plant Detection*\n\n" +
                "🌿 *Result:* " + result + " (Code: " + classCode + ")\n" +
                "📊 *Confidence:* " + confidence + "\n" +
                "💡 *Recommendation:*\n" + recommendation + "\n\n" +
                "🔗 *Reference:*\n" + reference;

  var url = "https://api.telegram.org/bot" + token + "/sendMessage";
  var payload = {
    "chat_id": chat_id,
    "text": message,
    "parse_mode": "Markdown"
  };

  var options = {
    "method": "post",
    "contentType": "application/json",
    "payload": JSON.stringify(payload)
  };

  try {
    UrlFetchApp.fetch(url, options);
  } catch (err) {
    Logger.log("Failed to sent to Telegram: " + err);
  }
}
