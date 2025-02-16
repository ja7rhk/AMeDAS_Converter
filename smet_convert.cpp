#include "smet_convert.h"
#include <corecrt_math_defines.h>

// アメダス地点名のデータ
extern AMeDAS_Loc loc;

// データのリスト
extern std::vector<AMeDASData> data;

//---------------------------------------------
//  データ変換 ⇒ 日照・日射量換算
//---------------------------------------------
int convert_solar_radiation ()
{
    //const MidpointRounding away = MidpointRounding.AwayFromZero;

    //********************************************
    // (1) 大気外日射量の推定
    //********************************************
    // Latitude (rad)
    if (loc.station == "\n")
    {
        std::cout << "観測点を設定して下さい。" << std::endl;
        return -1;
    }
    double gamma = loc.longitude * (M_PI / 180);  // 東経γ(rad)
    double phi = loc.latitude * (M_PI / 180);     // 北緯φ(rad)

    for (int i = 0; i < data.size(); i++)
    //for (AMeDASData s : data)
    {
        //     <<太陽方位、高度、大気外日射量の計算>>
        // http://www.es.ris.ac.jp/~nakagawa/met_cal/solar.html
        //太陽から供給される日射は、地球大気系に有効な唯一のエネルギー源です。
        //任意の緯度φ、経度λの地点における任意の日時の太陽方位ψ、高度α、
        //大気外全天日射量Qは、計算で求めることができます。
        //先ず、次式
        //　　　θo=2π(dn-1)/365
        //struct tm ts = *localtime_s(&data[i].Timestamp);
        struct tm ts;
        localtime_s(&ts, &data[i].Timestamp);
        int dn = ts.tm_yday + 1;
        double theta0 = 2 * M_PI * (dn - 1) / 365;

        //により元旦からの通し日数dnに基いて定めたθoを用いて、当該日の
        //太陽赤緯δ
        //　　　δ=0.006918-0.399912cos(θo)+0.070257sin(θo)-0.006758cos(2θo)+0.000907sin(2θo)-0.002697cos(3θo)+0.001480sin(3θo)
        double delta = 0.006918 - 0.399912 * cos(theta0) + 0.070257 * sin(theta0);
        delta += -0.006758 * cos(2 * theta0) + 0.000907 * sin(2 * theta0);
        delta += -0.002697 * cos(3 * theta0) + 0.001480 * sin(3 * theta0);
        //地心太陽距離r/r*、
        //　　　r/r*=1/{1.000110+0.034221cos(θo)+0.001280sin(θo)+0.000719cos(2θo)+0.000077sin(2θo)}^0.5
        double rstar_r = 1.000110 + 0.034221 * cos(theta0) + 0.001280 * sin(theta0);
        rstar_r += 0.000719 * cos(2 * theta0) + 0.000077 * sin(2 * theta0);
        rstar_r = sqrt(rstar_r);   // r*/r
        //均時差Eq
        //　　　Eq=0.000075+0.001868cos(θo)-0.032077sin(θo)-0.014615cos(2θo)-0.040849sin(2θo)
        double Eq = 0.000075 + 0.001868 * cos(theta0) - 0.032077 * sin(theta0);
        Eq += -0.014615 * cos(2 * theta0) - 0.040849 * sin(2 * theta0);
        //
        //日本標準時間JSTから、太陽の時角hを求めます。
        //　　　h=(JST-12)π/12+標準子午線からの経度差+均時差(Eq)
        double h = (ts.tm_hour - 12) * M_PI / 12;
        h += gamma - (135 * M_PI / 180);
        h += Eq;
        //太陽赤緯δ、緯度φ、時角ｈの値が既知となったので、太陽方位ψ、高度αは、それぞれ、
        //　　　α=arcsin{sin(φ)sin(δ)+cos(φ)cos(δ)cos(h)}
        double alpha = asin(sin(phi) * sin(delta) + cos(phi) * cos(delta) * cos(h));
        //　　　ψ=arctan[cos(φ)cos(δ)sin(h)/{sin(φ)sin(α)-sin(δ)}]
        double psi = atan(cos(phi) * cos(delta) * sin(h) / (sin(phi) * sin(alpha) - sin(delta)));
        //として求めることができます。
        //最後に、大気外全天日射量Qを、
        //　　　Q=1367(r*/r)^(2)sin(α)
        double Q = 1367 * pow(rstar_r, 2) * sin(alpha);
        //により求めることができます。1367W/m^2は太陽定数です。
        if (Q < 0)
            data[i].Ext_Solar = 0;
        else
            data[i].Ext_Solar = Q;

        data[i].AirMass = 1 / sin(alpha);
    }

    //***************************
    // (2)ISWR(W/m^2)の計算
    //***************************

    double n = 0;      // 日照時間(0～1.0)
    double iswr_0 = 0;          // 大気外日射量(W/m^2)
    double iswr = 0;            // 時間毎の日射量(W/m^2)
    double hs = 0;              // 積雪深(cm)
    double m = 0;               // Air Mass
    double ps = 0;

    // genarate hourly weight and amount of it
    for (int i = 0; i < data.size(); i++)
    {
        // clear ISWR 
        data[i].ISWR = 0;
        iswr_0 = data[i].Ext_Solar;
        n = data[i].Daylight;
        hs = data[i].Height_Snow;
        m = data[i].AirMass;
        ps = data[i].Precipitation;

        //改良モデル(METPV-3)

        if (hs < 5)        //積雪時が5cm未満(積雪時のモデルを使用)またはデータ無(-999)
        {
            if (n > 0)         //日照時
            {
                if (m < 4)
                    iswr = iswr_0 * (0.353 - 0.0189 * m + (0.441 - 0.0447 * m) * (n - 0.1));
                else
                    iswr = iswr_0 * (0.277 + 0.263 * (n - 0.1));
            }
            else if (n == 0)      //不日照時
            {
                if (ps <= 0)    //無降水時またはデータ無(-999)
                {
                    if (m < 3.5)
                        iswr = iswr_0 * (0.223 - 0.0155 * m);
                    else //(m >= 3.5)
                        iswr = iswr_0 * 0.169;
                }
                else
                    iswr = iswr_0 * (0.100 - 0.006 * m);
            }
            else
                //日照時間の範囲外、エラー
                continue;
        }
        else        //積雪時が5cm以上(積雪時のモデルを使用)
        {
            if (n > 0)         //日照時
            {
                if (m < 4)
                    iswr = iswr_0 * (0.369 + (0.501 - 0.063 * m) * (n - 0.1));
                else //(m >= 4)
                    iswr = iswr_0 * (0.369 + 0.25 * (n - 0.1));
            }
            else if (n == 0)      //不日照時
            {
                if (ps <= 0)    //無降水時またはデータ無(-999)
                {
                    if (m < 4)
                        iswr = iswr_0 * (0.306 - 0.0132 * m);
                    else //(m >= 4)
                        iswr = iswr_0 * 0.253;
                }
                else if (ps > 0 && ps <= 1)
                {
                    if (m < 2.5)
                        iswr = iswr_0 * (0.372 - 0.0772 * m);
                    else
                        iswr = iswr_0 * 0.179;
                }
                else
                    iswr = iswr_0 * 0.111;
            }
            else
                //日照時間の範囲外、エラー
                continue;

        }
        //9～15時に不日照の場合の補正
        if (n == 0)
        {
            //struct tm ts = *localtime(&data[i].Timestamp);
            struct tm ts;
            localtime_s(&ts, &data[i].Timestamp);
            int hr_now = ts.tm_hour;
            // 最初のデータと最後のデータは以降でエラーが出るので何もしない
            if ((i - 1) < 0 || (i + 1) >= data.size())
            {
                data[i].ISWR = round(iswr);
                continue;
            }
            // 例外処理を入れてみる
            try
            {
                if (data[i - 1].Daylight > 0 && data[i + 1].Daylight > 0)
                    iswr = 1.47 * iswr;
            }
            catch (...)
            {
                std::cout << "9～15時に不日照の場合の補正でエラーがでました。" << std::endl;
            }
        }
        data[i].ISWR = round(iswr);
    }
    return 0;
}

//---------------------------------------------
//  データ変換 ⇒ SMET変換
//---------------------------------------------

/// <summary>
/// This function converts the string to the angle in degree, start with North(= 0 deg)
/// </summary>
/// <param name="sWD">srtring of wind direction.</param>
/// <returns> angle(deg) of wind direvion start with North(= 0 deg)</returns>
double isWindDirection(std::string sWD)
{
    double WD;

    if (sWD.compare("北") == 0)
        WD = 0;
    else if (sWD.compare("北北東") == 0)
        WD = 22.5 * 1;
    else if (sWD.compare("北東") == 0)
        WD = 22.5 * 2;
    else if (sWD.compare("東北東") == 0)
        WD = 22.5 * 3;
    else if (sWD.compare("東") == 0)
        WD = 22.5 * 4;
    else if (sWD.compare("東南東") == 0)
        WD = 22.5 * 5;
    else if (sWD.compare("南東") == 0)
        WD = 22.5 * 6;
    else if (sWD.compare("南南東") == 0)
        WD = 22.5 * 7;
    else if (sWD.compare("南") == 0)
        WD = 22.5 * 8;
    else if (sWD.compare("南南西") == 0)
        WD = 22.5 * 9;
    else if (sWD.compare("南西") == 0)
        WD = 22.5 * 10;
    else if (sWD.compare("西南西") == 0)
        WD = 22.5 * 11;
    else if (sWD.compare("西") == 0)
        WD = 22.5 * 12;
    else if (sWD.compare("西北西") == 0)
        WD = 22.5 * 13;
    else if (sWD.compare("北西") == 0)
        WD = 22.5 * 14;
    else if (sWD.compare("北北西") == 0)
        WD = 22.5 * 15;
    else
        WD = -999;

    return WD;
}

/// <summary>
/// This function genarates Wet adiabatic lapse rate(K/100m)
/// </summary>
/// <param name="temp">temperature (K)</param>
/// <param name="pressure">atmospheric pressure (hPa)</param>
/// <returns>Wet adiabatic lapse rate(K/100m)</returns>
double isLapseRate(double temp, double pressure)
{
    double T;       // temperature (K)
    double L;       // latent meat of evaporation of water (J/kg)
    double Es;      // saturated vapor pressure (hPa)
    double Pb;      // atmospheric pressure (hPa)
    double lapse_rate = 0.6;    // Wet adiabatic lapse rate(K/100m)

    T = temp;
    L = (596.73 - 0.601 * T) * 4.184 * 1000;
    Es = 6.1078 * pow(10, (7.5 * (T - 273.15)) / T);
    Pb = pressure;
    // temp value
    double c = (0.622 * L * Es) / (Pb * 287 * T);

    // Wet adiabatic lapse rate(K / 100m)
    lapse_rate = 0.976 * (1 + c) / (1 + (c * L * 0.622) / (1004 * T));

    return lapse_rate;
}

/// <summary>
/// this function genarates the atmospheric pressure of the station
/// based on sea level temperature and there atmospheric pressure.
/// if the sea level data are not measured (values should be '-999'),
/// this function returns default value.
/// </summary>
/// <param name="alt">altitude of the station (m)</param>
/// <param name="temp_0">sea level temperature (K)</param>
/// <param name="pressure_0">sea level atmospheric pressure (hPa)</param>
/// <returns>atmospheric pressure of the station (hPa)</returns>
double isPressure(double alt, double temp_0, double pressure_0)
{
    double T0 = 273.15;         // sea level temperature (K)
    double P0 = 1013.25;        // sea level pressure (hPa)
    double H;                   // altitude of the station (m)
    double pressure;

    if (temp_0 != -999)
        T0 = temp_0;
    if (pressure_0 != -999)
        P0 = pressure_0;
    H = alt;

    pressure = P0 * pow((1 - 0.0065 * H / T0), 5.257);

    return pressure;
}

// 地表温度を0℃とする。
bool TSGeq0 = true;

// 相対湿度の欠測値を70％とする。
bool RHeq70 = true;

int write_smet(std::string station_name, double altitude)
{
    std::string line;
    std::vector<std::string> read_feilds;
    std::string smetFile;

    double temp, tsg, rh, vw, dw = 0, iswr, hs, psum;
    double pre_dw = 0;  // previous valid direction of wind
    double Pb;    // atmospheric pressure (hPa)
    double walr = 0.6;  // Wet adiabatic lapse rate(K/100m)
    double alt_estimated = loc.revised_altitude;
    // if altitude is changed, add it to the station ID.
    bool altitude_val = false;

    if (loc.revised_altitude != loc.altitude)
        altitude_val = true;

    if (altitude_val == true)
        smetFile = station_name + "_c" + std::to_string((int)altitude) + ".smet";
    else
        smetFile = station_name + ".smet";

    std::cout << "SMET出力ファイル = " << smetFile << std::endl;

    // 書き込むファイルを開く(std::ofstreamのコンストラクタで開く)
    std::ofstream smet_file(smetFile);
    if (!smet_file)
    {
        std::cout << "ファイルオープン失敗" << std::endl;
        return -1;
    }

    if (TSGeq0 == true)
        std::cout << "地表温度を0℃に設定" << std::endl;
    if (RHeq70 == true)
        std::cout << "相対湿度の欠測に70(%)を挿入" << std::endl;
    if (altitude_val == true)
        std::cout << "気温を" << altitude << "(m)に補正" << std::endl;

    if (altitude_val == true)
    {
        loc.station_id = loc.station_id + "_c" + std::to_string((int)loc.revised_altitude);
        loc.station_name = loc.station_name + " (+" + std::to_string((int)(alt_estimated - loc.altitude)) + "m)";
    }

    ////// header //////
    smet_file << "SMET 1.1 ASCII" << std::endl;
    smet_file << "[HEADER]" << std::endl;
    smet_file << "station_id       = " << loc.station_id << std::endl;
    smet_file << "station_name     = " << loc.station_name << std::endl;
    smet_file << "latitude         = " << round(loc.latitude * 1000) / 1000 << std::endl;
    smet_file << "longitude        = " << round(loc.longitude * 1000) / 1000 << std::endl;

    if (altitude_val == false)
        smet_file << "altitude         = " << std::to_string((int)loc.altitude) << std::endl;
    else
        smet_file << "altitude         = " << std::to_string((int)alt_estimated) << std::endl ;

    smet_file << "nodata           = -999" << std::endl;
    smet_file << "tz               = +9" << std::endl;
    smet_file << "fields           = timestamp TA TSG RH VW DW ISWR HS PSUM" << std::endl;

    ////// data //////
    smet_file << "[DATA]" << std::endl;
    // calculate ISWR and Cloud_cover

    for (int i = 0; i < data.size(); i++)
    {
        if (data[i].Temperature != -999)
        {
            temp = data[i].Temperature + 273.15;        // [K]

            if (altitude_val == true)
            {
                // atmospheric pressure (hPa) at the estimated point
                // isPrassure(double alt, double temp_0, double pressure_0)
                Pb = isPressure
                        (alt_estimated                          // altitude to be estimated.
                        , (data[i].Temperature_0 + 273.15)      // sea level temperature.
                        , data[i].Pressure_0);                  // sea level pressure.

                // Wet adiabatic lapse rate(K / 100m)
                // isLapseRate(double temp, double pressure)
                walr = isLapseRate(temp, Pb);
                temp += (loc.altitude - alt_estimated) * walr / 100;
            }
        }
        else
        {
            temp = -999;
        }

        if (TSGeq0 == true)
            tsg = 273.15;                       // [K]
        else
            tsg = -999;                         // AMeDAS has no TSG data.

        if (data[i].Humidity != -999)
            rh = data[i].Humidity / 100;
        else
        {
            if (RHeq70 == true)
                rh = 0.7;
            else
                rh = -999;
        }

        if (data[i].Wind_Velocity != -999)
            vw = data[i].Wind_Velocity;                     // [m/sec]
        else
            vw = 0;

        if (data[i].Wind_Direction != "-999")
        {
            dw = isWindDirection(data[i].Wind_Direction);   // [deg]
            // save a valid direction of wind
            if (dw != -999)
                pre_dw = dw;
        }
        // if direction is invalid, then use previous one.
        if (dw == -999)
            dw = pre_dw;

        iswr = data[i].ISWR;                                // [W/m^2]

        if (data[i].Height_Snow != -999)
            hs = data[i].Height_Snow / 100;                 // [m]
        else
            hs = -999;

        psum = data[i].Precipitation;                       // [mm]

        
        //struct tm ts = *localtime(&data[i].Timestamp);
        struct tm ts;
        localtime_s(&ts, &data[i].Timestamp);
        char datetime[100];
        strftime(datetime, sizeof(datetime), "%Y-%m-%dT%H:%M", &ts);

        smet_file << datetime << "\t"
                    << round(temp * 10) / 10 << "\t" 
                    << tsg << "\t" 
                    << rh << "\t" 
                    << vw << "\t" 
                    << dw << "\t" 
                    << iswr << "\t" 
                    << hs << "\t" 
                    << psum << std::endl;
    }

    smet_file.close();

    std::cout << "書き込み完了" << std::endl;

    return 0;
}

