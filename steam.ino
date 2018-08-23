/*********************************************************************

  steam.ino

    恒温槽制御プログラム
                                                                                                                                                                                                                    
  Copyright (c) 2018 Akihisa ONODA All rights reserved.

*********************************************************************/

#include <SPI.h>
#include <TimerOne.h>
#include <Adafruit_MAX31855.h>

/*********************************************************************
 *
 * ピンの定義
 *
 ********************************************************************/
#define SPI_CLK          3
#define SPI_CS_CH1       4
#define SPI_SI           5

#define HEATER           6


/*********************************************************************
 *
 * 恒温槽制御のパラメータ
 *
 ********************************************************************/
#define TEMP_TARGET 55.0

#define KP      0.01
#define KI      0.75
#define KD      2.00

int    is_init = 0;                 // 履歴が溜まったか否か
double term_p, term_i, term_d;     // 各制御量出力のためのパラメータ
double temp_target = 0.0;


/*********************************************************************
 *
 * 履歴の保持
 *
 ********************************************************************/
#define MAX_HISTORY      20
enum POS {
  POS_T1 = 0,
  POS_CN
};

int id = 0;                    // 履歴のIndex
double t_history[MAX_HISTORY]; // 恒温槽制御のための温度履歴
double t_prev[POS_CN];         // 正常に取得した温度履歴
double t_curr[POS_CN];              // 温度データ

#define t1               t_curr[POS_T1]


/*********************************************************************
 *
 * 温度データの補正
 *
 ********************************************************************/
#define checkThermocoupleValue(pos)                                  \
                      (isnan(t_curr[pos]) ? t_prev[pos] : t_curr[pos])


/*********************************************************************
 *
 * ログの出力
 *
 ********************************************************************/

#define BUF_SIZE         256
#define printString(ctrl, ctrl_calc)  do {                           \
                                        char buf[BUF_SIZE];          \
                                        char t1_buf[8];              \
                                        char ct_buf[8];              \
                                        char term_p_buf[8];          \
                                        char term_i_buf[8];          \
                                        char term_d_buf[8];          \
                                        char trg_buf[8];             \
                                        dtostrf(t1,                  \
                                                5, 2, t1_buf);       \
                                        dtostrf(ctrl_calc,           \
                                                5, 2, ct_buf);       \
                                        dtostrf(term_p,              \
                                                5, 2, term_p_buf);   \
                                        dtostrf(term_i,              \
                                                5, 2, term_i_buf);   \
                                        dtostrf(term_d,              \
                                                5, 2, term_d_buf);   \
                                        dtostrf(temp_target,         \
                                                5, 2, trg_buf);      \
                                        snprintf(buf, BUF_SIZE,      \
                                                 "%10lu\t"           \
                                                 "T1 = %s\t"         \
                                                 "ctrl = %2d\t"      \
                                                 "ctrl_calc = %s\t"  \
                                                 "P = %s\t"          \
                                                 "I = %s\t"          \
                                                 "D = %s\t"          \
                                                 "temp_trgt = %s"    \
                                                 "\r\n",             \
                                                 cnt,                \
                                                 t1_buf,             \
                                                 ctrl,               \
                                                 ct_buf,             \
                                                 term_p_buf,         \
                                                 term_i_buf,         \
                                                 term_d_buf,         \
                                                 trg_buf);           \
                                        Serial.print(buf);          \
                                      } while(0)


/*********************************************************************
 *
 * 全体のパラメータ
 *
 ********************************************************************/
#undef POWER_OFF

unsigned long cnt  = 0;    // 全体のカウンタ


/*********************************************************************
 *
 * 温度モジュールの初期化
 *   thermocouple_ch1: temperature of thermostatic chamber
 *
 ********************************************************************/
Adafruit_MAX31855 thermocouple_ch1(SPI_CLK, SPI_CS_CH1, SPI_SI);


/*********************************************************************
 * 【概要】  PIDの計算を行う
 *
 * 【引数】  temp: 温度データ
 *
 * 【戻り値】制御量
 *********************************************************************/
double
calcPID(double temp)
{
  double ctrl_calc = 0.0, dcs = 0.0, dc = 0.0, cs = 0.0;
  double param_p, param_i, param_d;
  int i, ic, ip;

  id = ((id + 1) < MAX_HISTORY) ? (id + 1) : 0;
  t_history[id] = temp;
  cs = temp;
  
  for (i = 0; i < (MAX_HISTORY - 1); i++) {
    ic = ((id - i) >= 0) ? (id - i) : (id - i + MAX_HISTORY);
    ip = ((ic - 1) >= 0) ? (ic - 1) : (ic - 1 + MAX_HISTORY); 
    dcs += t_history[ip] - t_history[ic];

    if (i == 0)
      dc = dcs;
      
    cs += t_history[ip]; 
  }

  ic = id;
  ip = ((ic - 1) >= 0) ? (ic - 1) : (ic - 1 + MAX_HISTORY);
  param_p = temp_target - t_history[ic];
  param_i = ((temp_target * MAX_HISTORY) - cs) / MAX_HISTORY;
  param_d = dcs / (MAX_HISTORY - 1);

  term_p = KP * param_p;
  term_i = KI * param_i;
  term_d = KD * param_d;

  ctrl_calc  = term_p + term_i + term_d;

  return ctrl_calc;
}


/*********************************************************************
 * 【概要】  設定温度の計算を行う
 *
 * 【引数】  なし
 *
 * 【戻り値】設定温度
 *********************************************************************/
double
calcTarget()
{
  double calc_target;

  if (temp_target < TEMP_TARGET) {
    calc_target = ((TEMP_TARGET - temp_target) / 3.0) + temp_target;
    return ((TEMP_TARGET - calc_target) > 1.0) ? calc_target : TEMP_TARGET;
  }

  return TEMP_TARGET;
}


/*********************************************************************
 * 【概要】  500[ms]毎に実行する関数
 *
 * 【引数】  なし
 *
 * 【戻り値】なし
 *
 * 【備考】  なし
 *********************************************************************/
void
timerExec()
{
   double ctrl_calc = 0.0;
   int ctrl = 0;

   t1 = thermocouple_ch1.readCelsius();;
   t1 = checkThermocoupleValue(POS_T1);

   if (t1 >= (temp_target - 2.0) &&
     temp_target < TEMP_TARGET) {
     temp_target = calcTarget();
   }

   ctrl_calc = calcPID(t1);

   // PID制御量適用のフラグ
   if (cnt == MAX_HISTORY && is_init == 0) {
     is_init = 1;
   }

   if (is_init == 0) {
     // 初期化が完了するまで制御量を適用しない
     ctrl_calc = 0.0;
   }

   // 制御量の微調整
   if (ctrl_calc < 0.0) {
     ctrl = 0;
   } else if (ctrl_calc > 0.50) {
     ctrl_calc += 1.0;
     ctrl = (int)ctrl_calc;
     ctrl_calc -= 1.0;
   }

   cnt++;

#ifdef POWER_OFF
   ctrl = 0;
#endif /* POWER_OFF */

   analogWrite(HEATER, ctrl);

   if (cnt > 0) {
     printString(ctrl, ctrl_calc);
   }

   t_prev[POS_T1] = t1;
}


/*********************************************************************
   【概要】  Arduinoの初期化関数

   【引数】  なし

   【戻り値】なし
*********************************************************************/
void
setup()
{
  int i;
  
  Serial.begin(9600);
 
  while (!Serial)
    delay(1);

  Serial.println("Thermostat Chamber Start.");
  delay(500);

  pinMode(HEATER, OUTPUT);
  for (i = 0; i < MAX_HISTORY; i++) {
    t_history[i] = -1.0;
  }
  cnt = 0;

  for (i = 0; i < POS_CN; i++) {
    t_prev[i] = -1.0;
    t_curr[i]  = -1.0;
  }

  temp_target = TEMP_TARGET / 2.0;

  // マイクロ秒単位で設定すること
  Timer1.initialize(500000);
  Timer1.attachInterrupt(timerExec);
}


/*********************************************************************
   【概要】  Arduinoのメイン関数

   【引数】  なし

   【戻り値】なし
*********************************************************************/
void
loop()
{

}
