/** The MIT License (MIT)

Copyright (c) 2018 David Payne

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "OpenWeatherMapClient.h"

OpenWeatherMapClient::OpenWeatherMapClient(String ApiKey, int CityIDs[], int cityCount, boolean isMetric, String language) {
  updateCityIdList(CityIDs, cityCount);
  updateLanguage(language);
  myApiKey = ApiKey;
  setMetric(isMetric);
}

void OpenWeatherMapClient::updateWeatherApiKey(String ApiKey) {
  myApiKey = ApiKey;
}

void OpenWeatherMapClient::updateLanguage(String language) {
  lang = language;
  if (lang == "") {
    lang = "en";
  }
}

void OpenWeatherMapClient::updateWeather() {
  WiFiClient weatherClient;
  String apiGetData = "GET /data/2.5/group?id=" + myCityIDs + "&units=" + units + "&cnt=1&APPID=" + myApiKey + "&lang=" + lang + " HTTP/1.1";

  Serial.println("Getting Weather Data");
  Serial.println(apiGetData);
  result = "";
  if (weatherClient.connect(servername, 80)) {  //starts client connection, checks for connection
    weatherClient.println(apiGetData);
    weatherClient.println("Host: " + String(servername));
    weatherClient.println("User-Agent: ArduinoWiFi/1.1");
    weatherClient.println("Connection: close");
    weatherClient.println();
  } 
  else {
    Serial.println("connection for weather data failed"); //error message if no client connect
    Serial.println();
    return;
  }

  while(weatherClient.connected() && !weatherClient.available()) delay(1); //waits for data
 
  Serial.println("Waiting for data");

  // Check HTTP status
  char status[32] = {0};
  weatherClient.readBytesUntil('\r', status, sizeof(status));
  Serial.println("Response Header: " + String(status));
  if (strcmp(status, "HTTP/1.1 200 OK") != 0) {
    Serial.print(F("Unexpected response: "));
    Serial.println(status);
    return;
  }

    // Skip HTTP headers
  char endOfHeaders[] = "\r\n\r\n";
  if (!weatherClient.find(endOfHeaders)) {
    Serial.println(F("Invalid response"));
    return;
  }

  const size_t bufferSize = 710;
  DynamicJsonBuffer jsonBuffer(bufferSize);

  weathers[0].cached = false;
  weathers[0].error = "";
  // Parse JSON object
  JsonObject& root = jsonBuffer.parseObject(weatherClient);
  if (!root.success()) {
    Serial.println(F("Weather Data Parsing failed!"));
    weathers[0].error = "Weather Data Parsing failed!";
    return;
  }

  weatherClient.stop(); //stop client

  if (root.measureLength() <= 150) {
    Serial.println("Error Does not look like we got the data.  Size: " + String(root.measureLength()));
    weathers[0].cached = true;
    weathers[0].error = (const char*)root["message"];
    Serial.println("Error: " + weathers[0].error);
    return;
  }
  int count = root["cnt"];

  for (int inx = 0; inx < count; inx++) {
    weathers[inx].lat = (const char*)root["list"][inx]["coord"]["lat"];
    weathers[inx].lon = (const char*)root["list"][inx]["coord"]["lon"];
    weathers[inx].dt = (const char*)root["list"][inx]["dt"];
    weathers[inx].city = (const char*)root["list"][inx]["name"];
    weathers[inx].country = (const char*)root["list"][inx]["sys"]["country"];
    weathers[inx].temp = (const char*)root["list"][inx]["main"]["temp"];
    weathers[inx].humidity = (const char*)root["list"][inx]["main"]["humidity"];
    weathers[inx].condition = (const char*)root["list"][inx]["weather"][0]["main"];
    weathers[inx].cloudiness = (const char*)root["list"][inx]["clouds"]["all"];
    weathers[inx].wind = (const char*)root["list"][inx]["wind"]["speed"];
    weathers[inx].weatherId = (const char*)root["list"][inx]["weather"][0]["id"];
    weathers[inx].description = (const char*)root["list"][inx]["weather"][0]["description"];
    weathers[inx].icon = (const char*)root["list"][inx]["weather"][0]["icon"];

    Serial.println("lat: " + weathers[inx].lat);
    Serial.println("lon: " + weathers[inx].lon);
    Serial.println("dt: " + weathers[inx].dt);
    Serial.println("city: " + weathers[inx].city);
    Serial.println("country: " + weathers[inx].country);
    Serial.println("temp: " + weathers[inx].temp);
    Serial.println("humidity: " + weathers[inx].humidity);
    Serial.println("condition: " + weathers[inx].condition);
    Serial.println("cloudiness: " + weathers[inx].cloudiness);
    Serial.println("wind: " + weathers[inx].wind);
    Serial.println("weatherId: " + weathers[inx].weatherId);
    Serial.println("description: " + weathers[inx].description);
    Serial.println("icon: " + weathers[inx].icon);
    Serial.println();
    
  }
}

String OpenWeatherMapClient::roundValue(String value) {
  float f = value.toFloat();
  int rounded = (int)(f+0.5f);
  return String(rounded);
}

void OpenWeatherMapClient::updateCityIdList(int CityIDs[], int cityCount) {
  myCityIDs = "";
  for (int inx = 0; inx < cityCount; inx++) {
    if (CityIDs[inx] > 0) {
      if (myCityIDs != "") {
        myCityIDs = myCityIDs + ",";
      }
      myCityIDs = myCityIDs + String(CityIDs[inx]); 
    }
  }
}

void OpenWeatherMapClient::setMetric(boolean isMetric) {
  if (isMetric) {
    units = "metric";
  } else {
    units = "imperial";
  }
}

String OpenWeatherMapClient::getWeatherResults() {
  return result;
}

String OpenWeatherMapClient::getLat(int index) {
  return weathers[index].lat;
}

String OpenWeatherMapClient::getLon(int index) {
  return weathers[index].lon;
}

String OpenWeatherMapClient::getDt(int index) {
  return weathers[index].dt;
}

String OpenWeatherMapClient::getCity(int index) {
  weathers[index].city.replace("oe", "ö");
  return weathers[index].city;
  
}

String OpenWeatherMapClient::getCountry(int index) {
  return weathers[index].country;
}

String OpenWeatherMapClient::getTemp(int index) {
  return weathers[index].temp;
}

String OpenWeatherMapClient::getTempRounded(int index) {
  return roundValue(getTemp(index));
}

String OpenWeatherMapClient::getHumidity(int index) {
  return weathers[index].humidity;
}

String OpenWeatherMapClient::getHumidityRounded(int index) {
  return roundValue(getHumidity(index));
}

String OpenWeatherMapClient::getCondition(int index) {
  return weathers[index].condition;
}

String OpenWeatherMapClient::getCloudiness(int index) {
  return weathers[index].cloudiness;
}

String OpenWeatherMapClient::getWind(int index) {
  return weathers[index].wind;
}

String OpenWeatherMapClient::getWindRounded(int index) {
  return roundValue(getWind(index));
}

String OpenWeatherMapClient::getWeatherId(int index) {
  return weathers[index].weatherId;
}

String OpenWeatherMapClient::getDescription(int index) {
  return weathers[index].description;
}

String OpenWeatherMapClient::getIcon(int index) {
  return weathers[index].icon;
}

boolean OpenWeatherMapClient::getCached() {
  return weathers[0].cached;
}

String OpenWeatherMapClient::getMyCityIDs() {
  return myCityIDs;
}

String OpenWeatherMapClient::getError() {
  return weathers[0].error;
}

String OpenWeatherMapClient::getWeatherIcon(int index, int hours)
{
  int id = getWeatherId(index).toInt();
  //String icon = getIcon(index);
  String W = ")";

//  // clear sky
//  // 01d
//  if (icon == "01d")   {
//    return "B";
//  }
//  // 01n
//  if (icon == "01n")  {
//    W = "C";
//  }
//  // few clouds
//  // 02d
//  if (icon == "02d")  {
//    W = "H";
//  }
//  // 02n
//  if (icon == "02n")  {
//    W = "I";
//  }
//  // scattered clouds
//  // 03d
//  if (icon == "03d")  {
//    W = "N";
//  }
//  // 03n
//  if (icon == "03n")  {
//    W = "N";
//  }
//  // broken clouds
//  // 04d
//  if (icon == "04d")  {
//    W = "Y";
//  }
//  // 04n
//  if (icon == "04n")  {
//    W = "Y";
//  }
//  // shower rain
//  // 09d
//  if (icon == "09d")  {
//    W = "Q";
//  }
//  // 09n
//  if (icon == "09n")  {
//    W = "Q";
//  }
//  // rain
//  // 10d
//  if (icon == "10d")  {
//    W = "R";
//  }
//  // 10n
//  if (icon == "10n")  {
//    W = "R";
//  }
//  // thunderstorm
//  // 11d
//  if (icon == "11d")  {
//    W = "P";
//  }
//  // 11n
//  if (icon == "11n")  {
//    W = "P";
//  }
//  // snow
//  // 13d
//  if (icon == "13d")  {
//    W = "W";
//  }
//  // 13n
//  if (icon == "13n")  {
//    W = "W";
//  }
//  // mist
//  // 50d
//  if (icon == "50d")  {
//    W = "M";
//  }
//  // 50n
//  if (icon == "50n")  {
//    W = "M";
//  }

if (hours < 20 && hours > 6)
{
  switch(id)
  {
    case 800: W = "B"; break;
    case 801: W = "H"; break;
    case 802: W = "N"; break;
    case 803: W = "Y"; break;
    case 804: W = "%"; break;
    
    case 200: W = "Z"; break;
    case 201: W = "Z"; break;
    case 202: W = "Z"; break;
    case 210: W = "O"; break;
    case 211: W = "Z"; break;
    case 212: W = "0"; break;
    case 221: W = "Z"; break;
    case 230: W = "Z"; break;
    case 231: W = "Z"; break;
    case 232: W = "Z"; break;
    
    case 300: W = "Q"; break;
    case 301: W = "Q"; break;
    case 302: W = "Q"; break;
    case 310: W = "Q"; break;
    case 311: W = "Q"; break;
    case 312: W = "Q"; break;
    case 313: W = "Q"; break;
    case 314: W = "Q"; break;
    case 321: W = "Q"; break;
    
    case 500: W = "Q"; break;
    case 501: W = "R"; break;
    case 502: W = "R"; break;
    case 503: W = "R"; break;
    case 504: W = "R"; break;
    case 511: W = "R"; break;
    case 520: W = "R"; break;
    case 521: W = "R"; break;
    case 522: W = "R"; break;
    case 531: W = "R"; break;
    
    case 600: W = "V"; break;
    case 601: W = "W"; break;
    case 602: W = "W"; break;
    case 611: W = "W"; break;
    case 612: W = "W"; break;
    case 615: W = "W"; break;
    case 616: W = "W"; break;
    case 620: W = "W"; break;
    case 621: W = "W"; break;
    case 622: W = "W"; break;
    
    case 701: W = "M"; break;
    case 711: W = "M"; break;
    case 721: W = "M"; break;
    case 731: W = "M"; break;
    case 741: W = "M"; break;
    case 751: W = "M"; break;
    case 761: W = "M"; break;
    case 762: W = "M"; break;
    case 771: W = "M"; break;
    case 781: W = "M"; break;
    
    default:break; 
  }
} else if (hours > 19 && hours < 6)
{
  switch(id)
  {
    case 800: W = "C"; break;
    case 801: W = "I"; break;
    case 802: W = "N"; break;
    case 803: W = "Y"; break;
    case 804: W = "%"; break;
    
    case 200: W = "Z"; break;
    case 201: W = "Z"; break;
    case 202: W = "Z"; break;
    case 210: W = "O"; break;
    case 211: W = "Z"; break;
    case 212: W = "0"; break;
    case 221: W = "Z"; break;
    case 230: W = "Z"; break;
    case 231: W = "Z"; break;
    case 232: W = "Z"; break;
    
    case 300: W = "Q"; break;
    case 301: W = "Q"; break;
    case 302: W = "Q"; break;
    case 310: W = "Q"; break;
    case 311: W = "Q"; break;
    case 312: W = "Q"; break;
    case 313: W = "Q"; break;
    case 314: W = "Q"; break;
    case 321: W = "Q"; break;
    
    case 500: W = "Q"; break;
    case 501: W = "R"; break;
    case 502: W = "R"; break;
    case 503: W = "R"; break;
    case 504: W = "R"; break;
    case 511: W = "R"; break;
    case 520: W = "R"; break;
    case 521: W = "R"; break;
    case 522: W = "R"; break;
    case 531: W = "R"; break;
    
    case 600: W = "V"; break;
    case 601: W = "W"; break;
    case 602: W = "W"; break;
    case 611: W = "W"; break;
    case 612: W = "W"; break;
    case 615: W = "W"; break;
    case 616: W = "W"; break;
    case 620: W = "W"; break;
    case 621: W = "W"; break;
    case 622: W = "W"; break;
    
    case 701: W = "M"; break;
    case 711: W = "M"; break;
    case 721: W = "M"; break;
    case 731: W = "M"; break;
    case 741: W = "M"; break;
    case 751: W = "M"; break;
    case 761: W = "M"; break;
    case 762: W = "M"; break;
    case 771: W = "M"; break;
    case 781: W = "M"; break;
    
    default:break; 
  }
}
  
  return W;
}
