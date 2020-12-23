/*
 * CyberStick To USB
 * (Arduino Pro Micro)
 *
 *DSUB9P  --  pro micro  gpio
 *  1     --   d4(D0)     d4
 *  2     --   d5(D1)     c6
 *  3     --   d6(D2)     d7
 *  4     --   d7(D3)     e6
 *  5     --   Vcc(+5V)
 *  6     --   d2 (LH)    d1(int1)
 *  7     --   d3 (ACK)   d0(int0)
 *  8     --   d8 (REQ)   b4
 *  9     --   GND
 */

#include <Joystick.h>

Joystick_ Joystick = Joystick_(
  0x03,                    // reportid
  JOYSTICK_TYPE_JOYSTICK,  // type
  10,                      // button count
  0,                       // hat switch count
  true,                    // x axis enable
  true,                    // y axis enable
  false,                   // z axis enable
  true,                   // right x axis enable
  true,                   // right y axis enable
  false,                   // right z axis enable
  false,                   // rudder enable
  false,                    // throttle enable
  false,                   // accelerator enable
  false,                   // brake enable
  false                    // steering enable
);

// データ
#define AXIS_X    0       // 操縦桿左右 (左00H～右FFH)
#define AXIS_Y    1       // 操縦桿前後 (奥00H～手前FFH)
#define RAXIS_Y   2       // 操縦桿2前後 (奥00H～手前FFH)
#define BTN       3       // ボタン類１

#define BTN_END  4        // 最大バッファ
int data[BTN_END];

// gpio
#define DBUS1   4
#define DBUS2   5
#define DBUS3   6
#define DBUS4   7
#define LH      2
#define ACK     3
#define REQ     8

#define RCVSIZE  12
volatile byte rcvdata[RCVSIZE];
volatile int rcvcnt;

#define LOOP_INTERVAL 16  // 入力周期(msec)
unsigned long loopTimer;

// セットアップ
void setup() {
  
  // シリアル初期化
//  Serial.begin(115200);
//  while (!Serial);

  // pin初期設定
  pinMode(DBUS1,INPUT_PULLUP);
  pinMode(DBUS2,INPUT_PULLUP);
  pinMode(DBUS3,INPUT_PULLUP);
  pinMode(DBUS4,INPUT_PULLUP);
  pinMode(ACK,INPUT_PULLUP);
  pinMode(REQ,OUTPUT) ;
  PORTB |= 0b00010000 ;

  // ジョイスティック開始
  Joystick.begin();

  // ジョイスティック初期設定
  Joystick.setXAxisRange(0, 254);
  Joystick.setYAxisRange(0, 254);
  Joystick.setRxAxisRange(0, 254);
  Joystick.setRyAxisRange(0, 254);

  // CyberStick の仕様
  // (データの取り込みタイミングをアタッチ)
  // 割り込みチャンネル
  // 割り込み時の処理関数
  // 割り込み条件 FALLING ピンの状態がHIGHからLOWに変わったときに発生
  attachInterrupt(0, riseACK, FALLING);
}

int ck;
// メインループ
void loop() {
  
  loopTimer = millis();

  if(ck == 0) {
    PORTB &= 0b11111110;
    ck=1;
  } else {
    PORTB |= 0b00000001;
    ck=0;
  }
  // CyberStick からデータ読み込み
  getCyberStickStatus();

  // アナログ入力 ----------
  Joystick.setXAxis(data[AXIS_X]);
  Joystick.setYAxis(data[AXIS_Y]);
  Joystick.setRyAxis(data[RAXIS_Y]);

  // ボタン入力 ------------
  // a
  if(data[BTN] & (1 << 7)) {
    Joystick.setButton(0, 1);
  } else {
    Joystick.setButton(0, 0);
  }
  // b
  if(data[BTN] & (1 << 6)) {
    Joystick.setButton(1, 1);
  } else {
    Joystick.setButton(1, 0);
  }
  // c
  if(data[BTN] & (1 << 5)) {
    Joystick.setButton(2, 1);
  } else {
    Joystick.setButton(2, 0);
  }
  // d
  if(data[BTN] & (1 << 4)) {
    Joystick.setButton(3, 1);
  } else {
    Joystick.setButton(3, 0);
  }
  // e1
  if(data[BTN] & (1 << 2)) {
    Joystick.setButton(4, 1);
  } else {
    Joystick.setButton(4, 0);
  }
  // e2
  if(data[BTN] & (1 << 3)) {
    Joystick.setButton(5, 1);
  } else {
    Joystick.setButton(5, 0);
  }
  // start
  if(data[BTN] & (1 << 1)) {
    Joystick.setButton(6, 1);
  } else {
    Joystick.setButton(6, 0);
  }
  // select
  if(data[BTN] & (1 << 0)) {
    Joystick.setButton(7, 1);
  } else {
    Joystick.setButton(7, 0);
  }
//  // a'
//  if(data[BTN] & (1 << 9)) {
//    Joystick.setButton(8, 1);
//  } else {
//    Joystick.setButton(8, 0);
//  }
//  // b'
//  if(data[BTN] & (1 << 8)) {
//    Joystick.setButton(9, 1);
//  } else {
//    Joystick.setButton(9, 0);
//  }

  // 処理待ち
  delay(LOOP_INTERVAL);

	// HID デバイスへの出力に反映させる
	Joystick.sendState();
}

// CYBERSTICKのハードウェアデータ取り込み
void getCyberStickStatus(){

  rcvcnt = 0;
  // d8(REQ) LOW
  // CyberStick がデータを吐き出す
  PORTB &= 0b11101111;
  while(rcvcnt < RCVSIZE) {
    if(loopTimer < millis() + LOOP_INTERVAL) {
      // d8(REQ) HIGH
      PORTB |= 0b00010000 ;
      // タイムアウト
      break;
    }
    if(rcvcnt == 1) {
      // d8(REQ) HIGH
      PORTB |= 0b00010000 ;
    }
  }

  // サイクリックレバー X軸
  data[AXIS_X] = rcvdata[3] & 0xf0 | ((rcvdata[7]>>4) & 0x0f);

  // サイクリックレバー Y軸
  data[AXIS_Y] = rcvdata[2] & 0xf0 | ((rcvdata[6]>>4) & 0x0f);

  // サイクリックレバー Y軸
  data[RAXIS_Y] = rcvdata[4] & 0xf0 | ((rcvdata[8]>>4) & 0x0f);

  //  ボタン
  data[BTN] = rcvdata[0] & 0xf0 | ((rcvdata[1]>>4) & 0x0f);
  data[BTN] = 0xff - data[BTN] ;  // 逆倫理

//  //-------------------------
//  Serial.print(data[BTN], BIN);
//  Serial.print(",");
//  for (int i = 1; i < RCVSIZE; i++) {
//    Serial.print(rcvdata[i]);
//    Serial.print(",");
//    Serial.print(i);
//    Serial.print(":");
//  }
//  Serial.println("");
//  //-------------------------
}

// サイバースティックのクロックデータの取得
void riseACK() {

  byte retByte = 0;
  retByte |= (PIND & 0b00010000) ;
  retByte |= (PINC & 0b01000000) >> 1;
  retByte |= (PIND & 0b10000000) >> 1;
  retByte |= (PINE & 0b01000000) << 1;
  rcvdata[rcvcnt++] = retByte;
}
