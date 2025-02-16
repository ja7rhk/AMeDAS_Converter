#include "smet_convert.h"
#include <corecrt_math_defines.h>

// �A���_�X�n�_���̃f�[�^
extern AMeDAS_Loc loc;

// �f�[�^�̃��X�g
extern std::vector<AMeDASData> data;

//---------------------------------------------
//  �f�[�^�ϊ� �� ���ƁE���˗ʊ��Z
//---------------------------------------------
int convert_solar_radiation ()
{
    //const MidpointRounding away = MidpointRounding.AwayFromZero;

    //********************************************
    // (1) ��C�O���˗ʂ̐���
    //********************************************
    // Latitude (rad)
    if (loc.station == "\n")
    {
        std::cout << "�ϑ��_��ݒ肵�ĉ������B" << std::endl;
        return -1;
    }
    double gamma = loc.longitude * (M_PI / 180);  // ���o��(rad)
    double phi = loc.latitude * (M_PI / 180);     // �k�܃�(rad)

    for (int i = 0; i < data.size(); i++)
    //for (AMeDASData s : data)
    {
        //     <<���z���ʁA���x�A��C�O���˗ʂ̌v�Z>>
        // http://www.es.ris.ac.jp/~nakagawa/met_cal/solar.html
        //���z���狟���������˂́A�n����C�n�ɗL���ȗB��̃G�l���M�[���ł��B
        //�C�ӂ̈ܓx�ӁA�o�x�ɂ̒n�_�ɂ�����C�ӂ̓����̑��z���ʃՁA���x���A
        //��C�O�S�V���˗�Q�́A�v�Z�ŋ��߂邱�Ƃ��ł��܂��B
        //�悸�A����
        //�@�@�@��o=2��(dn-1)/365
        //struct tm ts = *localtime_s(&data[i].Timestamp);
        struct tm ts;
        localtime_s(&ts, &data[i].Timestamp);
        int dn = ts.tm_yday + 1;
        double theta0 = 2 * M_PI * (dn - 1) / 365;

        //�ɂ�茳�U����̒ʂ�����dn�Ɋ�Ē�߂���o��p���āA���Y����
        //���z�Ԉ܃�
        //�@�@�@��=0.006918-0.399912cos(��o)+0.070257sin(��o)-0.006758cos(2��o)+0.000907sin(2��o)-0.002697cos(3��o)+0.001480sin(3��o)
        double delta = 0.006918 - 0.399912 * cos(theta0) + 0.070257 * sin(theta0);
        delta += -0.006758 * cos(2 * theta0) + 0.000907 * sin(2 * theta0);
        delta += -0.002697 * cos(3 * theta0) + 0.001480 * sin(3 * theta0);
        //�n�S���z����r/r*�A
        //�@�@�@r/r*=1/{1.000110+0.034221cos(��o)+0.001280sin(��o)+0.000719cos(2��o)+0.000077sin(2��o)}^0.5
        double rstar_r = 1.000110 + 0.034221 * cos(theta0) + 0.001280 * sin(theta0);
        rstar_r += 0.000719 * cos(2 * theta0) + 0.000077 * sin(2 * theta0);
        rstar_r = sqrt(rstar_r);   // r*/r
        //�ώ���Eq
        //�@�@�@Eq=0.000075+0.001868cos(��o)-0.032077sin(��o)-0.014615cos(2��o)-0.040849sin(2��o)
        double Eq = 0.000075 + 0.001868 * cos(theta0) - 0.032077 * sin(theta0);
        Eq += -0.014615 * cos(2 * theta0) - 0.040849 * sin(2 * theta0);
        //
        //���{�W������JST����A���z�̎��ph�����߂܂��B
        //�@�@�@h=(JST-12)��/12+�W���q�ߐ�����̌o�x��+�ώ���(Eq)
        double h = (ts.tm_hour - 12) * M_PI / 12;
        h += gamma - (135 * M_PI / 180);
        h += Eq;
        //���z�Ԉ܃A�ܓx�ӁA���p���̒l�����m�ƂȂ����̂ŁA���z���ʃՁA���x���́A���ꂼ��A
        //�@�@�@��=arcsin{sin(��)sin(��)+cos(��)cos(��)cos(h)}
        double alpha = asin(sin(phi) * sin(delta) + cos(phi) * cos(delta) * cos(h));
        //�@�@�@��=arctan[cos(��)cos(��)sin(h)/{sin(��)sin(��)-sin(��)}]
        double psi = atan(cos(phi) * cos(delta) * sin(h) / (sin(phi) * sin(alpha) - sin(delta)));
        //�Ƃ��ċ��߂邱�Ƃ��ł��܂��B
        //�Ō�ɁA��C�O�S�V���˗�Q���A
        //�@�@�@Q=1367(r*/r)^(2)sin(��)
        double Q = 1367 * pow(rstar_r, 2) * sin(alpha);
        //�ɂ�苁�߂邱�Ƃ��ł��܂��B1367W/m^2�͑��z�萔�ł��B
        if (Q < 0)
            data[i].Ext_Solar = 0;
        else
            data[i].Ext_Solar = Q;

        data[i].AirMass = 1 / sin(alpha);
    }

    //***************************
    // (2)ISWR(W/m^2)�̌v�Z
    //***************************

    double n = 0;      // ���Ǝ���(0�`1.0)
    double iswr_0 = 0;          // ��C�O���˗�(W/m^2)
    double iswr = 0;            // ���Ԗ��̓��˗�(W/m^2)
    double hs = 0;              // �ϐ�[(cm)
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

        //���ǃ��f��(METPV-3)

        if (hs < 5)        //�ϐ᎞��5cm����(�ϐ᎞�̃��f�����g�p)�܂��̓f�[�^��(-999)
        {
            if (n > 0)         //���Ǝ�
            {
                if (m < 4)
                    iswr = iswr_0 * (0.353 - 0.0189 * m + (0.441 - 0.0447 * m) * (n - 0.1));
                else
                    iswr = iswr_0 * (0.277 + 0.263 * (n - 0.1));
            }
            else if (n == 0)      //�s���Ǝ�
            {
                if (ps <= 0)    //���~�����܂��̓f�[�^��(-999)
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
                //���Ǝ��Ԃ͈̔͊O�A�G���[
                continue;
        }
        else        //�ϐ᎞��5cm�ȏ�(�ϐ᎞�̃��f�����g�p)
        {
            if (n > 0)         //���Ǝ�
            {
                if (m < 4)
                    iswr = iswr_0 * (0.369 + (0.501 - 0.063 * m) * (n - 0.1));
                else //(m >= 4)
                    iswr = iswr_0 * (0.369 + 0.25 * (n - 0.1));
            }
            else if (n == 0)      //�s���Ǝ�
            {
                if (ps <= 0)    //���~�����܂��̓f�[�^��(-999)
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
                //���Ǝ��Ԃ͈̔͊O�A�G���[
                continue;

        }
        //9�`15���ɕs���Ƃ̏ꍇ�̕␳
        if (n == 0)
        {
            //struct tm ts = *localtime(&data[i].Timestamp);
            struct tm ts;
            localtime_s(&ts, &data[i].Timestamp);
            int hr_now = ts.tm_hour;
            // �ŏ��̃f�[�^�ƍŌ�̃f�[�^�͈ȍ~�ŃG���[���o��̂ŉ������Ȃ�
            if ((i - 1) < 0 || (i + 1) >= data.size())
            {
                data[i].ISWR = round(iswr);
                continue;
            }
            // ��O���������Ă݂�
            try
            {
                if (data[i - 1].Daylight > 0 && data[i + 1].Daylight > 0)
                    iswr = 1.47 * iswr;
            }
            catch (...)
            {
                std::cout << "9�`15���ɕs���Ƃ̏ꍇ�̕␳�ŃG���[���ł܂����B" << std::endl;
            }
        }
        data[i].ISWR = round(iswr);
    }
    return 0;
}

//---------------------------------------------
//  �f�[�^�ϊ� �� SMET�ϊ�
//---------------------------------------------

/// <summary>
/// This function converts the string to the angle in degree, start with North(= 0 deg)
/// </summary>
/// <param name="sWD">srtring of wind direction.</param>
/// <returns> angle(deg) of wind direvion start with North(= 0 deg)</returns>
double isWindDirection(std::string sWD)
{
    double WD;

    if (sWD.compare("�k") == 0)
        WD = 0;
    else if (sWD.compare("�k�k��") == 0)
        WD = 22.5 * 1;
    else if (sWD.compare("�k��") == 0)
        WD = 22.5 * 2;
    else if (sWD.compare("���k��") == 0)
        WD = 22.5 * 3;
    else if (sWD.compare("��") == 0)
        WD = 22.5 * 4;
    else if (sWD.compare("���쓌") == 0)
        WD = 22.5 * 5;
    else if (sWD.compare("�쓌") == 0)
        WD = 22.5 * 6;
    else if (sWD.compare("��쓌") == 0)
        WD = 22.5 * 7;
    else if (sWD.compare("��") == 0)
        WD = 22.5 * 8;
    else if (sWD.compare("��쐼") == 0)
        WD = 22.5 * 9;
    else if (sWD.compare("�쐼") == 0)
        WD = 22.5 * 10;
    else if (sWD.compare("���쐼") == 0)
        WD = 22.5 * 11;
    else if (sWD.compare("��") == 0)
        WD = 22.5 * 12;
    else if (sWD.compare("���k��") == 0)
        WD = 22.5 * 13;
    else if (sWD.compare("�k��") == 0)
        WD = 22.5 * 14;
    else if (sWD.compare("�k�k��") == 0)
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

// �n�\���x��0���Ƃ���B
bool TSGeq0 = true;

// ���Ύ��x�̌����l��70���Ƃ���B
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

    std::cout << "SMET�o�̓t�@�C�� = " << smetFile << std::endl;

    // �������ރt�@�C�����J��(std::ofstream�̃R���X�g���N�^�ŊJ��)
    std::ofstream smet_file(smetFile);
    if (!smet_file)
    {
        std::cout << "�t�@�C���I�[�v�����s" << std::endl;
        return -1;
    }

    if (TSGeq0 == true)
        std::cout << "�n�\���x��0���ɐݒ�" << std::endl;
    if (RHeq70 == true)
        std::cout << "���Ύ��x�̌�����70(%)��}��" << std::endl;
    if (altitude_val == true)
        std::cout << "�C����" << altitude << "(m)�ɕ␳" << std::endl;

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

    std::cout << "�������݊���" << std::endl;

    return 0;
}

