#pragma once
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <ctime>
#include <vector>
#include <list>
#include "amedas_data.h"

int convert_solar_radiation();

double isWindDirection(std::string sWD);

double isLapseRate(double temp, double pressure);

double isPressure(double alt, double temp_0, double pressure_0);

int write_smet(std::string station_name, double altitude);


