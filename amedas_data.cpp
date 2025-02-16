#include "amedas_data.h"

AMeDASData::AMeDASData() {

    Timestamp = 0;
}

AMeDAS_Loc::AMeDAS_Loc() {

    station = "invalid";
    station_id = "invalid";
    station_name = "invalid";
}


StringCode hashString(const std::string& str) {
    if (str == "気温(℃)")        return StringCode::temp;
    if (str == "降水量(mm)")      return StringCode::psum;
    if (str == "降雪(cm)")        return StringCode::psum_snow;
    if (str == "積雪(cm)")        return StringCode::hs;
    if (str == "日照時間(時間)")  return StringCode::daylight;
    if (str == "風速(m/s)")       return StringCode::vw;
    if (str == "風向")            return StringCode::dw;
    if (str == "相対湿度(％)")    return StringCode::rh;
    //if (str == "海面気温(℃)")    return StringCode::pressure_0;
    //if (str == "海面気圧(hPa)")   return StringCode::temp_0;
    return StringCode::unknown;
}

std::vector<std::string> split(std::string& input, char delimiter)
{
    std::istringstream stream(input);
    std::string field;
    std::vector<std::string> result;
    while (getline(stream, field, delimiter))
    {
        result.push_back(field);
    }
    return result;
}

time_t string_to_time_t(std::string s)
{
    int yy, mm, dd, hour, min, sec;
    struct tm when;
    time_t tme;
    size_t len = s.size();

    if (s[len - 3] == ':')
    {
        memset(&when, 0, sizeof(struct tm));
        sscanf_s(s.c_str(), "%d/%d/%d %d:%d", &yy, &mm, &dd, &hour, &min);
        time(&tme);
        _localtime64_s(&when, &tme);
        when.tm_year = yy - 1900;
        when.tm_mon = mm - 1;
        when.tm_mday = dd;
        when.tm_hour = hour;
        when.tm_min = min;
        when.tm_sec = 0;
        return _mktime64(&when);
    }
    else
    {
        memset(&when, 0, sizeof(struct tm));
        time(&tme);
        _localtime64_s(&when, &tme);
        when.tm_year = 1970;
        when.tm_mon = 1;
        when.tm_mday = 1;
        when.tm_hour = 0;
        when.tm_min = 0;
        when.tm_sec = 0;
        return _mktime64(&when);
    }
}

// アメダス地点名のデータ
extern AMeDAS_Loc loc;

int read_loc(std::string station_name, double altitude)
{
    std::string STATION_NAME = station_name;
    std::string line;
    std::vector<std::string> read_feilds;

    std::string locationFile = "AMeDAS_Location.csv";

    // 読み込むcsvファイルを開く(std::ifstreamのコンストラクタで開く)
    std::ifstream ifs_loc_file(locationFile, std::ios::in);
    if (ifs_loc_file.is_open())
    {
        std::cout << "アメダス地点情報 = " << locationFile << std::endl;
    }
    else
    {
        std::cout << "アメダス地点情報の入力ファイルがありません" << std::endl;
        return -1;
    }

    // getline関数で1行ずつ読み込む(読み込んだ内容はlineに格納)
    while (getline(ifs_loc_file, line))
    {
        read_feilds = split(line, ',');

        //***************************
        // CSVデータの読み出し
        //***************************
        std::string col = read_feilds[c_station_name];

        if (col.compare(STATION_NAME) == 0)
        {
            loc.station = read_feilds[c_station];
            loc.station_name = read_feilds[c_station_name];
            loc.station_id = read_feilds[c_station_id];
            loc.latitude = std::stof(read_feilds[c_latitude_1]) + std::stof(read_feilds[c_latitude_2]) / 60;
            loc.longitude = std::stof(read_feilds[c_longitude_1]) + std::stof(read_feilds[c_longitude_2]) / 60;
            loc.altitude = std::stof(read_feilds[c_altitude]);
            // ０ 以外は補正する
            if (altitude == 0)
                loc.revised_altitude = loc.altitude;
            else 
                loc.revised_altitude = altitude;
            // 処理を抜ける
            read_feilds.clear();
            break;
        }
        read_feilds.clear();
    }
    ifs_loc_file.close();

    // リスト内の各要素を表示
    std::cout << "station = " << loc.station << std::endl;
    std::cout << "station_id = " << loc.station_id << std::endl;
    std::cout << "station_name = " << loc.station_name << std::endl;
    std::cout << "latitude = " << loc.latitude << std::endl;
    std::cout << "longitude = " << loc.longitude << std::endl;
    std::cout << "altitude = " << loc.altitude << "(m)" << std::endl;
    std::cout << "revised_altitude = " << loc.revised_altitude << "(m)" << std::endl;
    std::cout << std::endl;

    return 0;
}

// データのリスト
extern std::vector<AMeDASData> data;

int read_data(std::string inputFile)
{
    std::string line;
    std::string str_conma_buf;
    //string input_csv_file_path = "in_data.csv";
    //string output_csv_file_path = "out_data.csv";
    std::vector<std::string> read_feilds;
    std::vector<std::string> header_0;
    std::vector<std::string> header_1;
    std::vector<std::string> header_2;
    int16_t n_col = 0;

    const int16_t c_date = 0;
    int16_t c_temp = 0, c_psum = 0, c_psum_snow = 0, c_hs = 0, c_daylight = 0, c_vw = 0, c_dw = 0, c_rh = 0;
    int16_t c_pressure_0 = 0, c_temp_0 = 0;

    std::cout << "アメダス入力ファイル = " << inputFile << std::endl;

    // 読み込むcsvファイルを開く(std::ifstreamのコンストラクタで開く)
    std::ifstream ifs_csv_file(inputFile, std::ios::in);
    if (!ifs_csv_file.is_open())
    {
        std::cout << "アメダス入力ファイルがありません" << std::endl;
        return -1;
    }

    // getline関数で1行ずつ読み込む(読み込んだ内容はlineに格納)
    while (getline(ifs_csv_file, line))
    {
        read_feilds = split(line, ',');

        //***************************
        // カラム番号を調べる
        //***************************
        if (read_feilds[c_date].compare("年月日時") == 0)
        {
            // ヘッダーの1行目を検出する
            header_0 = read_feilds;
            n_col = header_0.size();
            // 2行目を読み込む
            getline(ifs_csv_file, line);
            header_1 = split(line, ',');
            if (n_col > header_1.size())
                n_col = header_1.size();
            // 3行目を読み込む
            getline(ifs_csv_file, line);
            header_2 = split(line, ',');
            if (n_col > header_2.size())
                n_col = header_2.size();
            // ヘッダーから項目を抽出する
            for (int16_t col = 0; col < n_col; col++)
            {
                switch (hashString(header_0[col]))
                {
                case StringCode::temp:
                    if (header_1[col].compare("") == 0 && header_2[col].compare("") == 0)
                        c_temp = col;
                    break;
                case StringCode::psum:
                    if (header_1[col].compare("") == 0 && header_2[col].compare("") == 0)
                        c_psum = col;
                    break;
                case StringCode::psum_snow:
                    if (header_1[col].compare("") == 0 && header_2[col].compare("") == 0)
                        c_psum_snow = col;
                    break;
                case StringCode::hs:
                    if (header_1[col].compare("") == 0 && header_2[col].compare("") == 0)
                        c_hs = col;
                    break;
                case StringCode::daylight:
                    if (header_1[col].compare("") == 0 && header_2[col].compare("") == 0)
                        c_daylight = col;
                    break;
                case StringCode::vw:
                    if (header_1[col].compare("") == 0 && header_2[col].compare("") == 0)
                        c_vw = col;
                    else if (header_1[col].compare("風向") == 0 && header_2[col].compare("") == 0)
                        c_dw = col;
                    break;
                case StringCode::rh:
                    if (header_1[col].compare("") == 0 && header_2[col].compare("") == 0)
                        c_rh = col;
                    break;
                    /*
                case StringCode::temp_0:
                    if (header_1[col].compare("") == 0 && header_2[col].compare("") == 0)
                        c_temp_0 = col;
                    break;
                case StringCode::pressure_0:
                    if (header_1[col].compare("") == 0 && header_2[col].compare("") == 0)
                        c_pressure_0 = col;
                    break;
                    */
                default:
                    // (nothing to do)
                    break;
                }
            }
        }

        //***************************
        // CSVデータの読み出し
        //***************************
        // このラインが年月日時で始まるデータかチェック
        std::string d_min = "20xx/1/1 1:00";
        std::string d_max = "20xx/12/31 23:00";

        std::string col = read_feilds[c_date];
        if (col.size() < d_min.size() || col.size() > d_max.size() || col[col.size() - 3] != ':')
        {
            read_feilds.clear();
            continue;
        }

        AMeDASData s;
        s.Timestamp = string_to_time_t(read_feilds[c_date]);

        if ((c_temp != 0) && (read_feilds[c_temp] != ""))
            s.Temperature = std::stod(read_feilds[c_temp]);
        else
            s.Temperature = -999;

        if ((c_psum != 0) && (read_feilds[c_psum] != ""))
            s.Precipitation = std::stod(read_feilds[c_psum]);
        else
            s.Precipitation = -999;

        if ((c_psum_snow != 0) && (read_feilds[c_psum_snow] != ""))
            s.Precipitation_Snow = std::stod(read_feilds[c_psum_snow]);
        else
            s.Precipitation_Snow = -999;

        if ((c_hs != 0) && (read_feilds[c_hs] != ""))
            s.Height_Snow = std::stod(read_feilds[c_hs]);
        else
            s.Height_Snow = -999;

        if ((c_daylight != 0) && (read_feilds[c_daylight] != ""))
            s.Daylight = std::stod(read_feilds[c_daylight]);
        else
            s.Daylight = -999;

        s.Ext_Solar = -999;

        s.AirMass = -999;

        s.ISWR = -999;

        if ((c_vw != 0) && (read_feilds[c_vw] != ""))
            s.Wind_Velocity = std::stod(read_feilds[c_vw]);
        else
            s.Wind_Velocity = -999;

        if ((c_dw != 0) && (read_feilds[c_dw] != ""))
            s.Wind_Direction = read_feilds[c_dw];
        else
            s.Wind_Direction = "///";

        s.Wind_Angle = -999;

        if ((c_rh != 0) && (read_feilds[c_rh] != ""))
            s.Humidity = std::stod(read_feilds[c_rh]);
        else
            s.Humidity = -999;

        s.Weight_ISWR = -999;

        if ((c_temp_0 != 0) && (read_feilds[c_temp_0] != ""))
            s.Temperature_0 = std::stod(read_feilds[c_temp_0]);
        else
            s.Temperature_0 = -999;

        if ((c_pressure_0 != 0) && (read_feilds[c_pressure_0] != ""))
            s.Pressure_0 = std::stod(read_feilds[c_pressure_0]);
        else
            s.Pressure_0 = -999;

        // リストに追加する
        data.push_back(s);

        read_feilds.clear();
    }
    ifs_csv_file.close();


    // デバッグ用
    //std::cout << c_temp << " = " << header_0[c_temp] << std::endl;
    //std::cout << c_psum << " = " << header_0[c_psum] << std::endl;
    //std::cout << c_hs << " = " << header_0[c_hs] << std::endl;
    //std::cout << c_daylight << " = " << header_0[c_daylight] << std::endl;
    //std::cout << c_vw << " = " << header_0[c_vw] << std::endl;
    //std::cout << c_dw << " = " << header_0[c_dw] << std::endl;
    //std::cout << c_rh << " = " << header_0[c_rh] << std::endl;
    //std::cout << c_temp_0 << " = " << header_0[c_temp_0] << std:endl;
    //std::cout << c_pressure_0 << " = " << header_0[c_pressure_0] << std:endl;

    return 0;
}


