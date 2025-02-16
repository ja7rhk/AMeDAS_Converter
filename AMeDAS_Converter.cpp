#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <ctime>
#include <vector>
#include <list>
#include <algorithm>
#include "amedas_data.h"
#include "smet_convert.h"

//using namespace std;

// アメダス地点名のデータ
AMeDAS_Loc loc;

// データのリスト
std::vector<AMeDASData> data;

int main(int argc, char* argv[])
{
    std::string station_name;
    double altitude = 0;
    int res = 0;

    // コマンドライン引数のチェック
    if (argc > 1)
    {
        if (argc == 2)
            station_name = argv[1];
        else if (argc == 3)
        {
            station_name = argv[1];
            altitude = atof(argv[2]);
        }
        else
        {
            std::cout << "コマンドライン引数が多すぎます" << std::endl;
            return -1;
        }
    }
    else
    {
        std::cout << "アメダス地点名を指定して下さい" << std::endl;
        return -1;
    }

    //*************************************
    // アメダス地点名から観測地情報を取得
    //*************************************
    res = read_loc(station_name, altitude);
    if (res)
        return -1;

    //*************************************
    // アメダス観測データを読み込む
    //*************************************

    const std::string inputFile = station_name + ".csv";

    res = read_data(inputFile);
    if (res)
        return -1;

    // リスト内の各要素を表示
    //for (const auto& dt : data) {
    //    std::cout << "Wind_Velocity is " << dt.Wind_Velocity << std::endl;
    //}

    //*************************************
    // SMETに変換する
    //*************************************

    if (altitude == 0)
        altitude = loc.altitude;

    res = convert_solar_radiation();
    if (res)
        return -1;

    std::string station = loc.station_name;
    //小文字に変換する
    std::transform(
        station.begin(),
        station.end(),
        station.begin(),
        [](char c) { return std::tolower(c); }
    );

    res = write_smet(station, altitude);
    if (res)
        return -1;

    return 0;

}
