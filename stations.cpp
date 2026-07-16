#include "stations.h"

const RadioStation stations[] = {
  { 1,
    "Radio Cidade",
    "http://24273.live.streamtheworld.com/RADIOCIDADEAAC.aac" },
  { 2,
    "Antena 1",
    "https://antenaone.crossradio.com.br/stream/1/" },
  { 3,
    "CBN",
    "https://playerservices.streamtheworld.com/api/livestream-redirect/CBN_SPAAC.aac" },
  { 4,
    "Band News FM",
    "https://playerservices.streamtheworld.com/api/livestream-redirect/BANDNEWSFM_SPAAC.aac" },
  { 5,
    "BNR Radio",
    "https://stream.bnr.nl/bnr_mp3_128_20" },
  { 6,
    "Kiss FM",
    "http://24413.live.streamtheworld.com/RADIO_KISSFM_ADP.aac" },
  { 7,
    "89 FM",
    "http://26563.live.streamtheworld.com/RADIO_89FM_ADP.aac" },
  { 8,
    "Jovem Pan",
    "https://cast2.midiazdx.com.br:7110/stream" },
  { 9,
    "Radio Globo",
    "https://26673.live.streamtheworld.com/RADIO_GLOBO_RJAAC.aac" },
  { 10,
    "JB FM",
    "http://26683.live.streamtheworld.com/JBFMAAC.aac" }
};

const size_t STATION_COUNT = sizeof(stations) / sizeof(stations[0]);

const RadioStation* stationById(uint16_t id) {
  for (size_t i = 0; i < STATION_COUNT; i++) {
    if (stations[i].id == id) {
      return &stations[i];
    }
  }

  return nullptr;
}