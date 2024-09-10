#include "display_icons.h"




const uint8_t *get_bitmap(int id, int clouds, bool day)
{
  bool cloudy = clouds > 60; 
  switch (id)
  {
  // Group 2xx: Thunderstorm
  case 200: // Thunderstorm  thunderstorm with light rain     11d
  case 201: // Thunderstorm  thunderstorm with rain           11d
  case 202: // Thunderstorm  thunderstorm with heavy rain     11d
  case 210: // Thunderstorm  light thunderstorm               11d
  case 211: // Thunderstorm  thunderstorm                     11d
  case 212: // Thunderstorm  heavy thunderstorm               11d
  case 221: // Thunderstorm  ragged thunderstorm              11d
    if (!cloudy && day)          {return wi_day_thunderstorm;}
    return wi_thunderstorm;
  case 230: // Thunderstorm  thunderstorm with light drizzle  11d
  case 231: // Thunderstorm  thunderstorm with drizzle        11d
  case 232: // Thunderstorm  thunderstorm with heavy drizzle  11d
    if (!cloudy && day)          {return wi_day_storm_showers;}
    return wi_storm_showers;
  // Group 3xx: Drizzle
  case 300: // Drizzle       light intensity drizzle          09d
  case 301: // Drizzle       drizzle                          09d
  case 302: // Drizzle       heavy intensity drizzle          09d
  case 310: // Drizzle       light intensity drizzle rain     09d
  case 311: // Drizzle       drizzle rain                     09d
  case 312: // Drizzle       heavy intensity drizzle rain     09d
  case 313: // Drizzle       shower rain and drizzle          09d
  case 314: // Drizzle       heavy shower rain and drizzle    09d
  case 321: // Drizzle       shower drizzle                   09d
    if (!cloudy && day)          {return wi_day_showers;}
    return wi_showers;
  // Group 5xx: Rain
  case 500: // Rain          light rain                       10d
  case 501: // Rain          moderate rain                    10d
  case 502: // Rain          heavy intensity rain             10d
  case 503: // Rain          very heavy rain                  10d
  case 504: // Rain          extreme rain                     10d
    if (!cloudy && day)                   {return wi_day_rain;}
    return wi_rain;
  case 511: // Rain          freezing rain                    13d
    if (!cloudy && day)          {return wi_day_rain_mix;}
    return wi_rain_mix;
  case 520: // Rain          light intensity shower rain      09d
  case 521: // Rain          shower rain                      09d
  case 522: // Rain          heavy intensity shower rain      09d
  case 531: // Rain          ragged shower rain               09d
    if (!cloudy && day)          {return wi_day_showers;}
    return wi_showers;
  // Group 6xx: Snow
  case 600: // Snow          light snow                       13d
  case 601: // Snow          Snow                             13d
  case 602: // Snow          Heavy snow                       13d
    if (!cloudy && day)                   {return wi_day_snow;}
    return wi_snow;
  case 611: // Snow          Sleet                            13d
  case 612: // Snow          Light shower sleet               13d
  case 613: // Snow          Shower sleet                     13d
    if (!cloudy && day)          {return wi_day_sleet;}
    return wi_sleet;
  case 615: // Snow          Light rain and snow              13d
  case 616: // Snow          Rain and snow                    13d
  case 620: // Snow          Light shower snow                13d
  case 621: // Snow          Shower snow                      13d
  case 622: // Snow          Heavy shower snow                13d
    if (!cloudy && day)          {return wi_day_rain_mix;}
    return wi_rain_mix;
  // Group 7xx: Atmosphere
  case 701: // Mist          mist                             50d
    if (!cloudy && day)          {return wi_day_fog;}
    return wi_fog;

  case 741: // Fog           fog                              50d
    if (!cloudy && day)          {return wi_day_fog;}
    return wi_fog;
  // Group 800: Clear
  case 800: // Clear         clear sky                        01d 01n
    if (!day)  {return wi_night_clear;}
    return wi_day_sunny;
  // Group 80x: Clouds
  case 801: // Clouds        few clouds: 11-25%               02d 02n
    if (!day)  {return wi_night_alt_partly_cloudy;}
    return wi_day_sunny_overcast;
  case 802: // Clouds        scattered clouds: 25-50%         03d 03n
  case 803: // Clouds        broken clouds: 51-84%            04d 04n
    if (!day)          {return wi_cloudy;}
    return wi_day_cloudy;
  case 804: // Clouds        overcast clouds: 85-100%         04d 04n
    return wi_cloudy;
  default:
    // maybe this is a new icon in one of the existing groups
    if (id >= 200 && id < 300) {return wi_thunderstorm;}
    if (id >= 300 && id < 400) {return wi_showers;}
    if (id >= 500 && id < 600) {return wi_rain;}
    if (id >= 600 && id < 700) {return wi_snow;}
    if (id >= 700 && id < 800) {return wi_fog;}
    if (id >= 800 && id < 900) {return wi_cloudy;}
    return wi_na;
  }
}