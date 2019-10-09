//Morita Daiki 2019 @NIT.yonago
//2019/09/25
//Arduino Nano (old etc..)
//Black Motor Driver &インチキエアー
// #include <serial_communication_v0.h>
#include "PwmMotor.h"
#include "gyro_integral.h"
//上下，前後，つかむ
const int air_pin[3] = {A1, A2, A3};
boolean air_state[3];
int speed[3];
// uint8_t values[5];

unsigned long serial_tim_last = 0, gyro_tim_last = 0;
boolean sta13 = false;
float want_deg = 0, rot = 0.0, error_angle = 0.0;
int vx, vy, vrot, vrot_old;

PwmMotor motor[3] = {
    PwmMotor(4, 2, 3),
    PwmMotor(7, 5, 6),
    PwmMotor(A0, 8, 9),

};
gyro_integral gyro_1;

void setup()
{
    for (int i = 0; i < 3; i++)
        pinMode(air_pin[i], OUTPUT);
    pinMode(13, OUTPUT);
    Serial.begin(9600);
    // Serial.println("tim,want,jsx,d/s,fild/s,angl");
    gyro_1.init(500);
}

void loop()
{
    if ((millis() - gyro_tim_last) > 4) //4ms以上経過したら積分
    {
        gyro_1.integral();        //積分
        gyro_tim_last = millis(); //時間更新
    }

    if ((millis() - serial_tim_last) > 1000) //操縦が長らくないときリセット
    {
        vx = 0;                   //速度0
        vy = 0;                   //速度0
        omni(0, 0, 0);            //スピードオフ
        want_deg = 0.0;           //目標リセット
        gyro_1.robot_angle = 0.0; //ロボット角度リセット
        gyro_1.init(500);
    }
    error_angle = want_deg - gyro_1.robot_angle;   //誤差
    rot = constrain(error_angle * 4.0, -255, 255); //誤差補正出力
    // omni(vx, vy, rot);
    // delay(4);

    if (Serial.available() > 0) //シリアル受信時
    {
        if (Serial.read() == 0xff) //ヘッダなら
        {
            digitalWrite(13, sta13);        //LEDちかちか
            sta13 = !sta13;                 //LEDちかちか
            uint8_t read_data[4];           //受信データ
            Serial.readBytes(read_data, 4); //受信
            serial_tim_last = millis();     //シリアルの受信時間更新

            vx = read_data[0] - 31;   //-31~32
            vy = read_data[1] - 31;   //-31~32
            vrot = read_data[2] - 15; //-15~16

            if ((vrot == 0) && (vrot_old != 0)) //JSが原点に戻った時
                want_deg = gyro_1.robot_angle;  //それを目標角度にする
            else                                //その他
                rot = vrot * 15;                //JSが操作中は旋回速度を指定
            vrot_old = vrot;                    //かこJS更新

            air_move(read_data[3]);
        }
    }
    if (vx == 0 && vy == 0)
        rot = 0;
    omni(vx, vy, rot); //速度出力
}

void air_move(uint8_t air_cmd) //0:上下,1:前後,2:爪
{
    if ((air_cmd >> 3) & 1) //👆
        air_state[0] = true;
    else if ((air_cmd >> 4) & 1) //👇
        air_state[0] = false;

    if ((air_cmd >> 1) & 1) //前
        air_state[1] = true;
    else if ((air_cmd >> 2) & 1) //後
        air_state[1] = false;

    if (!air_state[1]) //縮めるとき開けない
        air_state[2] = false;

    if ((air_cmd >> 0) & 1) //爪開閉
    {
        // if (air_state[1])                 //腕前の時だけ
        air_state[2] = !air_state[2]; //切り替え可能
    }
    for (int i = 0; i < 3; i++)
        digitalWrite(air_pin[i], air_state[i]);
}

void omni(int vx, int vy, float vrot)
{
    float vtheta = vrot;
    float v = sqrt(vx * vx + vy * vy) * 4.0;
    float theta = atan2(vy, vx);
    float R[3] = {1.0, 1.0, 1.0};
    for (int i = 0; i < 3; i++)
        speed[i] = -v * cos(theta - PI * 2.0 / 3.0 * i) - vtheta * R[i];
    int max = 255;
    for (int i = 0; i < 3; i++)
        max = max(abs(speed[i]), max);
    for (int i = 0; i < 3; i++)
    {
        speed[i] = map(speed[i], -max, max, -255, 255);
        motor[i].speed(speed[i]);
    }
}
