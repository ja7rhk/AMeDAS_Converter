#pragma once
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <ctime>
#include <vector>
#include <list>

class AMeDASData
{
public:
    AMeDASData();
    // Auto-implemented properties.
    //struct tm* Timestamp;
    std::time_t Timestamp;
    double Temperature;     // (C-deg)
    double Precipitation;
    double Precipitation_Snow;
    double Height_Snow;
    double Daylight;
    //public double Pressure ;        // (hPa) if it is availlble.
    double Ext_Solar;       // 大気外日射量(extraterrestrial solar radiation)(W/m^2).
    double AirMass;         // Air Mass = 1 / sin(α).
    double ISWR;            // calculated ISWR(W/m^2) using Daylight.
    double Wind_Velocity;
    std::string Wind_Direction;  // 東西南北で表記
    int Wind_Angle;      // wind direction (deg).
    double Humidity;
    //public double Cos_ISWR ;      // hourly ISWR curve.
    double Weight_ISWR;     // 太陽高度hからsin(h)
    double Pressure_0;      // sea level atmospheric pressure(hPa).
    double Temperature_0;   // sea level temperature(C-deg)).
};

class AMeDAS_Loc
{
public:
    AMeDAS_Loc();
    // Auto-implemented properties.
    std::string station;
    std::string station_id;
    std::string station_name;
    double latitude;
    double longitude;
    double  altitude;
    double  revised_altitude;
};

enum ColLocation
{
    c_station = 0,
    c_station_id,
    c_station_name,
    c_latitude_1,
    c_latitude_2,
    c_longitude_1,
    c_longitude_2,
    c_altitude,
    unknown
};


enum class StringCode
{
    temp,
    psum,
    psum_snow,
    hs,
    daylight,
    vw,
    dw,
    rh,
    //pressure_0,
    //temp_0,
    unknown
};

StringCode hashString(const std::string& str);

std::vector<std::string> split(std::string& input, char delimiter);

time_t string_to_time_t(std::string s);

int read_loc(std::string station_name, double altitude);

int read_data(std::string inputFile);
